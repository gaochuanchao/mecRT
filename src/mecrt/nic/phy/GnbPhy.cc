//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// a single layer of LtePhyEnbD2D module in simu5g
// simulate the PHY stack of the NIC module of gNB
// LtePhyBase --> LtePhyEnb --> LtePhyEnbD2D
//
#include "mecrt/nic/phy/GnbPhy.h"
#include "stack/phy/das/DasFilter.h"
#include "stack/phy/packet/LteFeedbackPkt.h"
#include "common/LteCommon.h"

Define_Module(GnbPhy);

//short LtePhyBase::airFramePriority_ = 10;

void GnbPhy::initialize(int stage)
{
    ChannelAccess::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL)
    {
        if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
        if (enableInitDebug_)
            std::cout << "GnbPhy::initialize - stage: INITSTAGE_LOCAL - begins" << std::endl;

        airFramePriority_ = -1; // smaller value means higher priority

        binder_ = getBinder();
        // get gate ids
        upperGateIn_ = findGate("upperGateIn");
        upperGateOut_ = findGate("upperGateOut");
        radioInGate_ = findGate("radioIn");

        // Initialize and watch statistics
        numAirFrameReceived_ = numAirFrameNotReceived_ = 0;
        ueTxPower_ = par("ueTxPower");  // default(26)
        eNodeBtxPower_ = par("eNodeBTxPower");  // default(46)
        microTxPower_ = par("microTxPower");    // default(30)

        carrierFrequency_ = 2.1e+9;
        WATCH(numAirFrameReceived_);
        WATCH(numAirFrameNotReceived_);

        multicastD2DRange_ = par("multicastD2DRange");  // default(1000m)
        enableMulticastD2DRangeCheck_ = par("enableMulticastD2DRangeCheck");    // default(false)

        // ========== LtePhyEnb ===========
        // get local id
        nodeId_ = getAncestorPar("macNodeId");  // defined in GnbMac, the macNodeId of the NIC module
        EV << "Local MacNodeId: " << nodeId_ << endl;
        // cellInfo_ = getCellInfo(nodeId_);
        cellInfo_ = check_and_cast<CellInfo*>(getParentModule()->getParentModule()->getSubmodule("cellInfo"));
        if (cellInfo_ != NULL)
        {
            // the lambda in the update defines parameters related to wavelength or frequency channels
            cellInfo_->channelUpdate(nodeId_, intuniform(1, binder_->phyPisaData.maxChannel2()));
            das_ = new DasFilter(this, binder_, cellInfo_->getRemoteAntennaSet(), 0);
        }
        // isNr_ = (strcmp(getAncestorPar("nicType").stdstringValue().c_str(),"NRNicEnb") == 0) ? true : false;
        isNr_ = (strcmp(getAncestorPar("nodeType").stdstringValue().c_str(),"GNODEB") == 0) ? true : false;
        nodeType_ = (isNr_) ? GNODEB : ENODEB;
        WATCH(nodeType_);

        // ========== LtePhyEnbD2D ===========
        enableD2DCqiReporting_ = par("enableD2DCqiReporting");  // default(true)

        resAllocateMode_ = par("resAllocateMode");  // default(true)
        WATCH(resAllocateMode_);

        if (enableInitDebug_)
            std::cout << "GnbPhy::initialize - stage: INITSTAGE_LOCAL - ends" << std::endl;
    }
    else if (stage == inet::INITSTAGE_NETWORK_INTERFACE_CONFIGURATION)  // stage == 1
    {
        if (enableInitDebug_)
            std::cout << "GnbPhy::initialize - stage: INITSTAGE_NETWORK_INTERFACE_CONFIGURATION - begins" << std::endl;

        // ========== LtePhyEnb ===========
        // ==== START initializeFeedbackComputation(); ====
        const char* name = "REAL";

        double targetBler = par("targetBler");  // default(0.001)
        double lambdaMinTh = par("lambdaMinTh");    // default(0.02)
        double lambdaMaxTh = par("lambdaMaxTh");    // default(0.2)
        double lambdaRatioTh = par("lambdaRatioTh");    // default(20)

        // compute feedback for the primary carrier only
        // TODO add support for feedback computation for all carriers

        // lteFeedbackComputation_ = new LteFeedbackComputationRealistic(
        //    targetBler, cellInfo_->getLambda(), lambdaMinTh, lambdaMaxTh,
        //    lambdaRatioTh, cellInfo_->getNumBands());

        lteFeedbackComputation_ = new LteFeedbackComputationRealistic(
            targetBler, cellInfo_->getLambda(), lambdaMinTh, lambdaMaxTh,
            lambdaRatioTh, cellInfo_->getPrimaryCarrierNumBands());

        EV << "GnbPhy::initialize - Feedback Computation \"" << name << "\" loaded." << endl;
        // ==== END initializeFeedbackComputation(); ====

        //check eNb type and set TX power
        if (cellInfo_->getEnbType() == MICRO_ENB)
            txPower_ = microTxPower_;
        else
            txPower_ = eNodeBtxPower_;

        // set TX direction
        std::string txDir = par("txDirection"); // default("OMNI")
        if (txDir.compare(txDirections[OMNI].txDirectionName)==0)
        {
            txDirection_ = OMNI;
        }
        else   // ANISOTROPIC
        {
            txDirection_ = ANISOTROPIC;

            // set TX angle
            txAngle_ = par("txAngle");  // default(0)
        }

        bdcUpdateInterval_ = cellInfo_->par("broadcastMessageInterval");    // default(1s)
        if (bdcUpdateInterval_ != 0 && par("enableHandover").boolValue()) {
            // self message provoking the generation of a broadcast message
            bdcStarter_ = new cMessage("bdcStarter");
            scheduleAt(NOW, bdcStarter_);
        }

        if (enableInitDebug_)
            std::cout << "GnbPhy::initialize - stage: INITSTAGE_NETWORK_INTERFACE_CONFIGURATION - ends" << std::endl;
    }
    else if (stage == inet::INITSTAGE_PHYSICAL_LAYER)
    {
        if (enableInitDebug_)
            std::cout << "GnbPhy::initialize - stage: INITSTAGE_PHYSICAL_LAYER - begins" << std::endl;

        // initializeChannelModel();
        // std::string moduleName = (strcmp(getFullName(), "nrPhy") == 0) ? "nrChannelModel" : "channelModel";
        std::string moduleName = "channelModel";
        primaryChannelModel_ = check_and_cast<LteChannelModel*>(getParentModule()->getSubmodule(moduleName.c_str(),0));
        // default("NRChannelModel_3GPP38_901")
        primaryChannelModel_->setPhy(this);
        double carrierFrequency = primaryChannelModel_->getCarrierFrequency();
        channelModel_[carrierFrequency] = primaryChannelModel_;

        int vectSize = primaryChannelModel_->getVectorSize();
        LteChannelModel* chanModel = NULL;
        for (int index=1; index<vectSize; index++)
        {
            chanModel = check_and_cast<LteChannelModel*>(getParentModule()->getSubmodule(moduleName.c_str(),index));
            chanModel->setPhy(this);
            carrierFrequency = chanModel->getCarrierFrequency();
            channelModel_[carrierFrequency] = chanModel;
        }

        if (enableInitDebug_)
            std::cout << "GnbPhy::initialize - stage: INITSTAGE_PHYSICAL_LAYER - ends" << std::endl;
    }
}

