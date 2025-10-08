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
    localSchedulerSocketId_ = -1;

    gnbMac_ = nullptr;
    npc_ = nullptr;
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
        WATCH(localSchedulerSocketId_);

        if (enableInitDebug_)
            std::cout << "NodeInfo:initialize - stage: INITSTAGE_LOCAL - ends\n";
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

}

