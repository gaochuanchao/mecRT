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
#include "mecrt/packets/apps/Grant2Veh.h"

Define_Module(NodePacketController);

using namespace omnetpp;
using namespace inet;

NodePacketController::NodePacketController()
{
    localPort_ = -1;
    nodeInfo_ = nullptr;
    enableInitDebug_ = false;
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

        localPort_ = par("localPort");


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
            nodeInfo_ = getModuleFromPar<NodeInfo>(getAncestorPar("nodeInfoModulePath"), this);
        } catch (cException &e) {
            std::cerr << "NodePacketController:initialize - cannot find nodeInfo module\n";
            nodeInfo_ = nullptr;
        }

        if (nodeInfo_)
            nodeInfo_->setNpcPort(localPort_);

        WATCH_PTR(nodeInfo_);
        WATCH(localPort_);

        if (enableInitDebug_)
            std::cout << "NodePacketController::initialize - INITSTAGE_NETWORK_LAYER end" << endl;
    }
}


void NodePacketController::handleMessage(cMessage *msg)
{
    if(strcmp(msg->getArrivalGate()->getFullName(),"socketIn")==0)
    {
        EV << "NodePacketController::handleMessage - message from udp layer, route this message to UE" << endl;
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

    if (strcmp(pkt->getName(), "VehGrant") == 0)
    {
        // reached the offloading gnb, route it to the NIC module
        EV << NOW << "NodePacketController::handleFromUdp - It is a " << pkt->getName() << " packet for NIC, sending it to cellularNic" << endl;
        pkt->trim();
        pkt->clearTags();

        // add Interface-Request for cellular NIC
        if (nodeInfo_ != nullptr)
        {
            pkt->addTagIfAbsent<InterfaceReq>()->setInterfaceId(nodeInfo_->getNicInterfaceId());
            send(pkt,"pppGate");
        }
        else
        {
            throw cRuntimeError("NodePacketController::handleFromUdp - cannot find the cellular NIC interface");
        }
    }
}
