//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// a single layer of NRMacGnb module in simu5g
// simulate the MAC stack of the NIC module of UE
// ... --> LtePdcpRrcUeD2D --> NRPdcpRrcUe --> GnbPdcpRrcUe
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
