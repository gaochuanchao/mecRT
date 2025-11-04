//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// a single layer of NRPhyUe module in simu5g
// simulate the PHY stack of the NIC module of gNB
// LtePhyBase --> LtePhyUe --> LtePhyUeD2D --> NRPhyUe
//
#include "mecrt/nic/phy/UePhy.h"
#include "stack/phy/packet/LteFeedbackPkt.h"
#include "mecrt/packets/nic/VecDataInfo_m.h"
#include "mecrt/mobility/MecMobility.h"
#include "mecrt/nic/mac/GnbMac.h"

Define_Module(UePhy);

UePhy::UePhy()
{
    handoverStarter_ = nullptr;
    d2dDecodingTimer_ = nullptr;
    enableInitDebug_ = false;
    das_ = nullptr;
}

UePhy::~UePhy()
{
    if (enableInitDebug_)
        std::cout << "UePhy::~UePhy - destroying PHY protocol\n";

    if (handoverStarter_)
    {
        cancelAndDelete(handoverStarter_);
        handoverStarter_ = nullptr;
    }
    if (das_)
    {
        delete das_;
        das_ = nullptr;
    }
    if (d2dDecodingTimer_)
    {
        cancelAndDelete(d2dDecodingTimer_);
        d2dDecodingTimer_ = nullptr;
    }

    if (enableInitDebug_)
        std::cout << "UePhy::~UePhy - destroying PHY protocol done!\n";
}

