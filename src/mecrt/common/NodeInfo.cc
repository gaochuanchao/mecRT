//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecCommon.h/MecCommon.cc
//
//  Description:
//    Collect metadata for different submodules of this node,
//    make it easier to fetch the information from different modules within the same node.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-16
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/common/NodeInfo.h"
#include "mecrt/nic/mac/GnbMac.h"
#include "mecrt/coreNetwork/NodePacketController.h"
#include "mecrt/apps/server/Server.h"

Define_Module(NodeInfo);


NodeInfo::NodeInfo()
{
    enableInitDebug_ = false;

    nodeType_ = "";
    nodeState_ = true;

    nodeAddr_ = inet::Ipv4Address::UNSPECIFIED_ADDRESS;
    rtState_ = false;
    npcSocketId_ = -1;

    nodeId_ = -1;
    nicInterfaceId_ = -1;
    nicState_ = true;

    serverState_ = true;
    serverPort_ = -1;
    serverSocketId_ = -1;

    isGlobalScheduler_ = false;
    localSchedulerPort_ = -1;
    globalSchedulerAddr_ = inet::Ipv4Address::UNSPECIFIED_ADDRESS;
    scheduleInterval_ = 10.0; // default 10 seconds
    appStopInterval_ = 0.05; // default 0.05 seconds
    localSchedulerSocketId_ = -1;

    gnbMac_ = nullptr;
    npc_ = nullptr;
    server_ = nullptr;

    rsuStatusTimer_ = nullptr;
    nodeDownTimer_ = nullptr;
    ifDownTimer_ = nullptr;
}


NodeInfo::~NodeInfo()
{
    if (enableInitDebug_)
        std::cout << "NodeInfo::~NodeInfo - destroying NodeInfo module\n";

    if (rsuStatusTimer_)
    {
        cancelAndDelete(rsuStatusTimer_);
        rsuStatusTimer_ = nullptr;
    }

    if (enableInitDebug_)
        std::cout << "NodeInfo::~NodeInfo - destroying NodeInfo module done!\n";
}


void NodeInfo::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
        
        if (enableInitDebug_)
            std::cout << "NodeInfo:initialize - stage: INITSTAGE_LOCAL - begins\n";

        EV_INFO << "NodeInfo:initialize - stage: INITSTAGE_LOCAL\n";        
        // initialize default values
        nodeType_ = par("nodeType").stdstringValue();

        rsuStatusTimer_ = new cMessage("rsuStatusTimer");

        WATCH(nodeType_);
        WATCH(nodeState_);
        WATCH(nodeAddr_);
        WATCH(rtState_);
        WATCH(npcSocketId_);
        WATCH(nodeId_);
        WATCH(nicInterfaceId_);
        WATCH(nicState_);
        WATCH(serverState_);
        WATCH(serverPort_);
        WATCH(serverSocketId_);
        WATCH(isGlobalScheduler_);
        WATCH(globalSchedulerAddr_);
        WATCH(localSchedulerPort_);
        WATCH(scheduleInterval_);
        WATCH(appStopInterval_);
        WATCH(localSchedulerSocketId_);

        if (enableInitDebug_)
            std::cout << "NodeInfo:initialize - stage: INITSTAGE_LOCAL - ends\n";
    }
}


void NodeInfo::handleMessage(omnetpp::cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (msg == rsuStatusTimer_)
        {
            EV << "NodeInfo:handleMessage - rsuStatusTimer is triggered, update RSU status to the global scheduler\n";
            // send RSU status to the global scheduler
            if (!globalSchedulerAddr_.isUnspecified())
            {
                // check if the time is too early (the NEXT_SCHEDULING_TIME might be updated by other global scheduler)
                double nextUpdateTime = NEXT_SCHEDULING_TIME - appStopInterval_;
                if (simTime() < nextUpdateTime)
                {
                    EV << "NodeInfo:handleMessage - the time is too early to send RSU status update, reschedule rsuStatusTimer at "
                        << nextUpdateTime << "\n";
                    scheduleAt(nextUpdateTime, rsuStatusTimer_);
                    return;
                }

                recoverRsuStatus();
                scheduleAt(nextUpdateTime + scheduleInterval_, rsuStatusTimer_);
            }
        }
        return;
    }
    else
    {
        EV << "NodeInfo:handleMessage - received an unexpected message: " << msg->getName() << "\n";
        delete msg;
        msg = nullptr;
        return;
    }
}


void NodeInfo::setGlobalSchedulerAddr(inet::Ipv4Address addr)
{
    // this function is called by other modules (the nodeInfo_), so we need to use
    // Enter_Method or Enter_Method_Silent to tell the simulation kernel "switch context to this module"
    Enter_Method_Silent("setGlobalSchedulerAddr");

    // if the addr is unspecified address, it means that the network topology has changed and
    // the rsuStatusTimer_ should be cancelled.
    if (addr.isUnspecified())
    {
        if (rsuStatusTimer_->isScheduled())
        {
            cancelEvent(rsuStatusTimer_);
            EV_INFO << "NodeInfo: setGlobalSchedulerAddr - cancelled the rsuStatusTimer due to network topology change\n";
        }
        globalSchedulerAddr_ = addr;
        EV_INFO << "NodeInfo: setGlobalSchedulerAddr - the network topology has changed, terminate all services and release resources\n";
        releaseNicResources();
        releaseServerResources();
        return;
    }

    // otherwise, the new global scheduler is just been determined
    // then update rsu status and buffered service request in this node to the new global scheduler immediately
    // then start the rsuStatusTimer_ to periodically update the new global scheduler
    globalSchedulerAddr_ = addr;
    EV_INFO << "NodeInfo: setGlobalSchedulerAddr - the new global scheduler address is " << globalSchedulerAddr_ << "\n";
    recoverRsuStatus();
    recoverServiceRequests();

    if (rsuStatusTimer_->isScheduled())
    {
        cancelEvent(rsuStatusTimer_);
        EV_INFO << "NodeInfo: setGlobalSchedulerAddr - cancelled the rsuStatusTimer to reset the timer\n";
    }
    double timeNow = int(simTime().dbl() * 1000) / 1000.0;
    double nextUpdateTime = timeNow + scheduleInterval_;    // an offset of appStopInterval_ is already maintained in Scheduler
    scheduleAt(nextUpdateTime, rsuStatusTimer_);
    EV_INFO << "NodeInfo: setGlobalSchedulerAddr - scheduled the rsuStatusTimer at " 
        << nextUpdateTime << "\n";
}


void NodeInfo::releaseNicResources()
{
    if (gnbMac_ != nullptr)
    {
        gnbMac_->mecTerminateAllGrant();
    }
}


void NodeInfo::releaseServerResources()
{
    if (server_ != nullptr)
    {
        server_->releaseServerResources();
    }
}


void NodeInfo::recoverRsuStatus()
{
    if (gnbMac_ != nullptr)
    {
        gnbMac_->mecRecoverRsuStatus();
    }
}


void NodeInfo::recoverServiceRequests()
{
    if (npc_ != nullptr)
    {
        npc_->recoverServiceRequests();
    }
}

