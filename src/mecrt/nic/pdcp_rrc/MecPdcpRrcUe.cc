//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecPdcpRrcUe.cc / MecPdcpRrcUe.h
//
//  Description:
//    This file implements the PDCP/RRC layer for NR (New Radio) in the UE (User Equipment).
//    Original file: simu5g - "stack/pdcp_rrc/layer/NRPdcpRrcUe.h"
//    We add the control logic for MEC service subscription.
//
//    ... --> LtePdcpRrcUeD2D --> NRPdcpRrcUe --> GnbPdcpRrcUe
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/nic/pdcp_rrc/MecPdcpRrcUe.h"

#include "stack/pdcp_rrc/layer/entity/NRTxPdcpEntity.h"
#include "stack/pdcp_rrc/layer/entity/NRRxPdcpEntity.h"
#include "stack/packetFlowManager/PacketFlowManagerBase.h"

#include "inet/transportlayer/udp/UdpHeader_m.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "mecrt/packets/apps/VecPacket_m.h"

Define_Module(MecPdcpRrcUe);

/*
 * Upper Layer handlers
 */
void MecPdcpRrcUe::fromDataPort(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayer, pktAux);

    // Control Information
    auto pkt = check_and_cast<Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    setTrafficInformation(pkt, lteInfo);

    // select the correct nodeId
    MacNodeId nodeId = (lteInfo->getUseNR()) ? getNrNodeId() : nodeId_;

    // get destination info
    Ipv4Address destAddr = Ipv4Address(lteInfo->getDstAddr());
    // MacNodeId destId;

    lteInfo->setDirection(UL);
    lteInfo->setD2dTxPeerId(0);
    lteInfo->setD2dRxPeerId(0);

    // the direction of the incoming connection is a D2D_MULTI one if the application is of the same type,
    // else the direction will be selected according to the current status of the UE, i.e. D2D or UL
    // if (destAddr.isMulticast())
    // {
    //     binder_->addD2DMulticastTransmitter(nodeId);

    //     lteInfo->setDirection(D2D_MULTI);

    //     // assign a multicast group id
    //     // multicast IP addresses are 224.0.0.0/4.
    //     // We consider the host part of the IP address (the remaining 28 bits) as identifier of the group,
    //     // so as it is univocally determined for the whole network
    //     uint32_t address = Ipv4Address(lteInfo->getDstAddr()).getInt();
    //     uint32_t mask = ~((uint32_t)255 << 28);      // 0000 1111 1111 1111
    //     uint32_t groupId = address & mask;
    //     lteInfo->setMulticastGroupId((int32_t)groupId);
    // }
    // else
    // {
    //     destId = binder_->getMacNodeId(destAddr);
    //     if (destId != 0)  // the destination is a UE within the LTE network
    //     {
    //         if (binder_->checkD2DCapability(nodeId, destId))
    //         {
    //             // this way, we record the ID of the endpoints even if the connection is currently in IM
    //             // this is useful for mode switching
    //             lteInfo->setD2dTxPeerId(nodeId);
    //             lteInfo->setD2dRxPeerId(destId);
    //         }
    //         else
    //         {
    //             lteInfo->setD2dTxPeerId(0);
    //             lteInfo->setD2dRxPeerId(0);
    //         }

    //         // set actual flow direction based (D2D/UL) based on the current mode (DM/IM) of this peering
    //         lteInfo->setDirection(getDirection(nodeId,destId));
    //     }
    //     else  // the destination is outside the LTE network
    //     {
    //         lteInfo->setDirection(UL);
    //         lteInfo->setD2dTxPeerId(0);
    //         lteInfo->setD2dRxPeerId(0);
    //     }
    // }

    // Cid Request
    EV << "MecPdcpRrcUe::fromDataPort - Received CID request for Traffic [ " << "Source: " << Ipv4Address(lteInfo->getSrcAddr())
            << " Destination: " << Ipv4Address(lteInfo->getDstAddr())
            << " , ToS: " << lteInfo->getTypeOfService()
            << " , Direction: " << dirToA((Direction)lteInfo->getDirection()) << " ]\n";

    /*
     * Different lcid for different directions of the same flow are assigned.
     * RLC layer will create different RLC entities for different LCIDs
     */

    if(!strcmp(pkt->getName(), "SrvReq"))
    {
        EV << "MecPdcpRrcUe::fromDataPort - vehicle service request packet, send to lower RLC stack " << endl;

        lteInfo->setSourceId(getNodeId());   // TODO CHANGE HERE!!! Must be the NR node ID if this is a NR connection

        if (lteInfo->getMulticastGroupId() > 0)   // destId is meaningless for multicast D2D (we use the id of the source for statistic purposes at lower levels)
            lteInfo->setDestId(getNodeId());
        else
            lteInfo->setDestId(getDestId(lteInfo));

        sendToLowerLayer(pkt);

        return;
    }

    /**
     * if the data packet is fragmented, only the first fragment has the UDP header
     * Check More Fragments (MF) and Fragment Offset:
     *       MF = 0, Offset = 0 → Normal, not fragmented.
     *       MF = 1, Offset = 0 → First fragment.
     *       MF = 1, Offset > 0 → Middle fragment.
     *       MF = 0, Offset > 0 → Last fragment.
     * 
     * when Offset = 0, the packet is rither the first fragment or not fragmented, a udp header is present
     * when Offset > 0, the packet is either the last fragment or a middle fragment, a udp header is not present
     * when it is the last fragment, we can remove the recorded port number from the map ipv4IdToPort_
     */

    /***
     * in the original function NRPdcpRrcUe::fromDataPort(), the lcid is determined with the combination of
     *      srcAddr, dstAddr, typeOfService, direction
     *          to get typeOfService, call function lteInfo->getTypeOfService()
     * in VEC, we want to define a different lcid for each application (with a unique port); therefore, we determine lcid based on
     *      srcAddr, dstAddr, port, direction
     */
    auto ipv4Head = pkt->removeAtFront<Ipv4Header>();
    auto udpHead = pkt->peekAtFront<UdpHeader>();
    unsigned short port = udpHead->getSrcPort();
    pkt->insertAtFront(ipv4Head);

    EV << "MecPdcpRrcUe::fromDataPort - source port " << port << endl;

    LogicalCid mylcid;
    if ((mylcid = ht_->find_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(), port, lteInfo->getDirection())) == 0xFFFF)
    {
        // LCID not found

        // assign a new LCID to the connection
        // mylcid = lcid_++;
        mylcid = port;

        EV << "MecPdcpRrcUe::fromDataPort - Connection not found, new CID created with LCID " << mylcid << "\n";

        ht_->create_entry(lteInfo->getSrcAddr(), lteInfo->getDstAddr(), port, lteInfo->getDirection(), port);
    }

    mylcid = port;
    // assign LCID
    lteInfo->setLcid(mylcid);

    EV << "MecPdcpRrcUe::fromDataPort - Assigned Lcid: " << mylcid << "\n";
    EV << "MecPdcpRrcUe::fromDataPort - Assigned Node ID: " << nodeId << "\n";

    // get effective next hop dest ID
    // destId = getDestId(lteInfo);

    // obtain CID
    // MacCid cid = idToMacCid(destId, mylcid);

    MacCid cid = idToMacCid(nodeId, mylcid);
    EV << "MecPdcpRrcUe::fromDataPort - node ["<< nodeId<<"], lcid ["<< mylcid<<"], and cid ["<<cid<<"]"<< endl;

    // get the PDCP entity for this CID and process the packet
    LteTxPdcpEntity* entity = getTxEntity(cid);
    entity->handlePacketFromUpperLayer(pkt);
}


