//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    UeMac.cc / UeMac.h
//
//  Description:
//    This file implements the MAC layer for the UE in the MEC context.
//    Compared to the original NRMacUe, we add the control logic for data offloading.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//
// simulate the MAC stack of the NIC module of UE
// LteMac --> LteMacBase --> LteMacUe --> LteMacUeD2D --> NRMacUe
//

#ifndef _MECRT_UE_MAC_H_
#define _MECRT_UE_MAC_H_

#include "stack/mac/layer/NRMacUe.h"
#include "mecrt/common/MecCommon.h"
#include "mecrt/packets/apps/Grant2Veh.h"
#include "mecrt/mobility/MecMobility.h"
#include "mecrt/common/NodeInfo.h"

#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/transportlayer/udp/UdpHeader_m.h"

class UeMac : public NRMacUe
{
    friend class LteHarqBufferTx;
    friend class LteHarqBufferRx;
    friend class LteHarqBufferTxD2D;
    friend class LteHarqBufferRxD2D;

  public:

    /**
     * Initializes MAC Buffers
     */
    UeMac();

    /**
     * Deletes MAC Buffers
     */
    virtual ~UeMac();

  protected:
    // to collect the duplicated grants information
    int dupCount_ = 0;
    omnetpp::cMessage *dupCountTimer_;   /// start the scheduling
    omnetpp::simsignal_t dupCountSignal_;

    NodeInfo *nodeInfo_;  // the node information of the vehicle

    bool enableInitDebug_;
    std::map<AppId, inet::IntrusivePtr<const Grant2Veh>> vecGrant_;
    std::set<AppId> grantedApp_;
    std::map<AppId, double> grantFrequency_;
    std::set<AppId> requestedApps_;   // apps that has requested the data from RLC
    std::map<AppId, int> requiredTtiCount_;  // the required time for the app to transmit the data

    std::map<AppId, inet::Packet *> appPduList_;

    bool resAllocateMode_;  // whether considering resource allocation mode

    MecMobility *mobility_;   // the mobility module of the vehicle
    simtime_t moveStartTime_;	// the start time of the provided file, start moving
	  simtime_t moveStoptime_;	// the last time of provided file, stop moving

    // handle the grant from RSU server to the vehicle
    virtual void vecHandleVehicularGrant(cPacket* pktAux);

    // handle the RSU grant for resource allocation mode
    virtual int vecRequestBufferedData();

    /**
     * macPduMake() creates MAC PDUs (one for each CID)
     * by extracting SDUs from Real Mac Buffers according
     * to the Schedule List.
     * It sends them to H-ARQ (at the moment lower layer)
     *
     * On UE it also adds a BSR control element to the MAC PDU
     * containing the size of its buffer (for that CID)
     */
    virtual void vecMacPduMake(MacCid cid=0);

    // flush the app pdu list
    virtual void vecFlushAppPduList();

    /***
     * attach the ue to the all gNBs in the simulation
     */
    virtual void attachToGnb(MacNodeId gnbId);

    // ================================
    // ========== LteMacBase ==========
    // ================================

    /// Lower Layer Handler
    virtual void fromPhy(omnetpp::cPacket *pkt) override;

    /**
     * sendUpperPackets() is used
     * to send packets to upper layer
     *
     * @param pkt Packet to send
     */
    virtual void sendUpperPackets(omnetpp::cPacket* pkt);

    /**
     * sendLowerPackets() is used
     * to send packets to lower layer
     *
     * @param pkt Packet to send
     */
    virtual void sendLowerPackets(omnetpp::cPacket* pkt);

    // ================================
    // =========== LteMacUe ===========
    // ================================

    /**
     * macPduUnmake() extracts SDUs from a received MAC
     * PDU and sends them to the upper layer.
     *
     * @param pkt container packet
     */
    virtual void macPduUnmake(omnetpp::cPacket* pkt) override;

    /**
     * handleUpperMessage() is called every time a packet is
     * received from the upper layer
     */
    virtual void handleUpperMessage(omnetpp::cPacket* pkt) override;

    /**
     * bufferizePacket() is called every time a packet is
     * received from the upper layer
     */
    virtual bool bufferizePacket(omnetpp::cPacket* pkt) override;

    /**
     * Flush Tx H-ARQ buffers for the user
     */
    virtual void flushHarqBuffers() override;


    // ================================
    // ========== LteMacUeD2D =========
    // ================================

    /**
     * Reads MAC parameters for ue and performs initialization.
     */
    virtual void initialize(int stage) override;

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    /*
     * Checks RAC status
     */
    virtual void checkRAC() override;

    /*
     * Receives and handles RAC responses
     */
    virtual void macHandleRac(omnetpp::cPacket* pkt) override;

    /*
     * Receives and handles scheduling grants
     */
    virtual void macHandleGrant(omnetpp::cPacket* pkt) override;

    // ================================
    // ============ NRMacUe ===========
    // ================================

    /**
     * Main loop of the Mac level, calls the scheduler
     * and every other function every TTI : must be reimplemented
     * by derivate classes
     */
    virtual void handleSelfMessage() override;

    virtual void vecHandleSelfMessage();

    /**
     * macSduRequest() sends a message to the RLC layer
     * requesting MAC SDUs (one for each CID),
     * according to the Schedule List.
     */
    virtual int macSduRequest() override;

    /**
     * macPduMake() creates MAC PDUs (one for each CID)
     * by extracting SDUs from Real Mac Buffers according
     * to the Schedule List.
     * It sends them to H-ARQ (at the moment lower layer)
     *
     * On UE it also adds a BSR control element to the MAC PDU
     * containing the size of its buffer (for that CID)
     */
    virtual void macPduMake(MacCid cid=0) override;

};


#endif
