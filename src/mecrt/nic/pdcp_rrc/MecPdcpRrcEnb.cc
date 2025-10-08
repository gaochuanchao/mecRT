//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecPdcpRrcEnb.cc / MecPdcpRrcEnb.h
//
//  Description:
//    This file implements the PDCP/RRC layer for NR (New Radio) in the eNB (evolved Node B).
//    Original file: simu5g - "stack/pdcp_rrc/layer/NRPdcpRrcEnb.h"
//    We add the control logic for MEC service subscription.
//
//    ... --> LtePdcpRrcUeD2D --> NRPdcpRrcUe --> GnbPdcpRrcUe
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/nic/pdcp_rrc/MecPdcpRrcEnb.h"

#include "stack/pdcp_rrc/layer/entity/NRTxPdcpEntity.h"
#include "stack/pdcp_rrc/layer/entity/NRRxPdcpEntity.h"
#include "stack/packetFlowManager/PacketFlowManagerBase.h"
#include "stack/d2dModeSelection/D2DModeSwitchNotification_m.h"

#include "inet/transportlayer/udp/UdpHeader_m.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"

Define_Module(MecPdcpRrcEnb);

/*
 * Upper Layer handlers
 */
void MecPdcpRrcEnb::fromDataPort(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayer, pktAux);

    // Control Informations
    auto pkt = check_and_cast<inet::Packet *> (pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    setTrafficInformation(pkt, lteInfo);

    // get source info
    Ipv4Address srcAddr = Ipv4Address(lteInfo->getSrcAddr());
    // get destination info
    Ipv4Address destAddr = Ipv4Address(lteInfo->getDstAddr());
    MacNodeId srcId, destId;

    // set direction based on the destination Id. If the destination can be reached
    // using D2D, set D2D direction. Otherwise, set UL direction
    srcId = (lteInfo->getUseNR()) ? binder_->getNrMacNodeId(srcAddr) : binder_->getMacNodeId(srcAddr);
    destId = (lteInfo->getUseNR()) ? binder_->getNrMacNodeId(destAddr) : binder_->getMacNodeId(destAddr);   // get final destination
    lteInfo->setDirection(getDirection());

    // check if src and dest of the flow are D2D-capable UEs (currently in IM)
    if (getNodeTypeById(srcId) == UE && getNodeTypeById(destId) == UE && binder_->getD2DCapability(srcId, destId))
    {
        // this way, we record the ID of the endpoint even if the connection is in IM
        // this is useful for mode switching
        lteInfo->setD2dTxPeerId(srcId);
        lteInfo->setD2dRxPeerId(destId);
    }
    else
    {
        lteInfo->setD2dTxPeerId(0);
        lteInfo->setD2dRxPeerId(0);
    }

    // Cid Request
    EV << "MecPdcpRrcEnb::fromDataPort - Received CID request for Traffic [ " << "Source: " << Ipv4Address(lteInfo->getSrcAddr())
            << " Destination: " << Ipv4Address(lteInfo->getDstAddr())
            << " , ToS: " << lteInfo->getTypeOfService()
            << " , Direction: " << dirToA((Direction)lteInfo->getDirection()) << " ]\n";

    if (!strcmp(pkt->getName(), "NicGrant"))
    {
        // deliver to rlc stack
        EV << "MecPdcpRrcEnb::fromDataPort - Sending packet " << pkt->getName() << " to PDCP stack\n";
        sendToLowerLayer(pkt);
        return;
    }

    /*
     * Different lcid for different directions of the same flow are assigned.
     * RLC layer will create different RLC entities for different LCIDs
     */

    LogicalCid mylcid;
    if ((mylcid = ht_->find_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(), lteInfo->getTypeOfService(), lteInfo->getDirection())) == 0xFFFF)
    {
        // LCID not found

        // assign a new LCID to the connection
        mylcid = lcid_++;

        EV << "MecPdcpRrcEnb::fromDataPort - Connection not found, new CID created with LCID " << mylcid << "\n";

        ht_->create_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(), lteInfo->getTypeOfService(), lteInfo->getDirection(), mylcid);
    }

    // assign LCID
    lteInfo->setLcid(mylcid);

    // obtain CID
    MacCid cid = idToMacCid(destId, mylcid);

    EV << "MecPdcpRrcEnb::fromDataPort - Assigned Lcid: " << mylcid << " [CID: " << cid << "]\n";
    EV << "MecPdcpRrcEnb::fromDataPort - Assigned Node ID: " << nodeId_ << "\n";
    EV << "MecPdcpRrcEnb::fromDataPort - dest ID: " << destId << "\n";

    // get the PDCP entity for this LCID and process the packet
    LteTxPdcpEntity* entity = getTxEntity(cid);
    entity->handlePacketFromUpperLayer(pkt);
}

void MecPdcpRrcEnb::fromLowerLayer(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet*>(pktAux);
    pkt->trim();
    std::string pktName = pkt->getName();
    if(pktName == "SrvReq" || pktName == "RsuFD" || pktName == "SrvFD")
    {
        // deliver to IP layer
        EV << "MecPdcpRrcEnb::fromLowerLayer - Sending packet " << pktName << " to IP stack\n";
        toDataPort(pkt);
        return;
    }

    // if dual connectivity is enabled and this is a secondary node,
    // forward the packet to the PDCP of the master node
    MacNodeId masterId = binder_->getMasterNode(nodeId_);
    if (dualConnectivityEnabled_ && (nodeId_ != masterId) )
    {
        EV << NOW << " MecPdcpRrcEnb::fromLowerLayer - forward packet to the master node - id [" << masterId << "]" << endl;
        forwardDataToTargetNode(pkt, masterId);
        return;
    }

    emit(receivedPacketFromLowerLayer, pkt);

    auto lteInfo = pkt->getTag<FlowControlInfo>();

    EV << "MecPdcpRrcEnb::fromLowerLayer - Received packet with CID " << lteInfo->getLcid() << "\n";
    EV << "MecPdcpRrcEnb::fromLowerLayer - Packet size " << pkt->getByteLength() << " Bytes\n";

    MacCid cid = idToMacCid(lteInfo->getSourceId(), lteInfo->getLcid());   // TODO: check if you have to get master node id

    LteRxPdcpEntity* entity = getRxEntity(cid);
    entity->handlePacketFromLowerLayer(pkt);
}

void MecPdcpRrcEnb::handleMessage(cMessage* msg)
{
    auto pkt = check_and_cast<inet::Packet *>(msg);
    auto chunk = pkt->peekAtFront<Chunk>();

    EV << "MecPdcpRrcEnb::handleMessage - Received packet " << pkt->getName() << " from port " << pkt->getArrivalGate()->getName() << endl;

    // check whether the message is a notification for mode switch
    if (inet::dynamicPtrCast<const D2DModeSwitchNotification>(chunk) != nullptr)
    {
        auto switchPkt = pkt->peekAtFront<D2DModeSwitchNotification>();
        // call handler
        pdcpHandleD2DModeSwitch(switchPkt->getPeerId(), switchPkt->getNewMode());
        delete pkt;
    }
    else
    {
        //LtePdcpRrcEnb::handleMessage(msg);
        cGate* incoming = pkt->getArrivalGate();
        if (incoming == dataPort_[IN_GATE])
        {
            fromDataPort(pkt);
        }
        else if (incoming == eutranRrcSap_[IN_GATE])
        {
            fromEutranRrcSap(pkt);
        }
        else if (incoming == tmSap_[IN_GATE])
        {
            toEutranRrcSap(pkt);
        }
        else
        {
            fromLowerLayer(pkt);
        }
    }
}
