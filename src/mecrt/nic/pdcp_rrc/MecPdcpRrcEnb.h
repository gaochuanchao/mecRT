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

#ifndef _MECRT_VEC_PDCPRRCENB_H_
#define _MECRT_VEC_PDCPRRCENB_H_


#include <omnetpp.h>

#include "stack/pdcp_rrc/layer/NRPdcpRrcEnb.h"

/**
 * @class NRPdcpRrcUe
 * @brief PDCP Layer
 *
 * This is the PDCP/RRC layer of NR Stack
 *
 */
class MecPdcpRrcEnb : public NRPdcpRrcEnb
{
  protected:

    virtual void handleMessage(omnetpp::cMessage* msg) override;

    /**
    * handler for data port
    * @param pkt incoming packet
    */
    virtual void fromDataPort(cPacket *pkt) override;

    /**
     * handler for um/am sap
     *
     * it performs the following steps:
     * - decompresses the header, restoring original packet
     * - decapsulates the packet
     * - sends the packet to the application layer
     *
     * @param pkt incoming packet
     */
    virtual void fromLowerLayer(cPacket *pkt) override;
};

#endif