void UePhy::initialize(int stage)
{
    ChannelAccess::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL)
    {
        if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
        if (enableInitDebug_)
            std::cout << "UePhy::initialize - stage: INITSTAGE_LOCAL - begins" << std::endl;

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

        // ========= LtePhyUe ==========
        nodeType_ = UE;
        useBattery_ = false;  // disabled
        enableHandover_ = par("enableHandover");    // default(false)
        handoverLatency_ = par("handoverLatency").doubleValue();    // default(0.05s)
        handoverDetachment_ = handoverLatency_/2.0;                      // TODO: make this configurable from NED
        handoverAttachment_ = handoverLatency_ - handoverDetachment_;
        dynamicCellAssociation_ = par("dynamicCellAssociation");    // default(false)
        // configurable minimum threshold RSSI for attaching to an eNB
        if (par("minRssiDefault").boolValue())      // default(true)
            minRssi_ = binder_->phyPisaData.minSnr();
        else
            minRssi_ = par("minRssi").doubleValue();   // default(-99.0dB), meaningful only if minRssiDefault==false

        currentMasterRssi_ = -999.0;
        candidateMasterRssi_ = -999.0;
        hysteresisTh_ = 0;
        hysteresisFactor_ = 10;
        handoverDelta_ = 0.00001;

        dasRssiThreshold_ = 1.0e-5;
        das_ = new DasFilter(this, binder_, nullptr, dasRssiThreshold_);

        servingCell_ = registerSignal("servingCell");
        averageCqiDl_ = registerSignal("averageCqiDl");
        averageCqiUl_ = registerSignal("averageCqiUl");

        if (!hasListeners(averageCqiDl_))
            error("no phy listeners");

        WATCH(nodeType_);
        WATCH(masterId_);
        WATCH(candidateMasterId_);
        WATCH(dasRssiThreshold_);
        WATCH(currentMasterRssi_);
        WATCH(candidateMasterRssi_);
        WATCH(hysteresisTh_);
        WATCH(hysteresisFactor_);
        WATCH(handoverDelta_);

        // ========= LtePhyUeD2D ==========
        averageCqiD2D_ = registerSignal("averageCqiD2D");
        d2dTxPower_ = par("d2dTxPower");    // default(26)
        d2dMulticastEnableCaptureEffect_ = par("d2dMulticastCaptureEffect");    // default(true)
        d2dDecodingTimer_ = nullptr;

        // ========= NRPhyUe ==========
        isNr_ = (strcmp(getFullName(),"nrPhy") == 0) ? true : false;
        if (isNr_)
            otherPhy_ = check_and_cast<NRPhyUe*>(getParentModule()->getSubmodule("phy"));
        else
            otherPhy_ = check_and_cast<NRPhyUe*>(getParentModule()->getSubmodule("nrPhy"));

        resAllocateMode_ = par("resAllocateMode");
        offloadPower_ = par("offloadPower");    // default(2210) mW
        srsDistanceCheck_ = par("srsDistanceCheck");  // default(false)
        srsDistance_ = par("srsDistance");  // default(600m)

        rsuSet_.clear();
        grantedRsus_.clear();

        WATCH_SET(grantedRsus_);
        WATCH(resAllocateMode_);

        if (enableInitDebug_)
            std::cout << "UePhy::initialize - stage: INITSTAGE_LOCAL - ends" << std::endl;
    }
    else if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT)
    {
        if (enableInitDebug_)
            std::cout << "UePhy::initialize - stage: INITSTAGE_PHYSICAL_ENVIRONMENT - begins" << std::endl;

        // ========= LtePhyUe ==========
        if (useBattery_)
        {
            // TODO register the device to the battery with two accounts, e.g. 0=tx and 1=rx
            // it only affects statistics
            //registerWithBattery("LtePhy", 2);
//            txAmount_ = par("batteryTxCurrentAmount");
//            rxAmount_ = par("batteryRxCurrentAmount");
//
//            WATCH(txAmount_);
//            WATCH(rxAmount_);
        }

        txPower_ = ueTxPower_;
        lastFeedback_ = 0;
        handoverStarter_ = new cMessage("handoverStarter");

        if (isNr_)
        {
            mac_ = check_and_cast<LteMacUe *>(
                getParentModule()-> // nic
                getSubmodule("nrMac"));
            rlcUm_ = check_and_cast<LteRlcUm*>(
                getParentModule()-> // nic
                getSubmodule("nrRlc")->
                    getSubmodule("um"));
        }
        else
        {
            mac_ = check_and_cast<LteMacUe *>(
                getParentModule()-> // nic
                getSubmodule("mac"));
            rlcUm_ = check_and_cast<LteRlcUm*>(
                getParentModule()-> // nic
                getSubmodule("rlc")->
                    getSubmodule("um"));
        }
        pdcp_ = check_and_cast<LtePdcpRrcBase*>(
            getParentModule()-> // nic
            getSubmodule("pdcpRrc"));

        // get local id
        if (isNr_)
            nodeId_ = getAncestorPar("nrMacNodeId");
        else
            nodeId_ = getAncestorPar("macNodeId");
        EV << "Local MacNodeId: " << nodeId_ << endl;

        // get the reference to band manager
        bandManager_ = check_and_cast<BandManager*>(getSimulation()->getModuleByPath("bandManager"));
        bandManager_->addUePhy(nodeId_, this, offloadPower_);

        if (enableInitDebug_)
            std::cout << "UePhy::initialize - stage: INITSTAGE_PHYSICAL_ENVIRONMENT - ends" << std::endl;
    }
    else if (stage == inet::INITSTAGE_PHYSICAL_LAYER)
    {
        if (enableInitDebug_)
            std::cout << "UePhy::initialize - stage: INITSTAGE_PHYSICAL_LAYER - begins" << std::endl;

        // initializeChannelModel();
        // "nrChannelModel" - default("NRChannelModel_3GPP38_901")
        // "channelModel" - default("LteRealisticChannelModel")
        std::string moduleName = (strcmp(getFullName(), "nrPhy") == 0) ? "nrChannelModel" : "channelModel";
        primaryChannelModel_ = check_and_cast<LteChannelModel*>(getParentModule()->getSubmodule(moduleName.c_str(),0));
        primaryChannelModel_->setPhy(this);
        double carrierFrequency = primaryChannelModel_->getCarrierFrequency();
        unsigned int numerologyIndex = primaryChannelModel_->getNumerologyIndex();
        channelModel_[carrierFrequency] = primaryChannelModel_;
        binder_->registerCarrierUe(carrierFrequency, numerologyIndex, nodeId_);

        int vectSize = primaryChannelModel_->getVectorSize();
        LteChannelModel* chanModel = NULL;
        for (int index=1; index<vectSize; index++)
        {
            chanModel = check_and_cast<LteChannelModel*>(getParentModule()->getSubmodule(moduleName.c_str(),index));
            chanModel->setPhy(this);
            carrierFrequency = chanModel->getCarrierFrequency();
            numerologyIndex = chanModel->getNumerologyIndex();
            channelModel_[carrierFrequency] = chanModel;
            binder_->registerCarrierUe(carrierFrequency, numerologyIndex, nodeId_);
        }

        // ========= LtePhyUe ==========
        // get serving cell from configuration
        // TODO find a more elegant way
        if (isNr_)
            masterId_ = getAncestorPar("nrMasterId");   // the macNodeId of the corresponding gNB
        else
            masterId_ = getAncestorPar("masterId");     // the macNodeId of the corresponding eNB
        candidateMasterId_ = masterId_;

        // find the best candidate master cell
        if (dynamicCellAssociation_)
        {
            // this is a fictitious frame that needs to compute the SINR
            LteAirFrame *frame = new LteAirFrame("cellSelectionFrame");
            UserControlInfo *cInfo = new UserControlInfo();

            // get the list of all eNodeBs in the network
            std::vector<EnbInfo *>* gnbList = binder_->getEnbList();
            for (auto it = gnbList->begin(); it != gnbList->end(); ++it)
            {
                // the NR phy layer only checks signal from gNBs
                if (isNr_ && (*it)->nodeType != GNODEB)
                    continue;

                // the LTE phy layer only checks signal from eNBs
                if (!isNr_ && (*it)->nodeType != ENODEB)
                    continue;

                MacNodeId cellId = (*it)->id;   // refers to the base station macNodeId
                LtePhyBase* cellPhy = check_and_cast<LtePhyBase*>((*it)->eNodeB->getSubmodule("cellularNic")->getSubmodule("phy"));
                double cellTxPower = cellPhy->getTxPwr();
                Coord cellPos = cellPhy->getCoord();
                rsuSet_.insert(cellId);

                if (resAllocateMode_)
                    binder_->registerNextHop(cellId, nodeId_);

                // check whether the BS supports the carrier frequency used by the UE
                double ueCarrierFrequency = primaryChannelModel_->getCarrierFrequency();
                LteChannelModel* cellChannelModel = cellPhy->getChannelModel(ueCarrierFrequency);
                if (cellChannelModel == nullptr)
                    continue;

                // build a control info
                cInfo->setSourceId(cellId);
                cInfo->setTxPower(cellTxPower);
                cInfo->setCoord(cellPos);
                cInfo->setFrameType(BROADCASTPKT);
                cInfo->setDirection(DL);

                // get RSSI from the BS
                double rssi = 0;
                std::vector<double> rssiV = primaryChannelModel_->getRSRP(frame, cInfo);
                for (auto jt = rssiV.begin(); jt != rssiV.end(); ++jt)
                    rssi += *jt;
                rssi /= rssiV.size();   // compute the mean over all RBs

                EV << "LtePhyUe::initialize - RSSI from cell " << cellId << ": " << rssi << " dB (current candidate cell " << candidateMasterId_ << ": " << candidateMasterRssi_ << " dB)" << endl;

                if (rssi > candidateMasterRssi_)
                {
                    candidateMasterId_ = cellId;
                    candidateMasterRssi_ = rssi;
                }
            }
            delete cInfo;
            delete frame;

            // binder calls
            // if dynamicCellAssociation selected a different master
            if (candidateMasterId_ != 0 && candidateMasterId_ != masterId_)
            {
                if (!resAllocateMode_)
                    binder_->unregisterNextHop(masterId_, nodeId_);
                
                binder_->registerNextHop(candidateMasterId_, nodeId_);
            }
            masterId_ = candidateMasterId_;
            // set serving cell
            if (isNr_)
                getAncestorPar("nrMasterId").setIntValue(masterId_);
            else
                getAncestorPar("masterId").setIntValue(masterId_);
            currentMasterRssi_ = candidateMasterRssi_;
            updateHysteresisTh(candidateMasterRssi_);
        }

        EV << "UePhy::initialize - Attaching to eNodeB " << masterId_ << endl;

        das_->setMasterRuSet(masterId_);
        emit(servingCell_, (long)masterId_);



        if (enableInitDebug_)
            std::cout << "UePhy::initialize - stage: INITSTAGE_PHYSICAL_LAYER - ends" << std::endl;
    }
    else if (stage == inet::INITSTAGE_NETWORK_CONFIGURATION)
    {
        if (enableInitDebug_)
            std::cout << "UePhy::initialize - stage: INITSTAGE_NETWORK_CONFIGURATION - begins" << std::endl;

        // ========= LtePhyUe ==========
        // get cellInfo at this stage because the next hop of the node is registered
        // in the IP2Nic module at the INITSTAGE_NETWORK_LAYER
        if (masterId_ > 0)
        {
            if (resAllocateMode_)
            {
                std::vector<EnbInfo *>* gnbList = binder_->getEnbList();
                for (auto it = gnbList->begin(); it != gnbList->end(); ++it)
                {
                    MacNodeId cellId = (*it)->id;   // refers to the base station macNodeId
                    OmnetId omnetid = binder_->getOmnetId(cellId);
                    omnetpp::cModule* module = getSimulation()->getModule(omnetid);
                    CellInfo * cellInfo = module? check_and_cast<CellInfo*>(module->getSubmodule("cellInfo")) : nullptr;

                    if (cellId == masterId_)
                        cellInfo_ = cellInfo;

                    int index = intuniform(0, binder_->phyPisaData.maxChannel() - 1);
                    if (cellInfo != NULL)
                    {
                        cellInfo->lambdaInit(nodeId_, index);
                        cellInfo->channelUpdate(nodeId_, intuniform(1, binder_->phyPisaData.maxChannel2()));
                    }
                }
            }
            else
            {
                cellInfo_ = getCellInfo(nodeId_);
                int index = intuniform(0, binder_->phyPisaData.maxChannel() - 1);
                if (cellInfo_ != NULL)
                {
                    cellInfo_->lambdaInit(nodeId_, index);
                    cellInfo_->channelUpdate(nodeId_, intuniform(1, binder_->phyPisaData.maxChannel2()));
                }
            }
        }
        else
            cellInfo_ = nullptr;

        if (enableInitDebug_)
            std::cout << "UePhy::initialize - stage: INITSTAGE_NETWORK_CONFIGURATION - ends" << std::endl;
    }
    else if (stage == inet::INITSTAGE_LAST)
    {
        if (enableInitDebug_)
            std::cout << "UePhy::initialize - stage: INITSTAGE_LAST - begins" << std::endl;

        int fbTtiCount = getParentModule()->getSubmodule("nrDlFbGen")->par("fbPeriod");
        fbPeriod_ = fbTtiCount * TTI;   // convert to seconds

        if (enableInitDebug_)
            std::cout << "UePhy::initialize - stage: INITSTAGE_LAST - ends" << std::endl;
    }
}


