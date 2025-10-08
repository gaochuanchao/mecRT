//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecRlcUm.cc / MecRlcUm.h
//
//  Description:
//    This file implements the RLC/UM layer for NR (New Radio) in the UE/gNB.
//    Original file: simu5g - "stack/rlc/um/LteRlcUm.h"
//    We add control logic for MEC service subscription.
//    
//    LteRlcUm --> LteRlcUmD2D --> GnbRlcUm
// 
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//
#include "mecrt/nic/rlc/um/MecRlcUm.h"

#include "stack/mac/packet/LteMacSduRequest.h"
#include "stack/d2dModeSelection/D2DModeSwitchNotification_m.h"

Define_Module(MecRlcUm);
using namespace omnetpp;


void MecRlcUm::handleLowerMessage(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);

    // if it is a service request packet, no need to buffer, direct send to the pdcp stack
    std::string pktName = pkt->getName();
    if(pktName == "SrvReq" || pktName == "RsuFD" || pktName == "SrvFD" || pktName == "VehGrant")
    {
        // forward packet to PDCP
        EV << "MecRlcUm::handleLowerMessage - Sending packet " << pktName << " to port UM_Sap_up$o\n";
        send(pkt, up_[OUT_GATE]);
        return;
    }
    else
    {
        auto chunk = pkt->peekAtFront<inet::Chunk>();

        if (inet::dynamicPtrCast<const D2DModeSwitchNotification>(chunk) != nullptr)
        {
            EV << NOW << " MecRlcUm::handleLowerMessage - Received packet " << pkt->getName() << " from lower layer\n";
    
            auto switchPkt = pkt->peekAtFront<D2DModeSwitchNotification>();
            auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
    
            // add here specific behavior for handling mode switch at the RLC layer
    
            if (switchPkt->getTxSide())
            {
                // get the corresponding Rx buffer & call handler
                UmTxEntity* txbuf = getTxBuffer(lteInfo);
                txbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection(), switchPkt->getClearRlcBuffer());
    
                // forward packet to PDCP
                EV << "MecRlcUm::handleLowerMessage - Sending packet " << pkt->getName() << " to port UM_Sap_up$o\n";
                send(pkt, up_[OUT_GATE]);
            }
            else  // rx side
            {
                // get the corresponding Rx buffer & call handler
                UmRxEntity* rxbuf = getRxBuffer(lteInfo);
                rxbuf->rlcHandleD2DModeSwitch(switchPkt->getOldConnection(), switchPkt->getOldMode(), switchPkt->getClearRlcBuffer());
    
                delete pkt;
                pkt = nullptr;
            }
        }
        else
        {
            // LteRlcUm::handleLowerMessage(pkt);
            EV << "MecRlcUm::handleLowerMessage - Received packet " << pkt->getName() << " from lower layer\n";
            auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

            if (inet::dynamicPtrCast<const LteMacSduRequest>(chunk) != nullptr)
            {
                // get the corresponding Tx buffer
                UmTxEntity* txbuf = getTxBuffer(lteInfo);

                auto macSduRequest = pkt->peekAtFront<LteMacSduRequest>();
                unsigned int size = macSduRequest->getSduSize();

                drop(pkt);

                // do segmentation/concatenation and send a pdu to the lower layer
                txbuf->rlcPduMake(size);
                // since the mac stack always fetches the whole PDU, we clear the queue to ensure no residual data in the buffer
                txbuf->clearQueue();

                delete pkt;
            }
            else
            {
                emit(receivedPacketFromLowerLayer, pkt);

                // Extract informations from fragment
                UmRxEntity* rxbuf = getRxBuffer(lteInfo);
                drop(pkt);

                // Bufferize PDU
                EV << "LteRlcUm::handleLowerMessage - Enque packet " << pkt->getName() << " into the Rx Buffer\n";
                rxbuf->enque(pkt);
            }
        }
    }
}

void MecRlcUm::handleUpperMessage(cPacket *pktAux)
{
    emit(receivedPacketFromUpperLayer, pktAux);

    auto pkt = check_and_cast<inet::Packet *> (pktAux);
    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    EV << "MecRlcUm::handleUpperMessage - Received packet " << pkt->getName() << " from upper layer, size " << pktAux->getByteLength() << "\n";

    // if it is a service request packet, no need to buffer, direct send to the mac stack
    if(!strcmp(pkt->getName(), "SrvReq"))
    {
        EV << "MecRlcUm::handleUpperMessage - It is a service request, no need buffering. Sending packet " << pkt->getName() << " to port UM_Sap_down$o\n";
        send(pkt, down_[OUT_GATE]);
        return;
    }

    if(!strcmp(pkt->getName(), "NicGrant"))
    {
        EV << "MecRlcUm::handleUpperMessage - It is a grant packet form RSU server, no need buffering. Sending packet " 
                    << pkt->getName() << " to port UM_Sap_down$o\n";
        send(pkt, down_[OUT_GATE]);
        return;
    }    

    UmTxEntity* txbuf = getTxBuffer(lteInfo);

    // Create a new RLC packet
    auto rlcPkt = inet::makeShared<LteRlcSdu>();
    rlcPkt->setSnoMainPacket(lteInfo->getSequenceNumber());
    rlcPkt->setLengthMainPacket(pkt->getByteLength());
    pkt->insertAtFront(rlcPkt);

    drop(pkt);

    if (txbuf->isHoldingDownstreamInPackets())
    {
        // do not store in the TX buffer and do not signal the MAC layer
        EV << "MecRlcUm::handleUpperMessage - Enque packet " << rlcPkt->getClassName() << " into the Holding Buffer\n";
        txbuf->enqueHoldingPackets(pkt);
    }
    else
    {
        if(txbuf->enque(pkt)){
            EV << "MecRlcUm::handleUpperMessage - Enque packet " << rlcPkt->getClassName() << " into the Tx Buffer\n";

            // create a message so as to notify the MAC layer that the queue contains new data
            auto newDataPkt = inet::makeShared<LteRlcPduNewData>();
            // make a copy of the RLC SDU
            auto pktDup = pkt->dup();
            pktDup->insertAtFront(newDataPkt);
            // the MAC will only be interested in the size of this packet

            EV << "MecRlcUm::handleUpperMessage - Sending message " << newDataPkt->getClassName() << " to port UM_Sap_down$o\n";
            send(pktDup, down_[OUT_GATE]);
        } else {
            // Queue is full - drop SDU
            dropBufferOverflow(pkt);
        }
    }
}