void GnbPhy::handleMessage(cMessage* msg)
{
    EV << "GnbPhy::handleMessage - new message received" << endl;

    if (msg->isSelfMessage())
    {
        handleSelfMessage(msg);
    }
    // AirFrame
    else if (msg->getArrivalGate()->getId() == radioInGate_)
    {
        handleAirFrame(msg);
    }

    // message from stack
    else if (msg->getArrivalGate()->getId() == upperGateIn_)
    {
        handleUpperMessage(msg);
    }
    // unknown message
    else
    {
        EV << "Unknown message received." << endl;
        delete msg;
    }
}

void GnbPhy::handleAirFrame(cMessage* msg)
{
    UserControlInfo* lteInfo = check_and_cast<UserControlInfo*>(msg->removeControlInfo());
    LteAirFrame* frame = static_cast<LteAirFrame*>(msg);

    EV << "GnbPhy::handleAirFrame - received new LteAirFrame with ID " << frame->getId() << " from channel" << endl;

    // handle broadcast packet sent by another eNB
    if (lteInfo->getFrameType() == HANDOVERPKT)
    {
        EV << "GnbPhy::handleAirFrame - received handover packet from another eNodeB. Ignore it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    // check if the air frame was sent on a correct carrier frequency
    double carrierFrequency = lteInfo->getCarrierFrequency();
    LteChannelModel* channelModel = getChannelModel(carrierFrequency);
    if (channelModel == NULL)
    {
        EV << "Received packet on carrier frequency not supported by this node. Delete it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    // Check if the frame is for us ( MacNodeId matches or - if this is a multicast communication - enrolled in multicast group)
    if (lteInfo->getDestId() != nodeId_)
    {
        EV << "ERROR: Frame is not for us. Delete it." << endl;
        EV << "Packet Type: " << phyFrameTypeToA((LtePhyFrameType)lteInfo->getFrameType()) << endl;
        EV << "Frame MacNodeId: " << lteInfo->getDestId() << endl;
        EV << "Local MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    if (lteInfo->getMulticastGroupId() != -1 && !(binder_->isInMulticastGroup(nodeId_, lteInfo->getMulticastGroupId())))
    {
        EV << "Frame is for a multicast group, but we do not belong to that group. Delete the frame." << endl;
        EV << "Packet Type: " << phyFrameTypeToA((LtePhyFrameType)lteInfo->getFrameType()) << endl;
        EV << "Frame MacNodeId: " << lteInfo->getDestId() << endl;
        EV << "Local MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    /*
     * This could happen if the ue associates with a new master while it has
     * already scheduled a packet for the old master: the packet is in the air
     * while the ue changes master.
     * Event timing:      TTI x: packet scheduled and sent by the UE (tx time = 1ms)
     *                     TTI x+0.1: ue changes master
     *                     TTI x+1: packet from UE arrives at the old master
     */
    if (!resAllocateMode_)
    {
        if (binder_->getNextHop(lteInfo->getSourceId()) != nodeId_)
        {
            EV << "WARNING: frame from a UE that is leaving this cell (handover): deleted " << endl;
            EV << "Source MacNodeId: " << lteInfo->getSourceId() << endl;
            EV << "Master MacNodeId: " << nodeId_ << endl;
            delete lteInfo;
            delete frame;
            return;
        }
    }

    connectedNodeId_ = lteInfo->getSourceId();

    int sourceId = binder_->getOmnetId(connectedNodeId_);
    int senderId = binder_->getOmnetId(lteInfo->getDestId());
    if(sourceId == 0 || senderId == 0)
    {
        // either source or destination have left the simulation
        delete msg;
        return;
    }

    //handle all control pkt
    if (handleControlPkt(lteInfo, frame))
        return; // If frame contain a control pkt no further action is needed

    /***
     * Only non-control packet and non-handover packet (i.e., only data packet) need to check the packet error
     */
    bool result = true;

    if (!resAllocateMode_)
    {
        // apply decider to received packet
        RemoteSet r = lteInfo->getUserTxParams()->readAntennaSet();
        if (r.size() > 1)
        {
            // Use DAS
            // Message from ue
            for (RemoteSet::iterator it = r.begin(); it != r.end(); it++)
            {
                EV << "GnbPhy::handleAirFrame - Receiving Packet from antenna " << (*it) << "\n";

                /*
                    * On eNodeB set the current position
                    * to the receiving das antenna
                    */
                //move.setStart(
                cc->setRadioPosition(myRadioRef, das_->getAntennaCoord(*it));

                RemoteUnitPhyData data;
                data.txPower = lteInfo->getTxPower();
                data.m = getCoord();
                frame->addRemoteUnitPhyDataVector(data);
            }
            result = channelModel->isErrorDas(frame, lteInfo);
        }
        else
        {
            result = channelModel->isError(frame, lteInfo);
        }
    }
    
    if (result)
        numAirFrameReceived_++;
    else
        numAirFrameNotReceived_++;

    EV << "Handled LteAirframe with ID " << frame->getId() << " with result "
       << (result ? "RECEIVED" : "NOT RECEIVED") << endl;

    auto pkt = check_and_cast<inet::Packet *>(frame->decapsulate());

    // here frame has to be destroyed since it is no more useful
    delete frame;

    // attach the decider result to the packet as control info
    lteInfo->setDeciderResult(result);
    auto lteInfoTag = pkt->addTagIfAbsent<UserControlInfo>();
    *lteInfoTag = *lteInfo;
    delete lteInfo;

    // send decapsulated message along with result control info to upperGateOut_
    send(pkt, upperGateOut_);

    if (getEnvir()->isGUI())
        updateDisplayString();
}

// do not achieve real override, as the original method is not defined as virtual
bool GnbPhy::handleControlPkt(UserControlInfo* lteinfo, LteAirFrame* frame)
{
    MacNodeId senderMacNodeId = lteinfo->getSourceId();
    if (binder_->getOmnetId(senderMacNodeId) == 0)
    {
        EV << "Sender (" << senderMacNodeId << ") does not exist anymore!" << std::endl;
        delete frame;
        return true;    // FIXME ? make sure that nodes that left the simulation do not send
    }
    if (lteinfo->getFrameType() == HANDOVERPKT)
    {
        EV << "GnbPhy::handleControlPkt - airFrame type: HANDOVERPKT, delete" << endl;

        // handover broadcast frames must not be relayed or processed by eNB
        delete frame;
        return true;
    }
    // send H-ARQ feedback up
    if (lteinfo->getFrameType() == HARQPKT || lteinfo->getFrameType() == RACPKT)
    {
        if (lteinfo->getFrameType() == HARQPKT)
            EV << "GnbPhy::handleControlPkt - airFrame type: HARQPKT" << endl;
        else
            EV << "GnbPhy::handleControlPkt - airFrame type: RACPKT" << endl;

        //handleControlMsg(frame, lteinfo);
        auto pkt = check_and_cast<inet::Packet *>(frame->decapsulate());
        delete frame;
        *(pkt->addTagIfAbsent<UserControlInfo>()) = *lteinfo;
        delete lteinfo;
        send(pkt, upperGateOut_);

        return true;
    }
    //handle feedback pkt
    if (lteinfo->getFrameType() == FEEDBACKPKT)
    {
        EV << "GnbPhy::handleControlPkt - airFrame type: FEEDBACKPKT" << endl;

        handleFeedbackPkt(lteinfo, frame);
        delete frame;
        return true;
    }
    return false;
}

// do not achieve real override, as the original method is not defined as virtual
void GnbPhy::handleFeedbackPkt(UserControlInfo* lteinfo, LteAirFrame *frame)
{
    EV << "GnbPhy::handleFeedbackPkt - Handled Feedback Packet with ID " << frame->getId() << endl;
    auto pktAux = check_and_cast<Packet *>(frame->decapsulate());
    auto header =  pktAux->peekAtFront<LteFeedbackPkt>();

    *(pktAux->addTagIfAbsent<UserControlInfo>()) = *lteinfo;

    // if feedback was generated by dummy phy we can send up to mac else nodeb should generate the "real" feedback
    if (lteinfo->feedbackReq.request)
    {
        requestFeedback(lteinfo, frame, pktAux);

        // DEBUG
        bool debug = false;
        if( debug )
        {
            LteFeedbackDoubleVector::iterator it;
            LteFeedbackVector::iterator jt;
            LteFeedbackDoubleVector vec = header->getLteFeedbackDoubleVectorDl();
            for (it = vec.begin(); it != vec.end(); ++it)
            {
                for (jt = it->begin(); jt != it->end(); ++jt)
                {
                    MacNodeId id = lteinfo->getSourceId();
                    EV << endl << "Node:" << id << endl;
                    TxMode t = jt->getTxMode();
                    EV << "TXMODE: " << txModeToA(t) << endl;
                    if (jt->hasBandCqi())
                    {
                        std::vector<CqiVector> vec = jt->getBandCqi();
                        std::vector<CqiVector>::iterator kt;
                        CqiVector::iterator ht;
                        int i;
                        for (kt = vec.begin(); kt != vec.end(); ++kt)
                        {
                            for (i = 0, ht = kt->begin(); ht != kt->end();
                                ++ht, i++)
                            EV << "Banda " << i << " Cqi " << *ht << endl;
                        }
                    }
                    else if (jt->hasWbCqi())
                    {
                        CqiVector v = jt->getWbCqi();
                        CqiVector::iterator ht = v.begin();
                        for (; ht != v.end(); ++ht)
                        EV << "wb cqi " << *ht << endl;
                    }
                    if (jt->hasRankIndicator())
                    {
                        EV << "Rank " << jt->getRankIndicator() << endl;
                    }
                }
            }
        }
    }
    delete lteinfo;
    // send decapsulated message along with result control info to upperGateOut_
    send(pktAux, upperGateOut_);
}


void GnbPhy::requestFeedback(UserControlInfo* lteinfo, LteAirFrame* frame, Packet* pktAux)
{
    EV << NOW << " GnbPhy::requestFeedback " << endl;

    LteFeedbackDoubleVector fb;

    // select the correct channel model according to the carrier freq
    LteChannelModel* channelModel = getChannelModel(lteinfo->getCarrierFrequency());

    auto header = pktAux->removeAtFront<LteFeedbackPkt>();

    //get UE Position
    Coord sendersPos = lteinfo->getCoord();
    cellInfo_->setUePosition(lteinfo->getSourceId(), sendersPos);

    //Apply analog model (pathloss)
    //Get snr for UL direction
    std::vector<double> snr;
    if (channelModel != NULL)
        snr = channelModel->getSINR(frame, lteinfo);
    else
        throw cRuntimeError("GnbPhy::requestFeedback - channelModel is null pointer. Abort");
    FeedbackRequest req = lteinfo->feedbackReq;
    //Feedback computation
    fb.clear();
    //get number of RU
    int nRus = cellInfo_->getNumRus();
    TxMode txmode = req.txMode;
    FeedbackType type = req.type;
    RbAllocationType rbtype = req.rbAllocationType;
    std::map<Remote, int> antennaCws = cellInfo_->getAntennaCws();
    unsigned int numPreferredBand = cellInfo_->getNumPreferredBands();
    Direction dir = UL;
    while (dir != UNKNOWN_DIRECTION)
    {
        //for each RU is called the computation feedback function
        if (req.genType == IDEAL)
        {
            fb = lteFeedbackComputation_->computeFeedback(type, rbtype, txmode,
                antennaCws, numPreferredBand, IDEAL, nRus, snr,
                lteinfo->getSourceId());
        }
        else if (req.genType == REAL)
        {
            RemoteSet::iterator it;
            fb.resize(das_->getReportingSet().size());
            for (it = das_->getReportingSet().begin();
                it != das_->getReportingSet().end(); ++it)
            {
                fb[(*it)].resize((int) txmode);
                fb[(*it)][(int) txmode] =
                lteFeedbackComputation_->computeFeedback(*it, txmode,
                    type, rbtype, antennaCws[*it], numPreferredBand,
                    REAL, nRus, snr, lteinfo->getSourceId());
            }
        }
        // the reports are computed only for the antenna in the reporting set
        else if (req.genType == DAS_AWARE)
        {
            RemoteSet::iterator it;
            fb.resize(das_->getReportingSet().size());
            for (it = das_->getReportingSet().begin();
                it != das_->getReportingSet().end(); ++it)
            {
                fb[(*it)] = lteFeedbackComputation_->computeFeedback(*it, type,
                    rbtype, txmode, antennaCws[*it], numPreferredBand,
                    DAS_AWARE, nRus, snr, lteinfo->getSourceId());
            }
        }
        if (dir == UL)
        {
            header->setLteFeedbackDoubleVectorUl(fb);
            //Prepare  parameters for next loop iteration - in order to compute SNR in DL
            lteinfo->setTxPower(txPower_);
            lteinfo->setDirection(DL);
            //Get snr for DL direction
            if (channelModel != NULL)
                snr = channelModel->getSINR(frame, lteinfo);
            else
                throw cRuntimeError("GnbPhy::requestFeedback - channelModel is null pointer. Abort");

            dir = DL;
        }
        else if (dir == DL)
        {
            header->setLteFeedbackDoubleVectorDl(fb);

            if (enableD2DCqiReporting_)
            {
                // compute D2D feedback for all possible peering UEs
                std::vector<UeInfo*>* ueList = binder_->getUeList();
                std::vector<UeInfo*>::iterator it = ueList->begin();
                for (; it != ueList->end(); ++it)
                {
                    MacNodeId peerId = (*it)->id;
                    if (peerId != lteinfo->getSourceId() && binder_->getD2DCapability(lteinfo->getSourceId(), peerId) && binder_->getNextHop(peerId) == nodeId_)
                    {
                         // the source UE might communicate with this peer using D2D, so compute feedback (only in-cell D2D)

                         // retrieve the position of the peer
                         Coord peerCoord = (*it)->phy->getCoord();

                         // get SINR for this link
                         if (channelModel != NULL)
                             snr = channelModel->getSINR_D2D(frame, lteinfo, peerId, peerCoord, nodeId_);
                         else
                             throw cRuntimeError("GnbPhy::requestFeedback - channelModel is null pointer. Abort");

                         // compute the feedback for this link
                         fb = lteFeedbackComputation_->computeFeedback(type, rbtype, txmode,
                                 antennaCws, numPreferredBand, IDEAL, nRus, snr,
                                 lteinfo->getSourceId());

                         header->setLteFeedbackDoubleVectorD2D(peerId, fb);
                    }
                }
            }
            dir = UNKNOWN_DIRECTION;
        }
    }
    EV << "GnbPhy::requestFeedback - Pisa Feedback Generated for nodeId: "
       << nodeId_ << " with generator type "
       << fbGeneratorTypeToA(req.genType) << " Fb size: " << fb.size()
       << " Carrier: " << lteinfo->getCarrierFrequency() << endl;

    pktAux->insertAtFront(header);
}


void GnbPhy::handleUpperMessage(cMessage* msg)
{
    EV << "GnbPhy::handleUpperMessage - message from stack" << endl;

    auto pkt = check_and_cast<inet::Packet *>(msg);
    auto lteInfo = pkt->removeTag<UserControlInfo>();

    LteAirFrame* frame = nullptr;

    if (lteInfo->getFrameType() == HARQPKT
        || lteInfo->getFrameType() == GRANTPKT
        || lteInfo->getFrameType() == RACPKT
        || lteInfo->getFrameType() == D2DMODESWITCHPKT)
    {
        frame = new LteAirFrame("harqFeedback-grant");
        frame->setSchedulingPriority(airFramePriority_-1);
    }
    else
    {
        // create LteAirFrame and encapsulate the received packet
        frame = new LteAirFrame("airframe");
        frame->setSchedulingPriority(airFramePriority_);
    }

    frame->encapsulate(check_and_cast<cPacket*>(msg));

    // initialize frame fields
    // if (lteInfo->getFrameType() == D2DMODESWITCHPKT)
    //     frame->setSchedulingPriority(-1);
    // else
    //     frame->setSchedulingPriority(airFramePriority_);

    // set transmission duration according to the numerology
    NumerologyIndex numerologyIndex = binder_->getNumerologyIndexFromCarrierFreq(lteInfo->getCarrierFrequency());
    double slotDuration = binder_->getSlotDurationFromNumerologyIndex(numerologyIndex);
    frame->setDuration(slotDuration);

    // set current position
    lteInfo->setCoord(getCoord());
    lteInfo->setTxPower(txPower_);
    frame->setControlInfo(lteInfo.get()->dup());

    EV << "GnbPhy::handleUpperMessage - " << nodeTypeToA(nodeType_) << " with id " << nodeId_
       << " sending message to the air channel. Dest=" << lteInfo->getDestId() << endl;
    sendUnicast(frame);
}

void GnbPhy::sendBroadcast(LteAirFrame *airFrame)
{
    EV << NOW << " GnbPhy::sendBroadcast - broadcast frame." << endl;

    // delegate the ChannelControl to send the airframe
    sendToChannel(airFrame);
}

void GnbPhy::sendMulticast(LteAirFrame *frame)
{
    UserControlInfo *ci = check_and_cast<UserControlInfo *>(frame->getControlInfo());

    // get the group Id
    int32_t groupId = ci->getMulticastGroupId();
    if (groupId < 0)
        throw cRuntimeError("GnbPhy::sendMulticast - Error. Group ID %d is not valid.", groupId);

    // send the frame to nodes belonging to the multicast group only
    std::map<int, OmnetId>::const_iterator nodeIt = binder_->getNodeIdListBegin();
    for (; nodeIt != binder_->getNodeIdListEnd(); ++nodeIt)
    {
        MacNodeId destId = nodeIt->first;

        // if the node in the list does not use the same LTE/NR technology of this PHY module, skip it
        if (isNrUe(destId) != isNr_)
            continue;

        if (destId != nodeId_ && binder_->isInMulticastGroup(nodeIt->first, groupId))
        {
            EV << NOW << " GnbPhy::sendMulticast - node " << destId << " is in the multicast group"<< endl;

            // get a pointer to receiving module
            cModule *receiver = getSimulation()->getModule(nodeIt->second);
            LtePhyBase * recvPhy;
            double dist;

            if( enableMulticastD2DRangeCheck_ )
            {
                // get the correct PHY layer module
                recvPhy =  (isNrUe(destId)) ? check_and_cast<LtePhyBase *>(receiver->getSubmodule("cellularNic")->getSubmodule("nrPhy"))
                                  : check_and_cast<LtePhyBase *>(receiver->getSubmodule("cellularNic")->getSubmodule("phy"));

                dist = recvPhy->getCoord().distance(getCoord());

                if( dist > multicastD2DRange_ )
                {
                    EV << NOW << " GnbPhy::sendMulticast - node too far (" << dist << " > " << multicastD2DRange_ << ". skipping transmission" << endl;
                    continue;
                }
            }

            EV << NOW << " GnbPhy::sendMulticast - sending frame to node " << destId << endl;

            sendDirect(frame->dup(), 0, frame->getDuration(), receiver, getReceiverGateIndex(receiver, isNrUe(destId)));
        }
    }

    // delete the original frame
    delete frame;
}

void GnbPhy::sendUnicast(LteAirFrame *frame)
{
    UserControlInfo *ci = check_and_cast<UserControlInfo *>(
        frame->getControlInfo());
    // dest MacNodeId from control info
    MacNodeId dest = ci->getDestId();

    EV << NOW << " GnbPhy::sendUnicast - sending frame to macNode " << dest << endl;

    // destination node (UE or ENODEB) omnet id
    try {
        binder_->getOmnetId(dest);
    } catch (std::out_of_range& e) {
        delete frame;
        return;         // make sure that nodes that left the simulation do not send
    }
    OmnetId destOmnetId = binder_->getOmnetId(dest);
    if (destOmnetId == 0){
        // destination node has left the simulation
        delete frame;
        return;
    }
    // get a pointer to receiving module
    cModule *receiver = getSimulation()->getModule(destOmnetId);

    sendDirect(frame, 0, frame->getDuration(), receiver, getReceiverGateIndex(receiver, isNrUe(dest)));
}