void UePhy::handleMessage(cMessage* msg)
{
    EV << "UePhy::handleMessage - new message received" << endl;

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


void UePhy::updateMasterNode()
{
    // check whether the current master node is still active
    NodeInfo* nodeInfo = check_and_cast_nullable<NodeInfo*>(binder_->getModuleByMacNodeId(masterId_)->getSubmodule("nodeInfo"));
    if (!nodeInfo)  
        return;
    else if (nodeInfo->isNodeActive())  // if the master node is active, do nothing
        return;

    EV << "UePhy::updateMasterNode - master node " << masterId_ << " is down, need to update master node" << endl;
    // select the closest node as the new master node
    MacNodeId candidateNode = 0;
    double minDist = 1.0e+10;
    // get the list of all eNodeBs in the network
    std::vector<EnbInfo *>* gnbList = binder_->getEnbList();
    for (auto it = gnbList->begin(); it != gnbList->end(); ++it)
    {
        // the NR phy layer only checks signal from gNBs
        if (isNr_ && (*it)->nodeType != GNODEB)
            continue;

        // the LTE phy layer only checks signal from eNBs
        if (!isNr_ && (*it)->nodeType != ENODEB)
            continue;

        GnbMac* nodeMac = check_and_cast_nullable<GnbMac*>((*it)->mac);
        if (nodeMac && !nodeMac->isNicDisabled())
        {
            LtePhyBase* nodePhy = (*it)->phy;
            double dist = nodePhy->getCoord().distance(getCoord());
            if (dist < minDist)
            {
                minDist = dist;
                candidateNode = (*it)->id;
            }
        }
    }

    if (candidateNode != 0)
    {
        EV << "UePhy::updateMasterNode - new master node selected: " << candidateNode << endl;
        // update masterId_
        masterId_ = candidateNode;
        binder_->registerNextHop(masterId_, nodeId_);
        das_->setMasterRuSet(masterId_);
    }
}


void UePhy::sendFeedback(LteFeedbackDoubleVector fbDl, LteFeedbackDoubleVector fbUl, FeedbackRequest req)
{
    Enter_Method("SendFeedback");
    EV << "UePhy::sendFeedback - feedback from Feedback Generator" << endl;

    //Create a feedback packet
    auto fbPkt = makeShared<LteFeedbackPkt>();
    //Set the feedback
    fbPkt->setLteFeedbackDoubleVectorDl(fbDl);
    fbPkt->setLteFeedbackDoubleVectorDl(fbUl);
    fbPkt->setSourceNodeId(nodeId_);

    auto pkt = new Packet("feedback_pkt");
    pkt->insertAtFront(fbPkt);

    UserControlInfo* uinfo = new UserControlInfo();
    uinfo->setSourceId(nodeId_);
    uinfo->setDestId(masterId_);
    uinfo->setFrameType(FEEDBACKPKT);
    uinfo->setIsCorruptible(false);
    // create LteAirFrame and encapsulate a feedback packet
    LteAirFrame* frame = new LteAirFrame("feedback_pkt");
    frame->encapsulate(check_and_cast<cPacket*>(pkt));
    uinfo->feedbackReq = req;
    uinfo->setDirection(UL);
    simtime_t signalLength = TTI;
    uinfo->setTxPower(txPower_);
    uinfo->setD2dTxPower(d2dTxPower_);
    // initialize frame fields

    frame->setSchedulingPriority(airFramePriority_-1);
    frame->setDuration(signalLength);

    uinfo->setCoord(getCoord());

    //TODO access speed data Update channel index
//    if (coherenceTime(move.getSpeed())<(NOW-lastFeedback_)){
//        cellInfo_->channelIncrease(nodeId_);
//        cellInfo_->lambdaIncrease(nodeId_,1);
//    }
    lastFeedback_ = NOW;

    // send one feedback packet for each carrier
    std::map<double, LteChannelModel*>::iterator cit = channelModel_.begin();
    for (; cit != channelModel_.end(); ++cit)
    {
        double carrierFrequency = cit->first;

        /***
         * when the next scheduling is going to start, broadcast feedback to the air channel (to all RSUs)
         * 5 TTI is to ensure that the last broadcast feedback is not too close to the scheduling time such that 
         * it has enough time to be sent to the scheduler.
         */
        EV << "UePhy::sendFeedback - NOW: " << NOW << ", fbPeriod_: " << fbPeriod_ 
            << ", NEXT_SCHEDULING_TIME: " << NEXT_SCHEDULING_TIME << endl;
        if ((NOW + fbPeriod_ + 5 * TTI >= NEXT_SCHEDULING_TIME) && (NOW + 5 * TTI <= NEXT_SCHEDULING_TIME))     
        {
            EV << "UePhy::sendFeedback - broadcast feedback to the air channel for carrier " << carrierFrequency << endl;

            /***
             * LteChannelControl: max interference distance:14057.7m
             * This has the same underlying principle as the sendBroadcast: based on sendDirect()
             * in sendBroadcast(), the airFrame is sent to neighbors (gNB within the max interference distance)
             */
            for (MacNodeId destId : rsuSet_)
            {
                // compute the distance to the RSU
                cModule *receiver = getSimulation()->getModule(binder_->getOmnetId(destId));
                LtePhyBase *recvPhy;
                // get the correct PHY layer module
                recvPhy = (isNrUe(destId)) ? check_and_cast<LtePhyBase *>(receiver->getSubmodule("cellularNic")->getSubmodule("nrPhy"))
                                            : check_and_cast<LtePhyBase *>(receiver->getSubmodule("cellularNic")->getSubmodule("phy"));
                double dist = recvPhy->getCoord().distance(getCoord());
                
                if (srsDistanceCheck_ && (dist > srsDistance_) && grantedRsus_.find(destId) == grantedRsus_.end())
                {
                    // if the RSU is too far and no service currently running on it, skip it
                    // this is to avoid sending feedback to RSUs that are not in the range of SRS
                    // and thus cannot receive the feedback

                    EV << "UePhy::sendFeedback - RSU " << destId << " is too far (" << dist << " > " << srsDistance_ << "), skipping transmission" << endl;
                    continue;   // skip this RSU
                }

                LteAirFrame* carrierFrame = frame->dup();
                UserControlInfo* carrierInfo = uinfo->dup();
                carrierInfo->setCarrierFrequency(carrierFrequency);
                carrierInfo->setDestId(destId);
                carrierInfo->setIsBroadcast(true);
                carrierFrame->setControlInfo(carrierInfo);

                EV << "UePhy::sendFeedback - " << nodeTypeToA(nodeType_) << " with id "
                    << nodeId_ << " sending feedback to RSU " << destId << endl;
                sendUnicast(carrierFrame);
            }
        }
        else
        {
            if (grantedRsus_.empty())
            {
                EV << "UePhy::sendFeedback - no granted RSUs, delete the feedback packet " << endl;
            }
            else
            {
                EV << "UePhy::sendFeedback - send feedback to the granted RSUs" << endl;
                // send feedback to the granted node
                for (MacNodeId destId : grantedRsus_)
                {
                    LteAirFrame* carrierFrame = frame->dup();
                    UserControlInfo* carrierInfo = uinfo->dup();
                    carrierInfo->setCarrierFrequency(carrierFrequency);
                    carrierInfo->setDestId(destId);
                    carrierFrame->setControlInfo(carrierInfo);

                    EV << "UePhy::sendFeedback - " << nodeTypeToA(nodeType_) << " with id "
                        << nodeId_ << " sending feedback to RSU " << destId << endl;
                    sendUnicast(carrierFrame);
                }
            }
        }
    }

    delete frame;
    delete uinfo;
}

void UePhy::sendBroadcast(LteAirFrame *airFrame)
{
    EV << NOW << " UePhy::sendBroadcast - broadcast airframe."<< endl;
    // delegate the ChannelControl to send the airframe
    sendToChannel(airFrame);
}

void UePhy::sendMulticast(LteAirFrame *frame)
{
    UserControlInfo *ci = check_and_cast<UserControlInfo *>(frame->getControlInfo());

    // get the group Id
    int32_t groupId = ci->getMulticastGroupId();
    if (groupId < 0)
        throw cRuntimeError("UePhy::sendMulticast - Error. Group ID %d is not valid.", groupId);

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
            EV << NOW << " UePhy::sendMulticast - node " << destId << " is in the multicast group"<< endl;

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
                    EV << NOW << " UePhy::sendMulticast - node too far (" << dist << " > " << multicastD2DRange_ << ". skipping transmission" << endl;
                    continue;
                }
            }

            EV << NOW << " UePhy::sendMulticast - sending frame to node " << destId << endl;

            sendDirect(frame->dup(), 0, frame->getDuration(), receiver, getReceiverGateIndex(receiver, isNrUe(destId)));
        }
    }

    // delete the original frame
    delete frame;
}