void MecPdcpRrcUe::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        dualConnectivityEnabled_ = getAncestorPar("dualConnectivityEnabled").boolValue();

        // initialize gates
        nrTmSap_[IN_GATE] = gate("TM_Sap$i",1);
        nrTmSap_[OUT_GATE] = gate("TM_Sap$o",1);
        nrUmSap_[IN_GATE] = gate("UM_Sap$i",1);
        nrUmSap_[OUT_GATE] = gate("UM_Sap$o",1);
        nrAmSap_[IN_GATE] = gate("AM_Sap$i",1);
        nrAmSap_[OUT_GATE] = gate("AM_Sap$o",1);
    }
    else if (stage == inet::INITSTAGE_NETWORK_CONFIGURATION)
    {
        nrNodeId_ = getAncestorPar("nrMacNodeId");
        EV << "MecPdcpRrcUe::initialize - nrNodeId " << nrNodeId_ << endl;
    }
        
    LtePdcpRrcUeD2D::initialize(stage);

}

void MecPdcpRrcUe::handleMessage(cMessage* msg)
{
    cPacket* pktAux = check_and_cast<cPacket *>(msg);

    // check whether the message is a notification for mode switch
    if (strcmp(pktAux->getName(),"VehGrant") == 0)
    {
        EV << "MecPdcpRrcUe::handleMessage - Received vehicle grant packet " << pktAux->getName() << endl;
        auto pkt = check_and_cast<Packet *>(pktAux);
        take(pkt);
        pkt->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ipv4);
        sendToUpperLayer(pkt);
    }
    else
    {
        LtePdcpRrcUeD2D::handleMessage(msg);
    }
}

