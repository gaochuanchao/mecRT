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

using namespace std;
using namespace inet;
using namespace omnetpp;

Define_Module(MecIP2Nic);


MecIP2Nic::MecIP2Nic()
{
    gnbAddress_ = Ipv4Address::UNSPECIFIED_ADDRESS;
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
            nodeInfo_ = getModuleFromPar<NodeInfo>(getAncestorPar("nodeInfoModulePath"), this);
        } catch (cException &e) {
            cerr << "MecIP2Nic:initialize - cannot find nodeInfo module\n";
            nodeInfo_ = nullptr;
        }
        registerInterface();

        if (enableInitDebug_)
            std::cout << "MecIP2Nic::initialize - registerInterface() done." << std::endl;
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        if (enableInitDebug_)
            std::cout << "MecIP2Nic::initialize - stage: INITSTAGE_APPLICATION_LAYER - begins" << std::endl;

        gnbAddress_ = inet::L3AddressResolver().resolve(getParentModule()->getParentModule()->getFullName());
        EV << "MecIP2Nic::initialize - local gNB IP " << gnbAddress_.toIpv4() << endl;
        // if (nodeType_ == ENODEB || nodeType_ == GNODEB)
        // {
        //     auto interfaceTable = check_and_cast<inet::IInterfaceTable *>(getParentModule()->getParentModule()->getSubmodule("interfaceTable"));
        //     pppIfInterfaceId_ = interfaceTable->findInterfaceByName("pppIf")->getInterfaceId();
        // }
        
        if (enableInitDebug_)
            std::cout << "MecIP2Nic::initialize - stage: INITSTAGE_APPLICATION_LAYER - ends." << std::endl;
    }
}

void MecIP2Nic::handleMessage(cMessage *msg)
{
    if(nodeType_ == ENODEB || nodeType_ == GNODEB){
        if (msg->getArrivalGate()->isName("stackNic$i"))
        {
            // check if the destination Ipv4 address is the gNB address
            auto pkt = check_and_cast<Packet *>(msg);
            auto ipHeader = pkt->peekAtFront<Ipv4Header>();
            auto destAddress = ipHeader->getDestAddress();

            if (destAddress == gnbAddress_.toIpv4())
            {
                EV << "MecIP2Nic::handleMessage - dest IP " << destAddress << ", the destination is the current gNB." << endl;
                EV << "MecIP2Nic::handleMessage - message from stack: send to IP layer" << endl;
                removeAllSimu5GTags(pkt);
               
                auto networkProtocolInd = pkt->addTagIfAbsent<NetworkProtocolInd>();
                networkProtocolInd->setProtocol(&Protocol::ipv4);
                networkProtocolInd->setNetworkProtocolHeader(ipHeader);
                // change protocol from LteProtocol::ipv4uu to Protocol::ipv4 such that the message will be delived to the RSU server
                pkt->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
                pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);
                // add Interface-Indication to indicate which interface this packet was received from
                pkt->addTagIfAbsent<InterfaceInd>()->setInterfaceId(networkIf->getInterfaceId());
                send(pkt,ipGateOut_);
            }
            else    // IP2Nic::handleMessage - message from stack: send to peer
            {
                EV << "MecIP2Nic::handleMessage - dest IP " << destAddress << ", the destination is not the current gNB." << endl;

                pkt->removeTagIfPresent<SocketInd>();
                removeAllSimu5GTags(pkt);
                
                // toIpBs(pkt);
                // pkt->addTagIfAbsent<InterfaceReq>()->setInterfaceId(pppIfInterfaceId_);
                // add Interface-Indication to indicate which interface this packet was received from
                pkt->addTagIfAbsent<InterfaceInd>()->setInterfaceId(networkIf->getInterfaceId());
                EV << "IP2Nic::toIpBs - message from stack: send to node " << destAddress << endl;
                send(pkt,ipGateOut_);
            }
        }
        else if (msg->getArrivalGate()->isName("upperLayerIn"))
        {
            EV << "MecIP2Nic::handleMessage - Packet " << msg->getName() << " from IP layer." << endl;
            auto ipDatagram = check_and_cast<Packet *>(msg);
            auto ipHeader = ipDatagram->peekAtFront<Ipv4Header>();
            auto destAddress = ipHeader->getDestAddress();

            if (!strcmp(ipDatagram->getName(), "VehGrant") && destAddress == gnbAddress_.toIpv4())
            {
                EV << "MecIP2Nic::handleMessage - dest IP " << destAddress << ", the destination is the current gNB, send to stack." << endl;
                // Remove control info from IP datagram
                ipDatagram->removeTagIfPresent<SocketInd>();
                removeAllSimu5GTags(ipDatagram);
                // Remove InterfaceReq Tag (we already are on an interface now)
                ipDatagram->removeTagIfPresent<InterfaceReq>();
    
                toStackBs(ipDatagram);
            }
            else
            {
                fromIpBs(ipDatagram);
            }
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

    if (nodeInfo_)
        nodeInfo_->setNicInterfaceId(networkIf->getInterfaceId());
}

