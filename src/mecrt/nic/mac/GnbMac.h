//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// a single layer of NRMacGnb module in simu5g
// simulate the MAC stack of the NIC module of gNB
// LteMacBase --> LteMacEnb --> LteMacEnbD2D --> NRMacGnb
//

#ifndef _MECRT_GNB_MAC_H_
#define _MECRT_GNB_MAC_H_

#include "stack/mac/layer/NRMacGnb.h"

#include "mecrt/apps/server/Server.h"
#include "mecrt/nic/mac/scheduler/GnbSchedulerUl.h"
#include "mecrt/nic/mac/scheduler/GnbSchedulerDl.h"
#include "mecrt/nic/mac/amc/MecNRAmc.h"
#include "mecrt/nic/mac/scheduler/RbManagerUl.h"

class RbManagerUl;
class GnbSchedulerUl;
class GnbSchedulerDl;

class GnbMac : public NRMacGnb
{
    friend class LteHarqBufferTx;
    friend class LteHarqBufferRx;
    friend class LteHarqBufferTxD2D;
    friend class LteHarqBufferRxD2D;
    friend class GnbSchedulerUl;
    friend class GnbSchedulerDl;
    friend class RbManagerUl;

  public:

    /**
     * Initializes MAC Buffers
     */
    GnbMac();

    /**
     * Deletes MAC Buffers
     */
    virtual ~GnbMac();

    /***
     * get the allowed bands for each UE, used for resource allocation algorithms
     *
     * TODO : the minimum resource unit for differentiating UEs is resource blocks (each contain 12 sub-carriers),
     * there might need some adjustment here when a band contains multiple resource blocks.
     */
    virtual std::set<Band> & getAllowedBandsUeUl(MacNodeId nodeId) { return allowedBandsUeUl_[nodeId];}
    virtual std::set<Band> & getAllowedBandsUeDl(MacNodeId nodeId) { return allowedBandsUeDl_[nodeId];}

    /***
     * set the allowed bands for each UE, this should be set by the global scheduler.
     */
    virtual void setAllowedBandsUeUl(MacNodeId nodeId, std::set<Band> bands) { allowedBandsUeUl_[nodeId] = bands;}
    virtual void setAllowedBandsUeDl(MacNodeId nodeId, std::set<Band> bands) { allowedBandsUeDl_[nodeId] = bands;}

    virtual void resetAllowedBandsUe()
    {
        allowedBandsUeUl_.clear();
        allowedBandsUeDl_.clear();
    }

    virtual unsigned int getRbPerBand() { return rbPerBand_;}

    virtual bool getResAllocationMode() { return resAllocateMode_;}


  protected:

    // ================================
    // ========= Newly Added ==========
    // ================================
    bool enableInitDebug_; // whether to enable debug info during initialization

    /// Vec AMC module
    MecNRAmc *amc_;
	  std::vector<cPacket*> grantList_;	// the list of new grant packets sent by RSU

    int numerologyCount_ = 0;  // the number of different numerology

    bool resAllocateMode_;  // whether considering resource allocation mode
    RbManagerUl* rbManagerUl_ = nullptr;  // resource block manager for uplink

    unsigned short serverPort_;   // record the port of the RSU server
    inet::L3Address gnbAddress_;   // the Ipv4/Ipv6 address of the gNB (the cellularNic IP address)

    std::map<AppId, inet::Packet *> appPduList_;  // received data packets from vehicle applications

    std::map<AppId, inet::Ptr<inet::Ipv4Header>> appIpv4Header_;
    std::map<AppId, inet::Ptr<inet::UdpHeader>> appUdpHeader_;

    omnetpp::cMessage * flushAppPduList_;  // flush the app pdu list
    bool srsDistanceCheck_;  // whether to check the distance for SRS transmission
    double srsDistance_;  // the effective distance for SRS transmission

    /***
     * set of allowed bands for each ue, used for frequency division resource allocation
     */
    std::map<MacNodeId, std::set<Band>>   allowedBandsUeUl_;
    std::map<MacNodeId, std::set<Band>>   allowedBandsUeDl_;

    unsigned int rbPerBand_;  // number of resource blocks per band

    GnbSchedulerUl* gnbSchedulerUl_ = nullptr;
    GnbSchedulerDl* gnbSchedulerDl_ = nullptr;

    /***
     * the carrier frequency that can be used for each UE
     * always store the latest frequency information
     */
    std::map<MacNodeId, double> ueCarrierFreq_;  

    int availableBands_;  // the number of available bands for the gNB