void UePhy::sendUnicast(LteAirFrame *frame)
{
    UserControlInfo *ci = check_and_cast<UserControlInfo *>(frame->getControlInfo());
    // dest MacNodeId from control info
    MacNodeId dest = ci->getDestId();

    EV << NOW << " UePhy::sendUnicast - sending frame to node " << dest << endl;

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

// TODO: ***reorganize*** method
void UePhy::handleAirFrame(cMessage* msg)
{
    UserControlInfo* lteInfo = check_and_cast<UserControlInfo*>(msg->removeControlInfo());

    if (useBattery_)
    {
        //TODO BatteryAccess::drawCurrent(rxAmount_, 0);
    }
    connectedNodeId_ = masterId_;
    LteAirFrame* frame = check_and_cast<LteAirFrame*>(msg);
    EV << "UePhy::handleAirFrame - received new LteAirFrame with ID " << frame->getId() << " from channel" << endl;

    int sourceId = lteInfo->getSourceId();
    if(binder_->getOmnetId(sourceId) == 0 )
    {
        // source has left the simulation
        delete msg;
        return;
    }

    double carrierFreq = lteInfo->getCarrierFrequency();
    LteChannelModel* channelModel = getChannelModel(carrierFreq);
    if (channelModel == NULL)
    {
        EV << "Received packet on carrier frequency not supported by this node. Delete it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    //Update coordinates of this user
    if (lteInfo->getFrameType() == HANDOVERPKT)
    {
        // check if the message is on another carrier frequency or handover is already in process
        if (carrierFreq != primaryChannelModel_->getCarrierFrequency() || (handoverTrigger_ != NULL && handoverTrigger_->isScheduled()))
        {
            EV << "Received handover packet on a different carrier frequency. Delete it." << endl;
            delete lteInfo;
            delete frame;
            return;
        }

        // check if the message is from a different cellular technology
        if (lteInfo->isNr() != isNr_)
        {
            EV << "Received handover packet [from NR=" << lteInfo->isNr() << "] from a different radio technology [to NR=" << isNr_ << "]. Delete it." << endl;
            delete lteInfo;
            delete frame;
            return;
        }

        // check if the eNodeB is a secondary node
        MacNodeId masterNodeId = binder_->getMasterNode(sourceId);
        if (masterNodeId != sourceId)
        {
            // the node has a master node, check if the other PHY of this UE is attached to that master.
            // if not, the UE cannot attach to this secondary node and the packet must be deleted.
            if (otherPhy_->getMasterId() != masterNodeId)
            {
                EV << "Received handover packet from " << sourceId << ", which is a secondary node to a master [" << masterNodeId << "] different from the one this UE is attached to. Delete packet." << endl;
                delete lteInfo;
                delete frame;
                return;
            }
        }

        handoverHandler(frame, lteInfo);
        return;
    }

    // Check if the frame is for us ( MacNodeId matches or - if this is a multicast communication - enrolled in multicast group)
    if (lteInfo->getDestId() != nodeId_ && !(binder_->isInMulticastGroup(nodeId_, lteInfo->getMulticastGroupId())))
    {
        EV << "ERROR: Frame is not for us. Delete it." << endl;
        EV << "Packet Type: " << phyFrameTypeToA((LtePhyFrameType)lteInfo->getFrameType()) << endl;
        EV << "Frame MacNodeId: " << lteInfo->getDestId() << endl;
        EV << "Local MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    /*
     * This could happen if the ue associates with a new master while a packet from the
     * old master is in-flight: the packet is in the air
     * while the ue changes master.
     * Event timing:      TTI x: packet scheduled and sent by the UE (tx time = 1ms)
     *                     TTI x+0.1: ue changes master
     *                     TTI x+1: packet from UE arrives at the old master
     */
    if (!resAllocateMode_ && lteInfo->getDirection() != D2D && lteInfo->getDirection() != D2D_MULTI && lteInfo->getSourceId() != masterId_)
    {
        EV << "WARNING: frame from a UE that is leaving this cell (handover): deleted " << endl;
        EV << "Source MacNodeId: " << lteInfo->getSourceId() << endl;
        EV << "UE MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }
    

    if (binder_->isInMulticastGroup(nodeId_,lteInfo->getMulticastGroupId()))
    {
        // HACK: if this is a multicast connection, change the destId of the airframe so that upper layers can handle it
        lteInfo->setDestId(nodeId_);
    }

    // send H-ARQ feedback up
    if (lteInfo->getFrameType() == HARQPKT || lteInfo->getFrameType() == GRANTPKT || lteInfo->getFrameType() == RACPKT || lteInfo->getFrameType() == D2DMODESWITCHPKT)
    {
        //handleControlMsg(frame, lteInfo);
        auto pkt = check_and_cast<inet::Packet *>(frame->decapsulate());
        delete frame;
        *(pkt->addTagIfAbsent<UserControlInfo>()) = *lteInfo;
        delete lteInfo;
        send(pkt, upperGateOut_);

        return;
    }

    // this is a DATA packet

    // if the packet is a D2D multicast one, store it and decode it at the end of the TTI
    if (d2dMulticastEnableCaptureEffect_ && binder_->isInMulticastGroup(nodeId_,lteInfo->getMulticastGroupId()))
    {
        // if not already started, auto-send a message to signal the presence of data to be decoded
        if (d2dDecodingTimer_ == NULL)
        {
            d2dDecodingTimer_ = new cMessage("d2dDecodingTimer");
            d2dDecodingTimer_->setSchedulingPriority(10);          // last thing to be performed in this TTI
            scheduleAt(NOW, d2dDecodingTimer_);
        }

        // store frame, together with related control info
        frame->setControlInfo(lteInfo);
        storeAirFrame(frame);            // implements the capture effect

        return;                          // exit the function, decoding will be done later
    }

    if ((lteInfo->getUserTxParams()) != NULL)
    {
        int cw = lteInfo->getCw();
        if (lteInfo->getUserTxParams()->readCqiVector().size() == 1)
            cw = 0;
        double cqi = lteInfo->getUserTxParams()->readCqiVector()[cw];
        if (lteInfo->getDirection() == DL)
        {
            emit(averageCqiDl_, cqi);
            recordCqi(cqi, DL);
        }
    }
    // apply decider to received packet
    bool result = true;
    if (!resAllocateMode_)
    {
        RemoteSet r = lteInfo->getUserTxParams()->readAntennaSet();
        if (r.size() > 1)
        {
            // DAS
            for (RemoteSet::iterator it = r.begin(); it != r.end(); it++)
            {
                EV << "UePhy::handleAirFrame - Receiving Packet from antenna " << (*it) << "\n";

                /*
                * On UE set the sender position
                * and tx power to the sender das antenna
                */

            //    cc->updateHostPosition(myHostRef,das_->getAntennaCoord(*it));
            //    // Set position of sender
            //    Move m;
            //    m.setStart(das_->getAntennaCoord(*it));
                RemoteUnitPhyData data;
                data.txPower=lteInfo->getTxPower();
                data.m=getCoord();
                frame->addRemoteUnitPhyDataVector(data);
            }
            // apply analog models For DAS
            result = channelModel->isErrorDas(frame,lteInfo);
        }
        else
        {
            result = channelModel->isError(frame,lteInfo);
        }
    }
    
    // update statistics
    if (result)
        numAirFrameReceived_++;
    else
        numAirFrameNotReceived_++;

    EV << "UePhy::handleAirFrame - Handled LteAirframe with ID " << frame->getId() << " with result "
       << ( result ? "RECEIVED" : "NOT RECEIVED" ) << endl;

    auto pkt = check_and_cast<inet::Packet *>(frame->decapsulate());

    // here frame has to be destroyed since it is no more useful
    delete frame;

    // attach the decider result to the packet as control info
    lteInfo->setDeciderResult(result);
    *(pkt->addTagIfAbsent<UserControlInfo>()) = *lteInfo;
    delete lteInfo;

    // send decapsulated message along with result control info to upperGateOut_
    send(pkt, upperGateOut_);

    if (getEnvir()->isGUI())
        updateDisplayString();
}

void LtePhyBase::handleControlMsg(LteAirFrame *frame,
    UserControlInfo *userInfo)
{
    auto pkt = check_and_cast<inet::Packet *>(frame->decapsulate());
    delete frame;
    *(pkt->addTagIfAbsent<UserControlInfo>()) = *userInfo;
    delete userInfo;
    send(pkt, upperGateOut_);
    return;
}

void UePhy::handleUpperMessage(cMessage* msg)
{
//    if (useBattery_) {
//    TODO     BatteryAccess::drawCurrent(txAmount_, 1);
//    }

    auto pkt = check_and_cast<inet::Packet *>(msg);
    auto lteInfo = pkt->removeTag<UserControlInfo>();
    simtime_t duration = TTI;
    if (pkt->findTag<VecDataInfo>() != nullptr)
    {
        auto vecInfo = pkt->removeTag<VecDataInfo>();
        duration = vecInfo->getDuration();
    }

    double carrierFreq = lteInfo->getCarrierFrequency();
    LteChannelModel* channelModel = getChannelModel(carrierFreq);
    if (channelModel == NULL)
        throw cRuntimeError("UePhy::handleUpperMessage - Carrier frequency [%f] not supported by any channel model", carrierFreq);


    if (lteInfo->getFrameType() == DATAPKT && (channelModel->isUplinkInterferenceEnabled() || channelModel->isD2DInterferenceEnabled()))
    {
        // Store the RBs used for data transmission to the binder (for UL interference computation)
        EV << "UePhy::handleUpperMessage - storing UL transmission to band manager, duration " << duration << endl;
        RbMap rbMap = lteInfo->getGrantedBlocks();
        bandManager_->addTransmissionUl(nodeId_, lteInfo->getDestId(), rbMap, NOW + duration);

        // Remote antenna = MACRO;  // TODO fix for multi-antenna
        // Direction dir = (Direction)lteInfo->getDirection();
        // if (resAllocateMode_)
        //     binder_->storeUlTransmissionMap(carrierFreq, antenna, rbMap, nodeId_, lteInfo->getDestId(), this, dir);
        // else
        //     binder_->storeUlTransmissionMap(channelModel->getCarrierFrequency(), antenna, rbMap, nodeId_, mac_->getMacCellId(), this, dir);
    }

    if (lteInfo->getFrameType() == DATAPKT && lteInfo->getUserTxParams() != nullptr)
    {
        double cqi = lteInfo->getUserTxParams()->readCqiVector()[lteInfo->getCw()];
        if (lteInfo->getDirection() == UL)
        {
            emit(averageCqiUl_, cqi);
            recordCqi(cqi, UL);
        }
        else if (lteInfo->getDirection() == D2D || lteInfo->getDirection() == D2D_MULTI)
            emit(averageCqiD2D_, cqi);
    }

    EV << NOW << " UePhy::handleUpperMessage - message from stack" << endl;
    LteAirFrame* frame = nullptr;

    if (lteInfo->getFrameType() == HARQPKT || lteInfo->getFrameType() == GRANTPKT || lteInfo->getFrameType() == RACPKT)
    {
        if (lteInfo->getFrameType() == RACPKT)
        {
            updateMasterNode();
            lteInfo->setDestId(masterId_);
        }
        
        frame = new LteAirFrame("harqFeedback-grant");
        // set transmission duration according to the numerology
        NumerologyIndex numerologyIndex = binder_->getNumerologyIndexFromCarrierFreq(lteInfo->getCarrierFrequency());
        double slotDuration = binder_->getSlotDurationFromNumerologyIndex(numerologyIndex);
        frame->setDuration(slotDuration);
    }
    else
    {
        // create LteAirFrame and encapsulate the received packet
        frame = new LteAirFrame("airframe");
        frame->setDuration(duration);
    }

    frame->encapsulate(pkt);
    // initialize frame fields
    frame->setSchedulingPriority(airFramePriority_);

    // set current position
    lteInfo->setCoord(getCoord());
    lteInfo->setTxPower(txPower_);
    lteInfo->setD2dTxPower(d2dTxPower_);
    frame->setControlInfo(lteInfo.get()->dup());

    EV << "UePhy::handleUpperMessage - " << nodeTypeToA(nodeType_) << " with id " << nodeId_
       << " sending message to the air channel. Dest=" << lteInfo->getDestId() << endl;

    // if this is a multicast/broadcast connection, send the frame to all neighbors in the hearing range
    // otherwise, send unicast to the destination
    if (lteInfo->getDirection() == D2D_MULTI)
        sendMulticast(frame);
    else
        sendUnicast(frame);

    lteInfo = nullptr;
    pkt = nullptr;
}



