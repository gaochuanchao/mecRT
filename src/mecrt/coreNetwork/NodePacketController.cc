//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    NodePacketController.cc / NodePacketController.h
//
//  Description:
//    This file implements a simple module that routes packets from the gnb to the user device.
//    When data is routing within the backhaul network, the data is first sent to this module of a gnb, then
//    it is forwarded to the user device by this module.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/coreNetwork/NodePacketController.h"
#include <inet/common/packet/printer/PacketPrinter.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include "inet/networklayer/common/L3AddressTag_m.h"
#include <inet/common/TimeTag_m.h>

#include "mecrt/packets/apps/Grant2Veh.h"


Define_Module(NodePacketController);

using namespace omnetpp;
using namespace inet;

NodePacketController::NodePacketController()
{
    localPort_ = -1;
    nodeInfo_ = nullptr;
    enableInitDebug_ = false;
    checkGlobalSchedulerTimer_ = nullptr;

    pendingSrvReqs_.clear();
    srvReqsBuffer_.clear();
}

NodePacketController::~NodePacketController()
{
    if (enableInitDebug_)
        std::cout << "NodePacketController::~NodePacketController - destroying NodePacketController module\n";

    if (checkGlobalSchedulerTimer_)
    {
        cancelAndDelete(checkGlobalSchedulerTimer_);
        checkGlobalSchedulerTimer_ = nullptr;
    }

    if (enableInitDebug_)
        std::cout << "NodePacketController::~NodePacketController - destroying NodePacketController module done!\n";
}


void NodePacketController::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {
        EV << "NodePacketController::initialize - local init stage" << endl;

        if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
        if (enableInitDebug_)
            std::cout << "NodePacketController::initialize - INITSTAGE_LOCAL begin" << endl;

        localPort_ = MEC_NPC_PORT; // default(37);
        checkGlobalSchedulerInterval_ = par("checkGlobalSchedulerInterval").doubleValue();

        checkGlobalSchedulerTimer_ = new cMessage("checkGlobalSchedulerTimer");

        if (enableInitDebug_)
            std::cout << "NodePacketController::initialize - INITSTAGE_LOCAL end" << endl;
    }
    else if (stage == inet::INITSTAGE_APPLICATION_LAYER)
    {
        if (enableInitDebug_)
            std::cout << "NodePacketController::initialize - INITSTAGE_NETWORK_LAYER begin" << endl;

        // transport layer access
        socket_.setOutputGate(gate("socketOut"));
        socket_.bind(localPort_);

        try {
            nodeInfo_ = getModuleFromPar<NodeInfo>(par("nodeInfoModulePath"), this);
        } catch (cException &e) {
            throw cRuntimeError("NodePacketController:initialize - cannot find nodeInfo module\n");
        }

        WATCH_PTR(nodeInfo_);
        WATCH(localPort_);

        if (enableInitDebug_)
            std::cout << "NodePacketController::initialize - INITSTAGE_NETWORK_LAYER end" << endl;
    }
}


void NodePacketController::handleMessage(cMessage *msg)
{
    // self message
    if (msg->isSelfMessage())
    {
        if (msg == checkGlobalSchedulerTimer_)
        {
            handleGlobalSchedulerTimer();
        }
    }
    else if(strcmp(msg->getArrivalGate()->getFullName(),"socketIn")==0)
    {
        EV << "NodePacketController::handleMessage - message from udp layer" << endl;
        Packet *packet = check_and_cast<Packet *>(msg);
        PacketPrinter printer; // turns packets into human readable strings
        printer.printPacket(EV, packet); // print to standard output

        handleFromUdp(packet);
    }
}


void NodePacketController::handleFromUdp(Packet * pkt)
{
    /*
     * when we get here, it means that the packet needs to be routed to the NIC module or the UE
     */

    if (strcmp(pkt->getName(), "SrvReq") == 0)  // service request packet from the UE
    {
        handleServiceRequest(pkt);
        delete pkt;
        pkt = nullptr;
        return;
    }
    else if (strcmp(pkt->getName(), "VehGrant") == 0)
    {
        // reached the offloading gnb, route it to the NIC module
        EV << NOW << "NodePacketController::handleFromUdp - It is a " << pkt->getName() << " packet for NIC, sending it to cellularNic" << endl;
        pkt->trim();
        pkt->clearTags();

        // add Interface-Request for cellular NIC
        pkt->addTagIfAbsent<InterfaceReq>()->setInterfaceId(nodeInfo_->getNicInterfaceId());
    }
}