    /***
     * send update packet to RSU server
     * the UE sends a feedback for each carrier
     */
    virtual void vecUpdateRsuFeedback(double carrierFreq, MacNodeId ueId, bool isBroadcast, double distance);
    virtual void terminateService(AppId appId);

    /***
     * handle the grant from RSU server to the vehicle
     */
    virtual void vecHandleGrantFromRsu(omnetpp::cPacket* pkt);
    
    virtual void vecSendGrantToVeh(AppId appId, bool isNewGrant, bool isUpdate, bool isStop, bool isPause);

    /**
     * TODO Notify the RSU and scheduler to stop the service if not enough bandwidth for app data offloading
     * status true means the service initialization is success, false means the service needs to be stopped
     */
    virtual void vecServiceFeedback(AppId appId, bool isSuccess);

    // virtual void vecNotifyServiceBandAdjust(AppId appId);

    /**
     * send data packet to RSU server
     */
    virtual void vecSendDataToServer(Packet* packet, MacNodeId ueId, int port, L3Address gnbAddress);

    // send the received data to the upper layer
    virtual void flushAppPduList();


    /**
     * Getter for AMC module
     */
    virtual MecNRAmc *getAmc() override
    {
        return amc_;
    }


    // ================================
    // ========== LteMacEnb ==========
    // ================================

    /**
     * bufferizePacket() is called every time a packet is
     * received from the upper layer
     *
     * @param pkt Packet to be buffered
     * @return TRUE if packet was buffered successfully, FALSE otherwise.
     */
    virtual bool bufferizePacket(omnetpp::cPacket* pkt) override;

    /**
     * bufferizeBsr() works much alike bufferizePacket()
     * but only saves the BSR in the corresponding virtual
     * buffer, eventually creating it if a queue for that
     * cid does not exists yet.
     *
     * @param bsr bsr to store
     * @param cid connection id for this bsr
     */
    void bufferizeBsr(MacBsr* bsr, MacCid cid);

    /**
     * macPduMake() creates MAC PDUs (one for each CID)
     * by extracting SDUs from Real Mac Buffers according
     * to the Schedule List (stored after scheduling).
     * It sends them to H-ARQ
     */
     virtual void macPduMake(MacCid cid) override;

    /**
     * handleUpperMessage() is called every time a packet is
     * received from the upper layer
     */
    virtual void handleUpperMessage(omnetpp::cPacket* pkt) override;

    /**
     * macSduRequest() sends a message to the RLC layer
     * requesting MAC SDUs (one for each CID),
     * according to the Schedule List.
     * Note: command this function first because cannot access the allocator_ within the enbSchedulerDl_ directly
     *       unless define GnbMac as friend class in LteSchedulerEnbDl.h
     */
    virtual void macSduRequest() override;

    /*
     * Get the number of nodes requesting retransmissions for the given carrier
     */
    virtual int getProcessForRtx(double carrierFrequency, Direction dir) override;

    /*
     * Inform the base station that the given node will need a retransmission
     */
    virtual void signalProcessForRtx(MacNodeId nodeId, double carrierFrequency, Direction dir, bool rtx = true) override;

    /*
     * Receives and handles RAC requests
     */
    virtual void macHandleRac(omnetpp::cPacket* pkt) override;


    // ================================
    // ========= LteMacEnbD2D =========
    // ================================

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    /// Lower Layer Handler
    virtual void fromPhy(omnetpp::cPacket *pkt) override;

    /**
     * macHandleFeedbackPkt is called every time a feedback pkt arrives on MAC
     */
    virtual void macHandleFeedbackPkt(omnetpp::cPacket* pkt) override;

    /**
     * Main loop of the Mac level, calls the scheduler
     * and every other function every TTI : must be reimplemented
     * by derivate classes
     */
    virtual void handleSelfMessage() override;

    /**
     * creates scheduling grants (one for each nodeId) according to the Schedule List.
     * It sends them to the  lower layer
     */
    virtual void sendGrants(std::map<double, LteMacScheduleList>* scheduleList) override;

    /**
     * Flush Tx H-ARQ buffers for all users
     */
    virtual void flushHarqBuffers() override;

    /**
     * macPduUnmake() extracts SDUs from a received MAC
     * PDU and sends them to the upper layer.
     *
     * On ENB it also extracts the BSR Control Element
     * and stores it in the BSR buffer (for the cid from
     * which packet was received)
     *
     * @param pkt container packet
     */
    virtual void macPduUnmake(omnetpp::cPacket* pkt) override;


    // ================================
    // ========== NRMacGnb ==========
    // ================================

    /**
     * Reads MAC parameters for ue and performs initialization.
     */
    virtual void initialize(int stage) override;
};


#endif
