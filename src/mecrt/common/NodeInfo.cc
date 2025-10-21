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
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "mecrt/apps/scheduler/Scheduler.h"

Define_Module(NodeInfo);


NodeInfo::NodeInfo()
{
    enableInitDebug_ = false;

    nodeType_ = "";
    nodeState_ = true;

    nodeAddr_ = inet::Ipv4Address::UNSPECIFIED_ADDRESS;
    // rtState_ = false;
    npcSocketId_ = -1;

    nodeId_ = -1;
    nicInterfaceId_ = -1;
    // nicState_ = true;

    // serverState_ = true;
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
    ift_ = nullptr;

    rsuStatusTimer_ = nullptr;
    nodeDownTimer_ = nullptr;
    ifDownTimer_ = nullptr;

    // ifFailProb_ = 0.0; // default 0.0, no interface failure
    // nodeFailProb_ = 0.0; // default 0.0, no node failure
    ifFailTime_ = 0; // default 0 seconds, the interface will be down for 0 seconds
    nodeFailTime_ = 0; // default 0 seconds, the node will be down for 0 seconds
    failedIfId_ = -1; // no interface is down by default
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

    if (nodeDownTimer_)
    {
        cancelAndDelete(nodeDownTimer_);
        nodeDownTimer_ = nullptr;
    }

    if (ifDownTimer_)
    {
        cancelAndDelete(ifDownTimer_);
        ifDownTimer_ = nullptr;
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
        ifFailTime_ = par("ifFailTime").doubleValue();
        nodeFailTime_ = par("nodeFailTime").doubleValue();

        rsuStatusTimer_ = new cMessage("rsuStatusTimer");
        nodeDownTimer_ = new cMessage("nodeDownTimer");
        ifDownTimer_ = new cMessage("ifDownTimer");

        // initialize the timers for iterface and node failure
        if (ifFailTime_ > 0)
        {
            scheduleAt(ifFailTime_, ifDownTimer_);
            EV_INFO << "NodeInfo:initialize - scheduled ifDownTimer at " << simTime() + ifFailTime_ << "\n";
        }

        if (nodeFailTime_ > 0)
        {
            scheduleAt(nodeFailTime_, nodeDownTimer_);
            EV_INFO << "NodeInfo:initialize - scheduled nodeDownTimer at " << simTime() + nodeFailTime_ << "\n";
        }

        WATCH(nodeType_);
        WATCH(nodeState_);
        WATCH(nodeAddr_);
        // WATCH(rtState_);
        WATCH(npcSocketId_);
        WATCH(nodeId_);
        WATCH(nicInterfaceId_);
        // WATCH(nicState_);
        // WATCH(serverState_);
        WATCH(serverPort_);
        WATCH(serverSocketId_);
        WATCH(isGlobalScheduler_);
        WATCH(globalSchedulerAddr_);
        WATCH(localSchedulerPort_);
        WATCH(scheduleInterval_);
        WATCH(appStopInterval_);
        WATCH(localSchedulerSocketId_);

        WATCH(ifFailTime_);
        WATCH(nodeFailTime_);
        WATCH(failedIfId_);

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
            handleNodeStatusTimer();
        }
        else if (msg == ifDownTimer_)
        {
            handleIfDownTimer();
        }
        else if (msg == nodeDownTimer_)
        {
            handleNodeDownTimer();
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


void NodeInfo::handleNodeDownTimer()
{
    EV << "NodeInfo:handleNodeDownTimer - nodeDownTimer is triggered, set node state to inactive\n";
    nodeState_ = false; // set the node state to inactive

    if (ifDownTimer_->isScheduled())
        cancelEvent(ifDownTimer_);

    // release all resources held by the NIC and server modules
    setGlobalSchedulerAddr(inet::Ipv4Address::UNSPECIFIED_ADDRESS);
    // reset scheduler is needed
    if (isGlobalScheduler_ && scheduler_)
        scheduler_->globalSchedulerReset();

    // disable all interfaces
    if (ift_)
    {
        int numIfaces = ift_->getNumInterfaces();
        for (int i = 0; i < numIfaces; i++)
        {
            NetworkInterface *ie = ift_->getInterface(i);
            if (!ie || ie->isLoopback() || !ie->isUp() || ie->isWireless()) // skip loopback/down/wireless interfaces
                continue;

            cDatarateChannel *dataChannel = check_and_cast_nullable<cDatarateChannel*>(ie->getRxTransmissionChannel());
            if (dataChannel)
            {
                dataChannel->setDisabled(true); // disable the channel to simulate interface failure
                EV << "NodeInfo:handleNodeDownTimer - disable link connected to interface " << i << "\n";
            }
        }
    }

    // disable gnbMac on data receiving

}



void NodeInfo::handleIfDownTimer()
{
    EV << "NodeInfo:handleIfDownTimer - ifDownTimer is triggered, randomly select an active interface to fail\n";

    if (!ift_)
    {
        EV << "NodeInfo:handleIfDownTimer - IInterfaceTable is not set, cannot fail any interface\n";
        return;
    }

    // when the ifDownTimer_ is triggered, randomly select an active wired interface to fail
    int numIfaces = ift_->getNumInterfaces();
    std::vector<int> activeIfIds;
    for (int i = 0; i < numIfaces; i++)
    {
        NetworkInterface *ie = ift_->getInterface(i);
        if (!ie || ie->isLoopback() || !ie->isUp() || ie->isWireless()) // skip loopback/down/wireless interfaces
            continue;
            
        activeIfIds.push_back(i);
    }

    if (!activeIfIds.empty())
    {
        failedIfId_ = activeIfIds[rand() % activeIfIds.size()];
        NetworkInterface *ie = ift_->getInterface(failedIfId_);
        if (ie)
        {
            // ie->setState(NetworkInterface::DOWN);
            // ie->setCarrier(false);
            cDatarateChannel *dataChannel = check_and_cast_nullable<cDatarateChannel*>(ie->getRxTransmissionChannel());
            if (dataChannel)
            {
                dataChannel->setDisabled(true); // disable the channel to simulate interface failure
                EV << "NodeInfo:handleIfDownTimer - disable link connected to interface " << failedIfId_ << "\n";
            }
        }
    }
    else
    {
        EV << "NodeInfo:handleIfDownTimer - no active interface to fail" << "\n";
    }
}


void NodeInfo::handleNodeStatusTimer()
{
    EV << "NodeInfo:handleNodeStatusTimer - rsuStatusTimer is triggered, update RSU status to the global scheduler\n";
    // send RSU status to the global scheduler
    if (!globalSchedulerAddr_.isUnspecified())
    {
        // check if the time is too early (the NEXT_SCHEDULING_TIME might be updated by other global scheduler)
        double nextUpdateTime = NEXT_SCHEDULING_TIME - appStopInterval_;
        if (simTime() < nextUpdateTime)
        {
            EV << "NodeInfo:handleNodeStatusTimer - the time is too early to send RSU status update, reschedule rsuStatusTimer at "
                << nextUpdateTime << "\n";
            scheduleAt(nextUpdateTime, rsuStatusTimer_);
            return;
        }

        recoverRsuStatus();
        scheduleAt(nextUpdateTime + scheduleInterval_, rsuStatusTimer_);
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

