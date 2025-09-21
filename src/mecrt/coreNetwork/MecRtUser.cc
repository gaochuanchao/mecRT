//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecRtUser.cc / MecRtUser.h
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

#include "mecrt/coreNetwork/MecRtUser.h"
#include <inet/common/packet/printer/PacketPrinter.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include "mecrt/packets/apps/Grant2Veh.h"

Define_Module(MecRtUser);

using namespace omnetpp;
using namespace inet;

void MecRtUser::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    // wait until all the IP addresses are configured
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;
    localPort_ = getAncestorPar("rtUserPort");

    // transport layer access
    socket_.setOutputGate(gate("socketOut"));
    socket_.bind(localPort_);

    ie_ = detectInterface();
}

NetworkInterface* MecRtUser::detectInterface()
{
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    const char *interfaceName = getAncestorPar("cellularNicName");
    NetworkInterface *ie = nullptr;

    if (strlen(interfaceName) > 0) {
        ie = ift->findInterfaceByName(interfaceName);
        if (ie == nullptr)
            throw cRuntimeError("Interface \"%s\" does not exist", interfaceName);
    }

    return ie;
}


void MecRtUser::handleMessage(cMessage *msg)
{
    if(strcmp(msg->getArrivalGate()->getFullName(),"socketIn")==0)
    {
        EV << "MecRtUser::handleMessage - message from udp layer, route this message to UE" << endl;
        Packet *packet = check_and_cast<Packet *>(msg);
        PacketPrinter printer; // turns packets into human readable strings
        printer.printPacket(EV, packet); // print to standard output

        handleFromUdp(packet);
    }
}


void MecRtUser::handleFromUdp(Packet * pkt)
{
    /*
     * when we get here, it means that the packet needs to be routed to the NIC module or the UE
     */

    if (strcmp(pkt->getName(), "VehGrant") == 0)
    {
        // reached the offloading gnb, route it to the NIC module
        EV << NOW << "MecRtUser::handleFromUdp - It is a " << pkt->getName() << " packet for NIC, sending it to cellularNic" << endl;
        pkt->trim();
        pkt->clearTags();

        // add Interface-Request for cellular NIC
        if (ie_ != nullptr)
        {
            pkt->addTagIfAbsent<InterfaceReq>()->setInterfaceId(ie_->getInterfaceId());
            send(pkt,"pppGate");
        }
        else
        {
            throw cRuntimeError("MecRtUser::handleFromUdp - cannot find the cellular NIC interface");
        }
    }
}
