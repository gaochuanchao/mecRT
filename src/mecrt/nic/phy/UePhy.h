//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// a single layer of NRPhyUe module in simu5g
// simulate the PHY stack of the NIC module of gNB
// LtePhyBase --> LtePhyUe --> LtePhyUeD2D --> NRPhyUe
//
#ifndef _MECRT_UE_AIRPHY_H_
#define _MECRT_UE_AIRPHY_H_

#include "stack/phy/layer/NRPhyUe.h"
#include "mecrt/common/BandManager.h"
#include "mecrt/common/NodeInfo.h"


class BandManager;

/**
 * @class LtePhy
 * @brief Physical layer of Lte Nic.
 *
 * This class implements the physical layer of the Lte Nic.
 * It contains methods to manage analog models and decider.
 *
 * The module receives packets from the LteStack and
 * sends them to the air channel, encapsulated in LteAirFrames.
 *
 * The module receives LteAirFrames from the radioIn gate,
 * filters the received signal using the analog models,
 * processes the received signal using the decider,
 * then decapsulates the inner packet and sends it to the
 * LteStack with LteDeciderControlInfo attached.
 */
class UePhy : public NRPhyUe
{
    friend class DasFilter;

  public:

    /**
    * Constructor
    */
    UePhy();

    /**
    * Destructor
    */
    ~UePhy();

	virtual void addGrantedRsu(MacNodeId id) { grantedRsus_.insert(id); }
	virtual void removeGrantedRsu(MacNodeId id) { grantedRsus_.erase(id); }

  protected:
    bool enableInitDebug_;
    bool resAllocateMode_;  // whether considering resource allocation mode
    bool srsDistanceCheck_;  // whether checking the distance for SRS transmission
    // the distance for SRS transmission, if srsDistanceCheck_ is true, the SRS will only be sent to RSUs within this distance
    double srsDistance_;  

	// ========= for offloading =========
	/***
     * the offloading power consumption of the device. It is different from the txPower: 
     *    offload power is the power of whole NIC module
     *    txPower is the power within the signal (at the transmitter side)
     */
    double offloadPower_;
    BandManager *bandManager_;

	/***
	 * Only do broadcasting when the scheduling is going to start
	 * after scheduling, only send feedback to the offloading rsu to reduce the number of feedback packets
	 * i.e., if no grant is received by the ue, only broadcast the feedback when next scheduling is going to start 
	 */
	double fbPeriod_;
	std::set<MacNodeId> grantedRsus_;

	// ========= for broadcasting =========
	std::set<MacNodeId> rsuSet_;  // the list of RSUs in the simulation

    // ================================
    // ========= LtePhyBase ==========
    // ================================
    /**
     * Processes messages received from #radioInGate_ or from the stack (#upperGateIn_).
     *
     * @param msg message received from stack or from air channel
     */
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    /**
     * Sends a frame to all NICs in range.
     *
     * Frames are sent with zero transmission delay.
     */
    virtual void sendBroadcast(LteAirFrame *airFrame) override;

    /**
     * Sends a frame to the modules registered to the multicast group indicated in the frame
     *
     * Frames are sent with zero transmission delay.
     */
    virtual void sendMulticast(LteAirFrame *frame) override;

    /**
     * Sends a frame uniquely to the dest specified in carried control info.
     *
     * Delay is calculated based on sender's and receiver's positions.
     */
    virtual void sendUnicast(LteAirFrame *airFrame) override;


    // ================================
    // ========= LtePhyUeD2D ==========
    // ================================
    /**
     * Send Feedback, called by feedback generator in DL
     */
    virtual void sendFeedback(LteFeedbackDoubleVector fbDl, LteFeedbackDoubleVector fbUl, FeedbackRequest req) override;

    /**
     * Sends the given message to the wireless channel.
     *
     * Called by the handleMessage() method
     * when a message from #upperGateIn_ gate is received.
     *
     * The message is encapsulated into an LteAirFrame to which
     * a Signal object containing info about TX power, bit-rate and
     * move pattern is attached.
     * The LteAirFrame is then sent to the wireless channel.
     *
     * @param msg packet received from LteStack
     */
    virtual void handleUpperMessage(omnetpp::cMessage* msg) override;


    // ================================
    // =========== NRPhyUe ============
    // ================================

    /**
     * Performs initialization operations to prepare gates' IDs, analog models,
     * the decider and statistics.
     *
     * In stage 0 gets gates' IDs and a pointer to the world module.
     * It also get the CRNTI from RRC layer and initializes statistics
     * to be watched.
     * In stage 1 parses the xml file to fill the #analogModel list and
     * assign the #lteDecider_ pointer.
     *
     * @param stage initialization stage
     */
    virtual void initialize(int stage) override;

    /**
     * Processes messages received from the wireless channel.
     *
     * Called by the handleMessage() method
     * when a message from #radioInGate_ is received.
     *
     * TODO Needs Work
     *
     * #####################################################################
     * This function handles the Airframe by performing following steps:
     * - If airframe is a broadcast/feedback packet and host is
     *   an UE attached to eNB or eNB calls the appropriate
     *   function of the DAS filter
     * - If airframe is received by a UE attached to a Relay
     *   it leaves the received signal unchanged
     * - If airframe is received by eNodeB it performs a loop over
     *   the remoteset written inside the control info and for each
     *   Remote changes the destination (current move variable) with
     *   the remote one before calling filterSignal().
     * - If airframe is received by UE attached to eNB it performs a loop over
     *   the remoteset written inside the control info and for each
     *   Remote changes the source (written inside the signal) with
     *   the remote one before calling filterSignal().
     * At the end only one packet is delivered to the upper layer
     * #####################################################################
     *
     * The analogModels prepared during the initialization phase are
     * applied to the Signal object carried with the received LteAirFrame.
     * Then the decider processes the frame which is sent out to #upperGateOut_
     * gate along with the decider's result (attached as a control info).
     *
     * @param msg LteAirFrame received from the air channel
     */
    virtual void handleAirFrame(cMessage* msg) override;

};


#endif /* _UE_AIRPHY_H_ */