MacNodeId MecPdcpRrcUe::getDestId(inet::Ptr<FlowControlInfo> lteInfo)
{
    Ipv4Address destAddr = Ipv4Address(lteInfo->getDstAddr());
    MacNodeId destId = binder_->getMacNodeId(destAddr);
    MacNodeId srcId = (lteInfo->getUseNR())? nrNodeId_ : nodeId_;

    // check whether the destination is inside or outside the LTE network
    if (destId == 0 || getDirection(srcId,destId) == UL)
    {
        // if not, the packet is destined to the eNB

        // UE is subject to handovers: master may change
        return binder_->getNextHop(lteInfo->getSourceId());
    }

    return destId;
}


LteTxPdcpEntity* MecPdcpRrcUe::getTxEntity(MacCid lcid)
{
    // Find entity for this LCID
    PdcpTxEntities::iterator it = txEntities_.find(lcid);
    if (it == txEntities_.end())
    {
        // Not found: create
        std::stringstream buf;
        buf << "NRTxPdcpEntity Lcid: " << lcid;
        cModuleType* moduleType = cModuleType::get("simu5g.stack.pdcp_rrc.NRTxPdcpEntity");
        NRTxPdcpEntity* txEnt = check_and_cast<NRTxPdcpEntity*>(moduleType->createScheduleInit(buf.str().c_str(), this));

        txEntities_[lcid] = txEnt;    // Add to entities map

        EV << "MecPdcpRrcUe::getEntity - Added new PdcpEntity for Lcid: " << lcid << "\n";

        return txEnt;
    }
    else
    {
        // Found
        EV << "MecPdcpRrcUe::getEntity - Using old PdcpEntity for Lcid: " << lcid << "\n";

        return it->second;
    }
}

LteRxPdcpEntity* MecPdcpRrcUe::getRxEntity(MacCid cid)
{
    // Find entity for this CID
    PdcpRxEntities::iterator it = rxEntities_.find(cid);
    if (it == rxEntities_.end())
    {
        // Not found: create

        std::stringstream buf;
        buf << "NRRxPdcpEntity cid: " << cid;
        cModuleType* moduleType = cModuleType::get("simu5g.stack.pdcp_rrc.NRRxPdcpEntity");
        NRRxPdcpEntity* rxEnt = check_and_cast<NRRxPdcpEntity*>(moduleType->createScheduleInit(buf.str().c_str(), this));
        rxEntities_[cid] = rxEnt;    // Add to entities map

        EV << "MecPdcpRrcUe::getRxEntity - Added new RxPdcpEntity for Cid: " << cid << "\n";

        return rxEnt;
    }
    else
    {
        // Found
        EV << "MecPdcpRrcUe::getRxEntity - Using old RxPdcpEntity for Cid: " << cid << "\n";

        return it->second;
    }
}

void MecPdcpRrcUe::deleteEntities(MacNodeId nodeId)
{
    PdcpTxEntities::iterator tit;
    PdcpRxEntities::iterator rit;

    // delete connections related to the given master nodeB only
    // (the UE might have dual connectivity enabled)
    for (tit = txEntities_.begin(); tit != txEntities_.end(); )
    {
        if (MacCidToNodeId(tit->first) == nodeId)
        {
            (tit->second)->deleteModule();  // Delete Entity
            txEntities_.erase(tit++);       // Delete Elem
        }
        else
        {
            ++tit;
        }
    }
    for (rit = rxEntities_.begin(); rit != rxEntities_.end(); )
    {
        if (MacCidToNodeId(rit->first) == nodeId)
        {
            (rit->second)->deleteModule();  // Delete Entity
            rxEntities_.erase(rit++);       // Delete Elem
        }
        else
        {
            ++rit;
        }
    }
}

void MecPdcpRrcUe::sendToLowerLayer(Packet *pkt)
{

    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    if (!dualConnectivityEnabled_ || lteInfo->getUseNR())
    {
        EV << "MecPdcpRrcUe : Sending packet " << pkt->getName() << " on port "
           << (lteInfo->getRlcType() == UM ? "NR_UM_Sap$o\n" : "NR_AM_Sap$o\n");

        // use NR id as source
        lteInfo->setSourceId(nrNodeId_);

        // notify the packetFlowManager only with UL packet
        if(lteInfo->getDirection() != D2D_MULTI && lteInfo->getDirection() != D2D)
        {
            if(NRpacketFlowManager_!= nullptr)
            {
                EV << "LteTxPdcpEntity::handlePacketFromUpperLayer - notify NRpacketFlowManager_" << endl;
                NRpacketFlowManager_->insertPdcpSdu(pkt);
            }
        }

        // Send message
        send(pkt, (lteInfo->getRlcType() == UM ? nrUmSap_[OUT_GATE] : nrAmSap_[OUT_GATE]));

        emit(sentPacketToLowerLayer, pkt);
    }
    else
        LtePdcpRrcBase::sendToLowerLayer(pkt);
}

