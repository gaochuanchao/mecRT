//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// a single layer of NRMacGnb module in simu5g
// simulate the MAC stack of the NIC module of UE
// LteRlcUm --> LteRlcUmD2D --> GnbRlcUm
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
