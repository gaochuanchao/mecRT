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
#include "mecrt/routing/MecOspf.h"
#include "mecrt/common/Database.h"

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
    scheduler_ = nullptr;
    ospf_ = nullptr;

    rsuStatusTimer_ = nullptr;
    nodeDownTimer_ = nullptr;
    nodeUpTimer_ = nullptr;
    ifDownTimer_ = nullptr;
    ifUpTimer_ = nullptr;

    // ifFailProb_ = 0.0; // default 0.0, no interface failure
    // nodeFailProb_ = 0.0; // default 0.0, no node failure
    ifFailTime_ = 0; // default 0 seconds, the interface will be down for 0 seconds
    ifRecoverTime_ = 0; // default 0 seconds, the interface will be up for 0 seconds
    nodeFailTime_ = 0; // default 0 seconds, the node will be down for 0 seconds
    nodeRecoverTime_ = 0; // default 0 seconds, the node will be up for 0 seconds
    // failedIfId_ = -1; // no interface is down by default
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

    if (ifUpTimer_)
    {
        cancelAndDelete(ifUpTimer_);
        ifUpTimer_ = nullptr;
    }

    if (nodeUpTimer_)
    {
        cancelAndDelete(nodeUpTimer_);
        nodeUpTimer_ = nullptr;
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
        // ifFailTime_ = par("ifFailTime").doubleValue();
        // nodeFailTime_ = par("nodeFailTime").doubleValue();
        // ifRecoverTime_ = par("ifRecoverTime").doubleValue();
        // nodeRecoverTime_ = par("nodeRecoverTime").doubleValue();

        rsuStatusTimer_ = new cMessage("rsuStatusTimer");

        // initialize the timers for iterface and node failure
        ifDownTimer_ = new cMessage("ifDownTimer");
        ifUpTimer_ = new cMessage("ifUpTimer");
        nodeDownTimer_ = new cMessage("nodeDownTimer");
        nodeUpTimer_ = new cMessage("nodeUpTimer");

        linkStateChangedSignal = registerSignal("linkStateChanged");

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
        WATCH(ifRecoverTime_);
        WATCH(nodeFailTime_);
        WATCH(nodeRecoverTime_);
        WATCH_VECTOR(failedIfIds_);

        if (enableInitDebug_)
            std::cout << "NodeInfo:initialize - stage: INITSTAGE_LOCAL - ends\n";
    }
    else if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT)
    {
        if (enableInitDebug_)
            std::cout << "NodeInfo:initialize - stage: INITSTAGE_PHYSICAL_ENVIRONMENT - begins\n";

        EV_INFO << "NodeInfo:initialize - stage: INITSTAGE_PHYSICAL_ENVIRONMENT\n";

        if (nodeType_ == "GNODEB")
        {
            Database* database = check_and_cast<Database*>(getSimulation()->getModuleByPath("database"));
            int indexId = getParentModule()->getIndex();
            database->registerGnbNodeInfo(indexId, this);
            routeUpdate_ = database->getRouteUpdate();

            WATCH(routeUpdate_);
        }
        
        if (enableInitDebug_)
            std::cout << "NodeInfo:initialize - stage: INITSTAGE_PHYSICAL_ENVIRONMENT - ends\n";
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

            if (ifRecoverTime_ > ifFailTime_)
            {
                // schedule the ifUpTimer_
                if (!ifUpTimer_)
                    ifUpTimer_ = new cMessage("ifUpTimer");
                
                if (ifUpTimer_->isScheduled())
                    cancelEvent(ifUpTimer_); // cancel any previous schedule
                
                // schedule the ifUpTimer_ to recover the interface
                scheduleAt(ifRecoverTime_, ifUpTimer_);
                EV << "NodeInfo:handleMessage - schedule interface recovery at " << ifRecoverTime_ << "\n";
            }
        }
        else if (msg == nodeDownTimer_)
        {
            
            if (!routeUpdate_ && isGlobalScheduler_)
            {
                EV << "NodeInfo:handleMessage - nodeDownTimer is triggered!\n";
                EV << "\trouteUpdate is false and this node is the global scheduler, skip node failure\n";
                return;
            }

            handleNodeDownTimer();

            emit(linkStateChangedSignal, simTime().dbl());

            if (nodeRecoverTime_ > nodeFailTime_)
            {
                // schedule the nodeUpTimer_
                if (!nodeUpTimer_)
                    nodeUpTimer_ = new cMessage("nodeUpTimer");
                
                if (nodeUpTimer_->isScheduled())
                    cancelEvent(nodeUpTimer_); // cancel any previous schedule
                
                // schedule the nodeUpTimer_ to recover the node
                scheduleAt(nodeRecoverTime_, nodeUpTimer_);
                EV << "NodeInfo:handleMessage - schedule node recovery at " << nodeRecoverTime_ << "\n";
            }
        }
        else if (msg == ifUpTimer_)
        {
            handleIfUpTimer();

            emit(linkStateChangedSignal, simTime().dbl());
        }
        else if (msg == nodeUpTimer_)
        {
            handleNodeUpTimer();

            emit(linkStateChangedSignal, simTime().dbl());
        }
        else
        {
            EV << "NodeInfo:handleMessage - received an unknown self-message: " << msg->getName() << "\n";
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


void NodeInfo::recoverFromErrors()
{
    Enter_Method_Silent("NodeInfo::recoverFromErrors");
    if (ospf_)
        ospf_->recoverFromErrors();
}


void NodeInfo::injectNodeError(double failTime, double recoverTime)
{
    Enter_Method_Silent("NodeInfo::injectNodeError");
    
    EV << "NodeInfo:injectNodeError - injecting node error\n";

    // schedule the nodeDownTimer_
    if (nodeDownTimer_ && nodeDownTimer_->isScheduled())
        cancelEvent(nodeDownTimer_);

    nodeFailTime_ = failTime;
    nodeRecoverTime_ = recoverTime;

    scheduleAt(failTime, nodeDownTimer_);
    EV << "NodeInfo:injectNodeError - scheduled nodeDownTimer at " << simTime() + failTime << "\n";
}


void NodeInfo::injectLinkError(int numFailedLinks, double failedTime, double recoverTime)
{
    Enter_Method_Silent("NodeInfo::injectLinkError");
    
    EV << "NodeInfo:injectLinkError - injecting link error with " << numFailedLinks << " links to fail\n";
    // sample the failed links
    if (numFailedLinks <= 0)
    {
        EV << "NodeInfo:injectLinkError - no links to fail, returning\n";
        return;
    }

    failedIfIds_.clear(); // clear the previous failed interface ids
    // collect all the alive wired interfaces
    int numIfaces = ift_ ? ift_->getNumInterfaces() : 0;
    set<int> aliveIfIds;
    for (int i = 0; i < numIfaces; i++)
    {
        NetworkInterface *ie = ift_->getInterface(i);
        if (!ie || ie->isLoopback() || !ie->isUp() || ie->isWireless()) // skip loopback/down/wireless interfaces
            continue;
        aliveIfIds.insert(ie->getInterfaceId());
    }

    if (aliveIfIds.empty())
    {
        EV << "NodeInfo:injectLinkError - no alive wired interfaces found, cannot inject link error\n";
        return;
    }

    for (int i = 0; i < numFailedLinks; ++i)
    {
        if (aliveIfIds.empty())
        {
            EV << "NodeInfo:injectLinkError - no more alive interfaces to fail, stopping link error injection\n";
            break;
        }

        // randomly select an alive interface to fail
        auto it = aliveIfIds.begin();
        std::advance(it, intuniform(0, aliveIfIds.size() - 1));
        int failedIfId = *it;
        failedIfIds_.push_back(failedIfId);
        aliveIfIds.erase(it);

        EV << "NodeInfo:injectLinkError - selected interface id " << failedIfId << " to fail\n";
    }
    
    ifFailTime_ = failedTime;
    ifRecoverTime_ = recoverTime;

    scheduleAt(failedTime, ifDownTimer_);
    EV << "NodeInfo:injectLinkError - scheduled ifDownTimer at " << simTime() + failedTime << "\n";
}


void NodeInfo::handleNodeUpTimer()
{
    EV << "NodeInfo:handleNodeUpTimer - nodeUpTimer is triggered!\n";

    // =========== handle self ==============
    EV << "NodeInfo:handleNodeUpTimer - set node state to active\n";
    nodeState_ = true; // set the node state to active

    // =========== handle scheduler/NIC/server/npc ===========
    // nothing special for scheduler/npc/server upon node recovery
    EV << "NodeInfo:handleNodeUpTimer - enable NIC module\n";    
    if (gnbMac_)
        gnbMac_->enableNic();

    // =========== handle links connecting other nodes ==============
    EV << "NodeInfo:handleNodeUpTimer - enable links connecting to this node\n";
    if (ift_)
    {
        int numIfaces = ift_->getNumInterfaces();
        for (int i = 0; i < numIfaces; i++)
        {
            NetworkInterface *ie = ift_->getInterface(i);
            if (!ie || ie->isLoopback() || ie->isWireless()) // skip loopback/wireless interfaces
                continue;

            cDatarateChannel *dataChannel = check_and_cast_nullable<cDatarateChannel*>(ie->getRxTransmissionChannel());
            if (dataChannel)
            {
                dataChannel->setDisabled(false); // enable the channel to simulate interface recovery
                EV << "NodeInfo:handleNodeUpTimer - recover link connected to interface " << i << "\n";
            }
        }
    }
}


void NodeInfo::handleIfUpTimer()
{
    EV << "NodeInfo:handleIfUpTimer - ifUpTimer is triggered, recover the failed interface\n";

    // up the interface that is down in failedIfIds_

    if (failedIfIds_.empty())
    {
        EV << "NodeInfo:handleIfUpTimer - no failed interface ids, nothing to recover\n";
        return;
    }

    // recover the failed interfaces
    bool linkRecovered = false;
    for (int ifId : failedIfIds_)
    {
        NetworkInterface *ie = ift_->getInterfaceById(ifId);
        if (ie)
        {            
            cDatarateChannel *dataChannel = check_and_cast_nullable<cDatarateChannel*>(ie->getRxTransmissionChannel());
            if (dataChannel)
            {                
                dataChannel->setDisabled(false); // enable the channel to simulate interface recovery
                EV << "NodeInfo:handleIfUpTimer - recover link connected to interface " << ifId << "\n";
                linkRecovered = true;
            }
        }
    }

    failedIfIds_.clear(); // clear the failed interface ids after recovery

    if (linkRecovered)
    {
        emit(linkStateChangedSignal, simTime().dbl());
    }
}


void NodeInfo::handleNodeDownTimer()
{
    EV << "NodeInfo:handleNodeDownTimer - nodeDownTimer is triggered\n";

    // =========== handle self ==============
    EV << "NodeInfo:handleNodeDownTimer - set node state to inactive\n";
    nodeState_ = false; // set the node state to inactive
    if (ifDownTimer_ && ifDownTimer_->isScheduled() && ifFailTime_ <= nodeRecoverTime_)
        cancelEvent(ifDownTimer_);

    if (ifUpTimer_ && ifUpTimer_->isScheduled() && ifRecoverTime_ <= nodeRecoverTime_)
        cancelEvent(ifUpTimer_);

    // =========== handle scheduler/NIC/server/OSPF ===========
    EV << "NodeInfo:handleNodeDownTimer - reset local scheduler/NIC/server/OSPF status\n";
    setGlobalSchedulerAddr(inet::Ipv4Address::UNSPECIFIED_ADDRESS);     // this will also reset NIC and server resources
    if (gnbMac_)
        gnbMac_->disableNic();
    if (ospf_)
        ospf_->handleNodeFailure();

    // =========== handle links connecting other nodes ==============
    EV << "NodeInfo:handleNodeDownTimer - disable all links connected to this node\n";
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
}


void NodeInfo::handleIfDownTimer()
{
    EV << "NodeInfo:handleIfDownTimer - ifDownTimer is triggered, randomly select an active interface to fail\n";

    if (!ift_)
    {
        EV << "NodeInfo:handleIfDownTimer - IInterfaceTable is not set, cannot fail any interface\n";
        return;
    }

    if (failedIfIds_.empty())
    {
        EV << "NodeInfo:handleIfDownTimer - no failed interface ids, cannot fail any interface\n";
        return;
    }

    // fail the interface in failedIfIds_
    bool linkFailed = false;
    for (int ifId : failedIfIds_)
    {
        NetworkInterface *ie = ift_->getInterfaceById(ifId);
        if (ie)
        {
            cDatarateChannel *dataChannel = check_and_cast_nullable<cDatarateChannel*>(ie->getRxTransmissionChannel());
            if (dataChannel)
            {
                dataChannel->setDisabled(true); // disable the channel to simulate interface failure
                EV << "NodeInfo:handleIfDownTimer - disable link connected to interface " << ifId << "\n";
                linkFailed = true;
            }
        }
    }

    if (linkFailed)
    {
        emit(linkStateChangedSignal, simTime().dbl());
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

        // TODO: adjust multiple global schedulers synchronization mechanism to avoid this check
        // if (simTime() < nextUpdateTime)
        // {
        //     EV << "NodeInfo:handleNodeStatusTimer - the time is too early to send RSU status update, reschedule rsuStatusTimer at "
        //         << nextUpdateTime << "\n";
        //     scheduleAt(nextUpdateTime, rsuStatusTimer_);
        //     return;
        // }

        if (gnbMac_)
            gnbMac_->mecRecoverRsuStatus();

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

        if (isGlobalScheduler_)
        {
            EV_INFO << "NodeInfo: setGlobalSchedulerAddr - this node was the global scheduler, resetting it\n";
            isGlobalScheduler_ = false;
            if (scheduler_)
                scheduler_->globalSchedulerReset();
            
            if (hasGUI())
                resetGlobalSchedulerIcon();
        }

        globalSchedulerAddr_ = addr;
        EV_INFO << "NodeInfo: setGlobalSchedulerAddr - the network topology has changed, terminate all services and release resources\n";
        if (gnbMac_)
            gnbMac_->mecTerminateAllGrant();
        
        if (server_)
            server_->releaseServerResources();
        
        return;
    }

    // otherwise, the new global scheduler is just been determined
    // then update rsu status and buffered service request in this node to the new global scheduler immediately
    // then start the rsuStatusTimer_ to periodically update the new global scheduler
    globalSchedulerAddr_ = addr;
    EV_INFO << "NodeInfo: setGlobalSchedulerAddr - the new global scheduler address is " << globalSchedulerAddr_ << "\n";

    if (globalSchedulerAddr_ == nodeAddr_)
    {
        isGlobalScheduler_ = true;
        if (scheduler_)
            scheduler_->globalSchedulerInit();

        if (hasGUI())
            enableGlobalSchedulerIcon();
    }

    if (gnbMac_)
        gnbMac_->mecRecoverRsuStatus();

    if (npc_)
        npc_->recoverServiceRequests();

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


void NodeInfo::updateAdjListToScheduler(map<MacNodeId, map<MacNodeId, double>>& adjList)
{
    if (scheduler_ && isGlobalScheduler_)
    {
        scheduler_->resetNetTopology(adjList);
    }
}


void NodeInfo::enableGlobalSchedulerIcon()
{
    /**
     *  t[0]-text Additionaltexttodisplay
     *  t[1]-textposition Positionofthetext.Values: left(l),right(r),top(t).Default: t
     *  t[2]-textcolor Color of the displayed text (color name, #RRGGBBor@HHSSBB).Default:blue
     */
    auto& ds = getParentModule()->getDisplayString();
    ds.setTagArg("t", 0, "GLOBAL\nSCHEDULER");
    ds.setTagArg("t", 2, "blue");
}


void NodeInfo::resetGlobalSchedulerIcon()
{
    auto& ds = getParentModule()->getDisplayString();
    ds.setTagArg("t", 0, "");
}