void NodePacketController::handleServiceRequest(Packet *pkt)
{
    /***
     * When receive a service request packet from the UE, create two copies of the packet:
     * 1) one is sent to the global scheduler (if it exists)
     * 2) the other is sent to the local scheduler
     * if local scheduler is voted as the global scheduler, then only one copy is sent to the local scheduler
     */
    auto srvReq = pkt->peekAtFront<VecRequest>();
    AppId appId = srvReq->getAppId();
    EV << NOW << "NodePacketController::handleFromUdp - Received a service request packet for app " << appId << endl;

    Ptr<VecRequest> srvReqCopy = makeShared<VecRequest>(*srvReq);
    if (srvReq->getUeIpAddress() == 0)
    {
        EV << NOW << "NodePacketController::handleFromUdp - fill in the UE IP address in the service request packet." << endl;
        // get ipv4 address of the ue from the VecRequest packet tag
        auto ueIpAddress = pkt->getTag<L3AddressInd>()->getSrcAddress().toIpv4();
        srvReqCopy->setUeIpAddress(ueIpAddress.getInt());
        // if want to get the src port, use pkt->getTag<L4PortInd>()->getSrcPort()
    }
    srvReqsBuffer_[appId] = srvReqCopy; // buffer the request in case the global scheduler recovers later
    
    // create a copy of the service request packet for the local scheduler
    EV << NOW << "NodePacketController::handleFromUdp - send a copy of the service request packet to the local scheduler." << endl;
    Packet* packetToLocal = new Packet("SrvReq");
    packetToLocal->insertAtBack(srvReqCopy);
    socket_.sendTo(packetToLocal, Ipv4Address::LOOPBACK_ADDRESS, nodeInfo_->getLocalSchedulerPort());

    // send a copy to the global scheduler (if it exists)
    if (!nodeInfo_->getIsGlobalScheduler() && !nodeInfo_->getGlobalSchedulerAddr().isUnspecified())
    {
        EV << NOW << "NodePacketController::handleFromUdp - send a copy of the service request packet to the global scheduler." << endl;
        Packet* packetToGlobal = new Packet("SrvReq");
        packetToGlobal->insertAtBack(srvReqsBuffer_[appId]);
        socket_.sendTo(packetToGlobal, nodeInfo_->getGlobalSchedulerAddr(), MEC_NPC_PORT);
    }
    else if (nodeInfo_->getGlobalSchedulerAddr().isUnspecified())
    {
        EV << NOW << "NodePacketController::handleFromUdp - global scheduler is not ready, buffer the service request packet." << endl;
        pendingSrvReqs_.push_back(appId);
        if (!checkGlobalSchedulerTimer_->isScheduled())
            scheduleAt(simTime() + checkGlobalSchedulerInterval_, checkGlobalSchedulerTimer_);
    }
}


void NodePacketController::handleGlobalSchedulerTimer()
{
    // check if the global scheduler is ready and we are the global scheduler
    if (nodeInfo_->getIsGlobalScheduler())
    {
        EV << NOW << "NodePacketController::handleGlobalSchedulerTimer - we are the global scheduler, delete all pending service requests." << endl;
        pendingSrvReqs_.clear();
        return;
    }

    // if we are not the global scheduler, check if the global scheduler is ready
    if (!nodeInfo_->getGlobalSchedulerAddr().isUnspecified())   // global scheduler is ready
    {
        EV << NOW << "NodePacketController::handleGlobalSchedulerTimer - global scheduler is ready, send buffered service request packets to it." << endl;
        // send all buffered service request packets to the global scheduler
        for (auto appId : pendingSrvReqs_)
        {
            if (srvReqsBuffer_.find(appId) != srvReqsBuffer_.end())
            {
                Packet* packetToGlobal = new Packet("SrvReq");
                packetToGlobal->insertAtBack(srvReqsBuffer_[appId]);
                socket_.sendTo(packetToGlobal, nodeInfo_->getGlobalSchedulerAddr(), MEC_NPC_PORT);
            }
        }
        pendingSrvReqs_.clear();
        return;
    }

    // if there are still pending requests, reschedule the timer
    if (!pendingSrvReqs_.empty())
    {
        if (!checkGlobalSchedulerTimer_->isScheduled())
            scheduleAt(simTime() + checkGlobalSchedulerInterval_, checkGlobalSchedulerTimer_);
    }
}

