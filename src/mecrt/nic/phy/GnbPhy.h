//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// a single layer of LtePhyEnbD2D module in simu5g
// simulate the PHY stack of the NIC module of gNB
// LtePhyBase --> LtePhyEnb --> LtePhyEnbD2D
//
#ifndef _MECRT_GNB_AIRPHY_H_
#define _MECRT_GNB_AIRPHY_H_

#include "stack/phy/layer/LtePhyEnbD2D.h"

/**
 * @class GnbPhy
 * @brief Physical layer of gNB Nic.
 *
 * This class implements the physical layer (stack) of the gNB Nic.
 * It contains methods to manage analog models and decider.
 *
 * The module receives packets from the MAC Stack and
 * sends them to the air channel, encapsulated in LteAirFrames.
 *
 * The module receives NRAirFrames from the radioIn gate,
 * filters the received signal using the analog models,
 * processes the received signal using the decider,
 * then decapsulates the inner packet and sends it to the
 * MAC Stack with LteDeciderControlInfo attached.
 */

class GnbPhy : public LtePhyEnbD2D
{
    friend class DasFilter;

  protected:
    bool enableInitDebug_;
    bool resAllocateMode_;  // whether considering resource allocation mode

    // ========= LtePhyEnbD2D ==========
    bool enableD2DCqiReporting_;

  public:

    /**
     * Constructor
     */
    GnbPhy() {  enableInitDebug_ = false; };

    /**
     * Destructor
     */
    ~GnbPhy() 
    { 
        if (enableInitDebug_)
            std::cout << "GnbPhy::~GnbPhy - destroying GnbPhy module\n";
    };

  protected:

    // ================================
    // ========== LtePhyBase ==========
    // ================================

    /**
     * Processes messages received from #radioInGate_ or from the stack (#upperGateIn_).
     *
     * @param msg message received from stack or from air channel
     */
    virtual void handleMessage(omnetpp::cMessage *msg) override;

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
    // ========== LtePhyEnb ===========
    // ================================

    // these two method is not defined as virtual in class LtePhyEnb, the virtual used here is for future child class
    virtual bool handleControlPkt(UserControlInfo* lteinfo, LteAirFrame* frame);
    virtual void handleFeedbackPkt(UserControlInfo* lteinfo, LteAirFrame* frame);

    // ================================
    // ========= LtePhyEnbD2D ==========
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
    virtual void handleAirFrame(omnetpp::cMessage* msg) override;

    virtual void requestFeedback(UserControlInfo* lteinfo, LteAirFrame* frame, inet::Packet * pkt) override;

};



#endif  /* _GNB_AIRPHY_H_ */
