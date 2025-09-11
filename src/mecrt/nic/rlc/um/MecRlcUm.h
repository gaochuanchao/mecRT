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

#ifndef _MECRT_RLCUM_H_
#define _MECRT_RLCUM_H_

#include "stack/rlc/um/LteRlcUmD2D.h"

/**
 * @class LteRlcUmD2D
 * @brief UM Module
 *
 * This is the UM Module of RLC
 *
 */
class MecRlcUm : public LteRlcUmD2D
{
  protected:

    /**
     * UM Mode
     *
     * handler for traffic coming from
     * lower layer (DTCH, MTCH, MCCH).
     *
     * handleLowerMessage() performs the following task:
     *
     * - reset the tx buffer based on the needs of Mac stack
     * - Search (or add) the proper RXBuffer, depending
     *   on the packet CID
     * - Calls the RXBuffer, that from now on takes
     *   care of the packet
     *
     * @param pkt packet to process
     */
    virtual void handleLowerMessage(omnetpp::cPacket *pkt) override;

    /**
     * handler for traffic coming
     * from the upper layer (PDCP)
     *
     * handleUpperMessage() performs the following tasks:
     * - Adds the RLC-UM header to the packet, containing
     *   the CID, the Traffic Type and the Sequence Number
     *   of the packet (extracted from the IP Datagram)
     * - Search (or add) the proper TXBuffer, depending
     *   on the packet CID
     * - Calls the TXBuffer, that from now on takes
     *   care of the packet
     *
     * @param pkt packet to process
     */
    virtual void handleUpperMessage(omnetpp::cPacket *pkt) override;

};

#endif
