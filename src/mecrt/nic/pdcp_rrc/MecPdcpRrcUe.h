//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// a single layer of NRMacGnb module in simu5g
// simulate the MAC stack of the NIC module of UE
// ... --> LtePdcpRrcUeD2D --> NRPdcpRrcUe --> GnbPdcpRrcUe
//

#ifndef _MECRT_GNB_PDCPRRCUE_H_
#define _MECRT_GNB_PDCPRRCUE_H_


#include <omnetpp.h>
#include "stack/pdcp_rrc/layer/NRPdcpRrcUe.h"
#include "stack/pdcp_rrc/layer/LtePdcpRrcUeD2D.h"

/**
 * @class NRPdcpRrcUe
 * @brief PDCP Layer
 *
 * This is the PDCP/RRC layer of NR Stack
 *
 */
class MecPdcpRrcUe : public LtePdcpRrcUeD2D
{
    cGate* nrTmSap_[2];
    cGate* nrUmSap_[2];
    cGate* nrAmSap_[2];

    /// Identifier for this node
    MacNodeId nrNodeId_;

    // flag for enabling Dual Connectivity
    bool dualConnectivityEnabled_;

    // map to store the port number for each IPv4 ID when the packet is fragmented
    std::map<unsigned short, unsigned short> ipv4IdToPort_; // <IPv4 ID, port>

  protected:

    virtual void initialize(int stage) override;

    virtual void handleMessage(omnetpp::cMessage *msg) override;

    /**
     * getNrNodeId(): returns the ID of this node
     */
    virtual MacNodeId getNrNodeId() override { return nrNodeId_; }

    // this function overrides the one in the base class because we need to distinguish the nodeId of the sender
    // i.e. whether to use the NR nodeId or the LTE one
    Direction getDirection(MacNodeId srcId, MacNodeId destId)
    {
        if (binder_->getD2DCapability(srcId, destId) && binder_->getD2DMode(srcId, destId) == DM)
            return D2D;
        return UL;
    }

    // this function was redefined so as to use the getDirection() function implemented above
    virtual MacNodeId getDestId(inet::Ptr<FlowControlInfo> lteInfo) override;

    /**
     * handler for data port
     * @param pkt incoming packet
     */
    virtual void fromDataPort(cPacket *pkt) override;

    /**
     * getEntity() is used to gather the NR PDCP entity
     * for that LCID. If entity was already present, a reference
     * is returned, otherwise a new entity is created,
     * added to the entities map and a reference is returned as well.
     *
     * @param lcid Logical CID
     * @return pointer to the PDCP entity for the LCID of the flow
     *
     */
    virtual LteTxPdcpEntity* getTxEntity(MacCid lcid) override;
    virtual LteRxPdcpEntity* getRxEntity(MacCid lcid) override;

    /*
     * sendToLowerLayer() forwards a PDCP PDU to the RLC layer
     */
    virtual void sendToLowerLayer(Packet *pkt) override;

    /*
     * Dual Connectivity support
     */
    virtual bool isDualConnectivityEnabled() override { return dualConnectivityEnabled_; }

  public:

    virtual void deleteEntities(MacNodeId nodeId) override;
};

#endif
