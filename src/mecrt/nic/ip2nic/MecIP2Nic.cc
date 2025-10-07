//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecIP2Nic.cc / MecIP2Nic.h
//
//  Description:
//    This file implements the IP to NIC interface. It extends the Simu5G IP2Nic module
//    to ensure data can be tranferred from the NIC module of an ES to its server module.
//    The original IP2Nic module in Simu5G does not allow data to be transferred to an unit
//    within the 5G core network.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//


#include "mecrt/nic/ip2nic/MecIP2Nic.h"

#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/linklayer/common/InterfaceTag_m.h>
#include <inet/common/socket/SocketTag_m.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include "inet/networklayer/ipv4/Ipv4RoutingTable.h"

using namespace std;
using namespace inet;
using namespace omnetpp;

Define_Module(MecIP2Nic);


MecIP2Nic::MecIP2Nic()
{
    nodeInfo_ = nullptr;
}


void MecIP2Nic::initialize(int stage)
{
    IP2Nic::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {
        if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
        if (enableInitDebug_)
            std::cout << "MecIP2Nic::initialize - stage: INITSTAGE_LOCAL - begins" << std::endl;
            
        nodeType_ = aToNodeType(par("nodeType").stdstringValue());

        if (enableInitDebug_)
            std::cout << "MecIP2Nic::initialize - nodeType_: " << nodeType_ << std::endl;
    }
    else if (stage == INITSTAGE_PHYSICAL_ENVIRONMENT)
    {
        if (enableInitDebug_)
            std::cout << "MecIP2Nic::initialize - stage: INITSTAGE_PHYSICAL_ENVIRONMENT - begins" << std::endl;
        
        // get node info module
        try {
            nodeInfo_ = getModuleFromPar<NodeInfo>(par("nodeInfoModulePath"), this);
        } catch (cException &e) {
            throw cRuntimeError("MecIP2Nic:initialize - cannot find nodeInfo module\n");
        }
        registerInterface();

        if (enableInitDebug_)
            std::cout << "MecIP2Nic::initialize - registerInterface() done." << std::endl;
    }
}

void MecIP2Nic::handleMessage(cMessage *msg)
{
    if(nodeType_ == ENODEB || nodeType_ == GNODEB){
        if (msg->getArrivalGate()->isName("stackNic$i"))    // message from stack (phy->mac->rrc->pdcprrc->ip2nic->npc)
        {
            // check if the destination Ipv4 address is the gNB address
            auto pkt = check_and_cast<Packet *>(msg);
            auto ipHeader = pkt->removeAtFront<Ipv4Header>();
            auto destAddress = ipHeader->getDestAddress();

            if (destAddress == MEC_UE_OFFLOAD_ADDR || destAddress == nodeInfo_->getNodeAddr())   // offloading packet from UE to the MEC server
            {
                EV << "MecIP2Nic::handleMessage - the destination is the current gNB, send to ipv4 module." << endl;
                // set the ipv4 address of current gNB as the destination address
                ipHeader->setDestAddress(nodeInfo_->getNodeAddr());
                
            }
            else    // IP2Nic::handleMessage - message from stack: send to peer
            {
                EV << "MecIP2Nic::handleMessage - the destination is not the current gNB, route to dest IP " << destAddress << endl;
            }

            auto networkProtocolInd = pkt->addTagIfAbsent<NetworkProtocolInd>();
            networkProtocolInd->setProtocol(&Protocol::ipv4);
            networkProtocolInd->setNetworkProtocolHeader(ipHeader);
            pkt->insertAtFront(ipHeader);
            removeAllSimu5GTags(pkt);

            prepareForIpv4(pkt);
            EV << "MecIP2Nic::handleMessage - message from stack: send to IP layer" << endl;
            send(pkt,ipGateOut_);
        }
        else if (msg->getArrivalGate()->isName("upperLayerIn"))   // message from transport layer: send to stack
        {
            EV << "MecIP2Nic::handleMessage - Packet " << msg->getName() << " from IP layer." << endl;
            auto ipDatagram = check_and_cast<Packet *>(msg);

            fromIpBs(ipDatagram);
        }
        else
        {
            // error: drop message
            EV << "IP2Nic::handleMessage - (E/GNODEB): Wrong gate " << msg->getArrivalGate()->getName() << endl;
            delete msg;
        }
    }
    else if( nodeType_ == UE )
    {
        // message from transport: send to stack
        if (msg->getArrivalGate()->isName("upperLayerIn"))
        {

            auto ipDatagram = check_and_cast<Packet *>(msg);
            EV << "LteIp: message from transport: send to stack" << endl;
            fromIpUe(ipDatagram);
        }
        else if(msg->getArrivalGate()->isName("stackNic$i"))
        {
            // message from stack: send to transport
            EV << "LteIp: message from stack: send to transport" << endl;
            auto pkt = check_and_cast<Packet *>(msg);
            pkt->removeTagIfPresent<SocketInd>();
    		removeAllSimu5GTags(pkt);
    		toIpUe(pkt);
        }
        else
        {
            // error: drop message
            EV << "IP2Nic (UE): Wrong gate " << msg->getArrivalGate()->getName() << endl;
            delete msg;
        }
    }
}

void MecIP2Nic::registerInterface()
{
    networkIf = getContainingNicModule(this);
    networkIf->setInterfaceName(par("interfaceName").stdstringValue().c_str());
    // TODO configure MTE size from NED
    networkIf->setMtu(par("mtu"));

    if (nodeType_ == UE)
    {
        // get the routing table, add a default route (0.0.0.0/0) to point towards this gateway.
        auto rt = check_and_cast<Ipv4RoutingTable *>(getModuleByPath("^.^.ipv4.routingTable"));
        Ipv4Route *route = new Ipv4Route();
        route->setDestination(MEC_UE_OFFLOAD_ADDR); // default route
        route->setNetmask(Ipv4Address::ALLONES_ADDRESS);
        route->setInterface(networkIf);
        route->setSourceType(Ipv4Route::MANUAL);
        route->setMetric(1);
        rt->addRoute(route);
    }


    if (nodeInfo_)
        nodeInfo_->setNicInterfaceId(networkIf->getInterfaceId());
}

