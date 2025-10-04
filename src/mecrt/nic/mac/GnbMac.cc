//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    GnbMac.cc / GnbMac.h
//
//  Description:
//    This file implements the MAC layer for the gNB in the MEC context.
//    Compared to the original NRMacGnb, we add the control logic for data offloading,
//    including interaction with the RSU server and the global scheduler, and a 
//    adaptive offloading control mechanism based on the real-time SRS feedback from UEs.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//S
// simulate the MAC stack of the NIC module of gNB
// LteMacBase --> LteMacEnb --> LteMacEnbD2D --> NRMacGnb
//
#include "mecrt/nic/mac/GnbMac.h"
#include "mecrt/nic/mac/scheduler/GnbSchedulerDl.h"
#include "mecrt/nic/mac/scheduler/GnbSchedulerUl.h"
#include "mecrt/packets/apps/RsuFeedback_m.h"
#include "mecrt/packets/apps/Grant2Veh.h"
#include "mecrt/packets/apps/ServiceStatus_m.h"

#include <inet/networklayer/common/L3AddressResolver.h>

#include "stack/rlc/packet/LteRlcDataPdu.h"
#include "stack/rlc/um/LteRlcUm.h"

#include "stack/mac/allocator/LteAllocationModule.h"
#include "stack/mac/amc/NRAmc.h"
#include "stack/mac/amc/AmcPilotD2D.h"
#include "stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/buffer/harq_d2d/LteHarqBufferRxD2D.h"
#include "stack/mac/buffer/LteMacQueue.h"
#include "stack/mac/conflict_graph/DistanceBasedConflictGraph.h"
#include "stack/mac/packet/LteSchedulingGrant.h"
#include "stack/mac/packet/LteMacSduRequest.h"
#include "stack/mac/scheduler/LteSchedulerEnbUl.h"
#include "stack/phy/layer/LtePhyBase.h"
#include "stack/phy/packet/LteFeedbackPkt.h"
#include "stack/packetFlowManager/PacketFlowManagerBase.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/socket/SocketTag_m.h"
#include "stack/rlc/am/packet/LteRlcAmPdu_m.h"
#include "stack/mac/packet/LteRac_m.h"


Define_Module(GnbMac);

using namespace omnetpp;
using namespace inet;

GnbMac::GnbMac()
{
    ttiTick_ = nullptr;
    flushAppPduList_ = nullptr;
    enableInitDebug_ = false;
    conflictGraph_ = nullptr;
    rbManagerUl_ = nullptr;
    nodeInfo_ = nullptr;
}

GnbMac::~GnbMac()
{
    if (enableInitDebug_)
        std::cout << "GnbMac::~GnbMac - destroying MAC protocol\n";

    // remove flushAppPduList_ message
    if (flushAppPduList_)
    {
        cancelAndDelete(flushAppPduList_);
        flushAppPduList_ = nullptr;
    }
    // remove ttiTick_ message
    if (ttiTick_)
    {
        cancelAndDelete(ttiTick_);
        ttiTick_ = nullptr;
    }
    // delete the uplink RB manager
    if (rbManagerUl_)
    {
        delete rbManagerUl_;
        rbManagerUl_ = nullptr;
    }

    if (conflictGraph_)
    {
        delete conflictGraph_;
        conflictGraph_ = nullptr;
    }

    if (enableInitDebug_)
        std::cout << "GnbMac::~GnbMac - destroying MAC protocol done!\n";
}

void GnbMac::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
        if (enableInitDebug_)
            std::cout << "GnbMac::initialize - stage: INITSTAGE_LOCAL - begins" << std::endl;

        /* Gates initialization */
        up_[IN_GATE] = gate("RLC_to_MAC");
        up_[OUT_GATE] = gate("MAC_to_RLC");
        down_[IN_GATE] = gate("PHY_to_MAC");
        down_[OUT_GATE] = gate("MAC_to_PHY");

        /* Create buffers */
        queueSize_ = par("queueSize");  // MAC Buffers queue size, default(2MiB)

        /* Get reference to binder */
        binder_ = getBinder();

        srsDistanceCheck_ = par("srsDistanceCheck");  // whether checking the distance for SRS transmission, default(false)
        srsDistance_ = par("srsDistance");  // the distance for SRS transmission

        /* Set The MAC MIB */

        muMimo_ = par("muMimo");    // default(true)

        harqProcesses_ = par("harqProcesses");  // default(8)

        /* statistics */
        statDisplay_ = par("statDisplay");  // Statistics display (in GUI), default(false)

        totalOverflowedBytes_ = 0;
        nrFromUpper_ = 0;
        nrFromLower_ = 0;
        nrToUpper_ = 0;
        nrToLower_ = 0;

        if(getParentModule()->findSubmodule("packetFlowManager") != -1)
        {
            RanNodeType nt = getNodeType();
            const char *cnt = (nt == ENODEB)? "ENODEB": "GNODEB";
            EV << "GnbMac::initialize - MAC layer, nodeType: "<< cnt  << endl;
            packetFlowManager_ = check_and_cast<PacketFlowManagerBase *>(getParentModule()->getSubmodule("packetFlowManager"));
        }

        // /* register signals */
        macBufferOverflowDl_ = registerSignal("macBufferOverFlowDl");
        macBufferOverflowUl_ = registerSignal("macBufferOverFlowUl");
        if (isD2DCapable())
            macBufferOverflowD2D_ = registerSignal("macBufferOverFlowD2D");
        receivedPacketFromUpperLayer = registerSignal("receivedPacketFromUpperLayer");
        receivedPacketFromLowerLayer = registerSignal("receivedPacketFromLowerLayer");
        sentPacketToUpperLayer = registerSignal("sentPacketToUpperLayer");
        sentPacketToLowerLayer = registerSignal("sentPacketToLowerLayer");
        measuredItbs_ = registerSignal("measuredItbs");

        WATCH(queueSize_);
        WATCH(nodeId_);
        /***
         * In C++, if a std::map (or any other standard container) is not explicitly initialized,
         * it is still in a valid, empty state by default when declared.
         */
        WATCH_MAP(mbuf_);
        WATCH_MAP(macBuffers_);

        // ========= LteMacEnb ===========
        /***
         * the default value of gNodeB->par("macNodeId") is 0, specified in gNodeB.ned;
         * its value is updated in IP2Nic::initialize(), which calls Binder::registerNode().
         * in Binder::registerNode(), the macNodeId of gNB/eNB is set started from value 1, i.e., 1, 2, ...;
         * if there is only one gNB, its corresponding gNodeB->par("macNodeId") will be set as 1.
         * because IP2Nic is initialized before GnbMac, the value of nodeId_ will be 1.
         */
        nodeId_ = getAncestorPar("macNodeId");
        cellId_ = nodeId_;

        // TODO: read NED parameters, when will be present
        // cellInfo_ = getCellInfo();
        cellInfo_ = check_and_cast<CellInfo*>(getParentModule()->getParentModule()->getSubmodule("cellInfo"));

        /* Get number of antennas */
        numAntennas_ = getNumAntennas();

        eNodeBCount = par("eNodeBCount");   // default(0)
        WATCH(numAntennas_);
        WATCH_MAP(bsrbuf_);

        // ========= LteMacEnbD2D ===========
        //cModule* rlc = getParentModule()->getSubmodule("rlc");
        //std::string rlcUmType = rlc->par("LteRlcUmType").stdstringValue();
        //bool rlcD2dCapable = rlc->par("d2dCapable").boolValue();
        //if (rlcUmType.compare("LteRlcUm") != 0 || !rlcD2dCapable)
            //throw cRuntimeError("GnbMac::initialize - %s module found, must be LteRlcUmD2D. Aborting", rlcUmType.c_str());

        if (enableInitDebug_)
            std::cout << "GnbMac::initialize - stage: INITSTAGE_LOCAL - ends" << std::endl;
    }
    else if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT)
    {
        if (enableInitDebug_)
            std::cout << "GnbMac::initialize - stage: INITSTAGE_PHYSICAL_ENVIRONMENT - begins" << std::endl;

        // get node info module
        try {
            nodeInfo_ = getModuleFromPar<NodeInfo>(par("nodeInfoModulePath"), this);
            nodeInfo_->setNodeId(nodeId_);
        } catch (cException &e) {
            throw cRuntimeError("GnbMac:initialize - cannot find nodeInfo module\n");
        }
        
        // ========= LteMacEnb ===========
        /* Create and initialize AMC module */
        // std::string amcType = par("amcType").stdstringValue();  // default("NRAmc")
        // if (amcType.compare("NRAmc") == 0)
        //     amc_ = new NRAmc(this, binder_, cellInfo_, numAntennas_);
        // else
        //     amc_ = new LteAmc(this, binder_, cellInfo_, numAntennas_);
        amc_ = new MecNRAmc(this, binder_, cellInfo_, numAntennas_);
        LteMacEnb::amc_ = amc_; // assign the amc module to the parent class

        std::string modeString = par("pilotMode").stdstringValue(); // default("ROBUST_CQI");

        if( modeString == "AVG_CQI" )
            amc_->setPilotMode(AVG_CQI);
        else if( modeString == "MAX_CQI" )
            amc_->setPilotMode(MAX_CQI);
        else if( modeString == "MIN_CQI" )
            amc_->setPilotMode(MIN_CQI);
        else if( modeString == "MEDIAN_CQI" )
            amc_->setPilotMode(MEDIAN_CQI);
        else if( modeString == "ROBUST_CQI" )
            amc_->setPilotMode(ROBUST_CQI);
        
        else
            throw cRuntimeError("GnbMac::initialize - Unknown Pilot Mode %s \n" , modeString.c_str());

        /* Insert EnbInfo in the Binder */
        EnbInfo* info = new EnbInfo();
        info->id = nodeId_;            // local mac ID
        info->nodeType = nodeType_;    // eNB or gNB
        info->type = MACRO_ENB;        // eNb Type
        info->init = false;            // flag for phy initialization
        info->eNodeB = this->getParentModule()->getParentModule();  // reference to the eNodeB module
        binder_->addEnbInfo(info);

        // register the pairs <id,name> and <id, module> to the binder
        cModule* module = getParentModule()->getParentModule();
        // Modified from getFullName() to getFullPath() to fix the usage in compound modules
        std::string tmpName = getParentModule()->getParentModule()->getFullPath();
        const char* moduleName = strcpy(new char[tmpName.length() + 1], tmpName.c_str());
        binder_->registerName(nodeId_, moduleName);
        binder_->registerModule(nodeId_, module);

        // get the reference to the PHY layer
        phy_ = check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("phy"));

        // ========= LteMacEnbD2D ===========
        usePreconfiguredTxParams_ = par("usePreconfiguredTxParams");    // default(false)
        Cqi d2dCqi = par("d2dCqi"); // default(7)
        if (usePreconfiguredTxParams_)
            check_and_cast<AmcPilotD2D*>(amc_->getPilot())->setPreconfiguredTxParams(d2dCqi);

        msHarqInterrupt_ = par("msHarqInterrupt").boolValue();  // ms: mode switch, default(true)
        msClearRlcBuffer_ = par("msClearRlcBuffer").boolValue();    // default(true)

        if (enableInitDebug_)
            std::cout << "GnbMac::initialize - stage: INITSTAGE_PHYSICAL_ENVIRONMENT - ends" << std::endl;
    }
    else if (stage == inet::INITSTAGE_LINK_LAYER)
    {
        if (enableInitDebug_)
            std::cout << "GnbMac::initialize - stage: INITSTAGE_LINK_LAYER - begins" << std::endl;

        // ========= NEW UPDATE ===========
        /**
         * TODO multiple carriers is not implemented yet, pending future work
         */
        
        resAllocateMode_ = par("resAllocateMode");
        serverPort_ = nodeInfo_->getServerPort();
        // std::cout << "server port: " << serverPort_ << endl;
        rbPerBand_ = par("numRbPerBand");
        EV << "GnbMac::initialize - number of resource blocks per Band " << rbPerBand_ << endl;

        // ========= NRMacGnb ===========
        /* Create and initialize NR MAC Uplink scheduler */
        if (gnbSchedulerUl_ == nullptr)
        {
            //enbSchedulerUl_ = new NRSchedulerGnbUl();
            gnbSchedulerUl_ = new GnbSchedulerUl();
            enbSchedulerUl_ = gnbSchedulerUl_;
            (gnbSchedulerUl_->resourceBlocks()) = cellInfo_->getNumBands() * rbPerBand_; // total number of bands of all usable carriers
            gnbSchedulerUl_->initialize(UL, this);
        }

        // ========= LteMacEnb ===========
        /* Create and initialize MAC Downlink scheduler */
        if (gnbSchedulerDl_ == nullptr)
        {
            // enbSchedulerDl_ = new LteSchedulerEnbDl();
            gnbSchedulerDl_ = new GnbSchedulerDl();
            enbSchedulerDl_ = gnbSchedulerDl_;
            (gnbSchedulerDl_->resourceBlocks()) = cellInfo_->getNumBands() * rbPerBand_;
            /* use dynamic casting to call GnbSchedulerDl::initialize() */
            gnbSchedulerDl_->initialize(DL, this);
        }

        const CarrierInfoMap* carriers = cellInfo_->getCarrierInfoMap();
        CarrierInfoMap::const_iterator it = carriers->begin();
        int i = 0;
        for ( ; it != carriers->end(); ++it, ++i)
        {
            double carrierFrequency = it->second.carrierFrequency;
            bgTrafficManager_[carrierFrequency] = check_and_cast<BackgroundTrafficManager*>(getParentModule()->getSubmodule("bgTrafficGenerator",i)->getSubmodule("manager"));
            bgTrafficManager_[carrierFrequency]->setCarrierFrequency(carrierFrequency);
        }

        if (enableInitDebug_)
            std::cout << "GnbMac::initialize - stage: INITSTAGE_LINK_LAYER - ends" << std::endl;
    }
    else if (stage == INITSTAGE_LAST)  // after all UEs have been initialized
    {
        if (enableInitDebug_)
            std::cout << "GnbMac::initialize - stage: INITSTAGE_LAST - begins" << std::endl;

        gnbAddress_ = nodeInfo_->getNodeAddr();
        EV << "GnbMac::initialize - gNB address " << gnbAddress_.toIpv4().str() << ", gNB MacNodeId " << nodeId_ << endl;
        binder_->setMacNodeId(gnbAddress_.toIpv4(), nodeId_);

        // ========= LteMacEnb ===========
        /* Start TTI tick */
        // the period is equal to the minimum period according to the numerologies used by the carriers in this node
        ttiTick_ = new cMessage("ttiTick_");
        ttiTick_->setSchedulingPriority(1);                                              // TTI TICK after other messages
        ttiPeriod_ = binder_->getSlotDurationFromNumerologyIndex(cellInfo_->getMaxNumerologyIndex());
        // scheduleAt(NOW + ttiPeriod_, ttiTick_);

        set<NumerologyIndex> numerologySet;
        const CarrierInfoMap* carriers = cellInfo_->getCarrierInfoMap();
        CarrierInfoMap::const_iterator it = carriers->begin();
        for ( ; it != carriers->end(); ++it)
        {
            // set periodicity for this carrier according to its numerology
            NumerologyPeriodCounter info;
            info.max = 1 << (cellInfo_->getMaxNumerologyIndex() - it->second.numerologyIndex); // 2^(maxNumerologyIndex - numerologyIndex)
            info.current = info.max - 1;
            numerologyPeriodCounter_[it->second.numerologyIndex] = info;

            numerologySet.insert(it->second.numerologyIndex);
        }
        numerologyCount_ = numerologySet.size();

        // set the periodicity for each scheduler
        enbSchedulerDl_->initializeSchedulerPeriodCounter(cellInfo_->getMaxNumerologyIndex());
        enbSchedulerUl_->initializeSchedulerPeriodCounter(cellInfo_->getMaxNumerologyIndex());
        /***
         * =======================================
         * initialize band status in each carrier (multiple carriers not support yet)
         * =======================================
         */

        if (resAllocateMode_ && rbManagerUl_ == nullptr)
        {
            if (cellInfo_->getCarriers()->size() > 1)
                throw cRuntimeError("GnbMac::initialize - multiple carriers not supported yet for resource allocation");
            rbManagerUl_ = new RbManagerUl(this, amc_);
            rbManagerUl_->setRbPerBand(rbPerBand_);

            for (it = carriers->begin(); it != carriers->end(); ++it)
            {
                double freq = it->second.carrierFrequency;
                int numberBands = it->second.numBands;

                rbManagerUl_->setFrequency(freq);
                NumerologyIndex numerologyIndex = binder_->getNumerologyIndexFromCarrierFreq(freq);
                rbManagerUl_->setNumerology(numerologyIndex);
                rbManagerUl_->setNumBands(numberBands);
                rbManagerUl_->initBandStatus();
            }

            availableBands_ = rbManagerUl_->getNumBands();
        }

        WATCH(availableBands_);
        // ========================================

        // ========= LteMacEnbD2D ===========
        reuseD2D_ = par("reuseD2D");    // default(false)
        reuseD2DMulti_ = par("reuseD2DMulti");  // default(false)

        if (reuseD2D_ || reuseD2DMulti_)
        {
            conflictGraphUpdatePeriod_ = par("conflictGraphUpdatePeriod");

            CGType cgType = CG_DISTANCE;  // TODO make this parametric
            switch(cgType)
            {
                case CG_DISTANCE:
                {
                    conflictGraph_ = new DistanceBasedConflictGraph(this, reuseD2D_, reuseD2DMulti_, par("conflictGraphThreshold"));
                    check_and_cast<DistanceBasedConflictGraph*>(conflictGraph_)->setThresholds(par("conflictGraphD2DInterferenceRadius"), par("conflictGraphD2DMultiTxRadius"), par("conflictGraphD2DMultiInterferenceRadius"));
                    break;
                }
                default: { throw cRuntimeError("LteMacEnbD2D::initialize - CG type unknown. Aborting"); }
            }

            scheduleAt(NOW + 0.05, new cMessage("updateConflictGraph"));
        }

        EV << "GnbMac::initialize - macNodeId  " << nodeId_ << ", macCellId " << cellId_ << endl;

        flushAppPduList_ = new cMessage("flushAppPduList");
        flushAppPduList_->setSchedulingPriority(1);        // after other messages

        if (enableInitDebug_)
            std::cout << "GnbMac::initialize - stage: INITSTAGE_LAST - ends" << std::endl;
    }
}

void GnbMac::handleMessage(cMessage* msg)
{
    if (msg->isSelfMessage())
    {
        if (msg->isName("D2DModeSwitchNotification"))
        {
            cPacket* pkt = check_and_cast<cPacket*>(msg);
            macHandleD2DModeSwitch(pkt);
            cancelAndDelete(msg);
        }
        else if (msg->isName("updateConflictGraph"))
        {
            // compute conflict graph for resource allocation
            conflictGraph_->computeConflictGraph();

            // debug
            // conflictGraph_->printConflictGraph();

            scheduleAt(NOW + conflictGraphUpdatePeriod_, msg);
        }
        else if (strcmp(msg->getName(), "flushHarqMsg") == 0)
        {
            flushHarqBuffers();
            cancelAndDelete(msg);
        }
        else if (strcmp(msg->getName(), "flushAppPduList") == 0)
        {
            flushAppPduList();
        }
        else
        {
            // if (!resAllocateMode_)
            //     handleSelfMessage();
            // else
            // {
            //     // init and reset global allocation information
            //     if (binder_->getLastUpdateUlTransmissionInfo() < NOW)  // once per TTI, even in case of multicell scenarios
            //         binder_->initAndResetUlTransmissionInfo();
                
            //     decreaseNumerologyPeriodCounter();
            // }
            // scheduleAt(NOW + ttiPeriod_, ttiTick_);

            // new grant from rsu server
            while (!grantList_.empty())
            {
                cPacket * grant = grantList_.back();
                vecHandleGrantFromRsu(grant);
                delete grant;
                grant = nullptr;
                grantList_.pop_back();
            }
        }
    }
    else
    {
        cPacket* pkt = check_and_cast<cPacket *>(msg);
        EV << "GnbMac::handleMessage - Received packet " << pkt->getName() <<
        " from port " << pkt->getArrivalGate()->getName() << endl;

        cGate* incoming = pkt->getArrivalGate();

        if (incoming == down_[IN_GATE])
        {
            // message from PHY_to_MAC gate (from lower layer)
            emit(receivedPacketFromLowerLayer, pkt);
            nrFromLower_++;
            fromPhy(pkt);
        }
        else
        {
            // message from RLC_to_MAC gate (from upper layer)
            emit(receivedPacketFromUpperLayer, pkt);
            nrFromUpper_++;
            // fromRlc(pkt);
            handleUpperMessage(pkt);
        }
    }
}

void GnbMac::handleSelfMessage()
{
    /***************
     *  MAIN LOOP  *
     ***************/

    EV << "GnbMac::handleSelfMessage - mac stack main loop starts." << endl;
    EV << "-----" << " GNB MAIN LOOP -----" << endl;

    /* Reception */

    // extract pdus from all harqrxbuffers and pass them to unmaker
    auto mit = harqRxBuffers_.begin();
    auto met = harqRxBuffers_.end();
    for (; mit != met; mit++)
    {
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(mit->first)) > 0)
            continue;

        auto hit = mit->second.begin();
        auto het = mit->second.end();
        for (; hit != het; hit++)
        {
            auto pduList = hit->second->extractCorrectPdus();
            while (!pduList.empty())
            {
                auto pdu = pduList.front();
                pduList.pop_front();
                macPduUnmake(pdu);
            }
        }
    }

    /*UPLINK*/
    EV << "============================================== UPLINK ==============================================" << endl;
    // init and reset global allocation information
    if (binder_->getLastUpdateUlTransmissionInfo() < NOW)  // once per TTI, even in case of multicell scenarios
        binder_->initAndResetUlTransmissionInfo();

    gnbSchedulerUl_->updateHarqDescs(); // does nothing for NRSchedulerGnbUl

    std::map<double, LteMacScheduleList>* scheduleListUl = gnbSchedulerUl_->schedule();
    // send uplink grants to PHY layer
    sendGrants(scheduleListUl);

    EV << "============================================ END UPLINK ============================================" << endl;

    EV << "============================================ DOWNLINK ==============================================" << endl;
    /*DOWNLINK*/

    // use this flag to enable/disable scheduling...don't look at me, this is very useful!!!
    bool activation = true;

    if (activation)
    {
        // clear previous schedule list
        if (scheduleListDl_ != nullptr)
        {
            std::map<double, LteMacScheduleList>::iterator cit = scheduleListDl_->begin();
            for (; cit != scheduleListDl_->end(); ++cit)
                cit->second.clear();
            scheduleListDl_->clear();
        }

        // perform Downlink scheduling
        scheduleListDl_ = gnbSchedulerDl_->schedule();

        // requests SDUs to the RLC layer
        macSduRequest();
    }
    EV << "========================================== END DOWNLINK ============================================" << endl;

    // purge from corrupted PDUs all Rx H-HARQ buffers for all users
    for (mit = harqRxBuffers_.begin(); mit != met; mit++)
    {
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(mit->first)) > 0)
            continue;

        HarqRxBuffers::iterator hit = mit->second.begin();
        HarqRxBuffers::iterator het = mit->second.end();
        for (; hit != het; hit++)
            hit->second->purgeCorruptedPdus();
    }

    // Message that triggers flushing of Tx H-ARQ buffers for all users
    // This way, flushing is performed after the (possible) reception of new MAC PDUs
    
    cMessage* flushHarqMsg = new cMessage("flushHarqMsg");
    flushHarqMsg->setSchedulingPriority(1);        // after other messages
    scheduleAt(NOW, flushHarqMsg);

    decreaseNumerologyPeriodCounter();

    EV << "--- END GNB MAIN LOOP ---" << endl;
}


void GnbMac::macPduUnmake(cPacket* pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    auto macPkt = pkt->removeAtFront<LteMacPdu>();

    /*
     * @author Alessandro Noferi
     * Notify the pfm about the successful arrival of a TB from a UE.
     * From ETSI TS 138314 V16.0.0 (2020-07)
     *   tSucc: the point in time when the MAC SDU i was received successfully by the network
     */
    auto userInfo = pkt->getTag<UserControlInfo>();

    if(packetFlowManager_ != nullptr)
    {
        packetFlowManager_->ulMacPduArrived(userInfo->getSourceId(), userInfo->getGrantId());
    }

    while (macPkt->hasSdu())
    {
        // Extract and send SDU
        auto upPkt = check_and_cast<Packet *>(macPkt->popSdu());
        take(upPkt);

        EV << "GnbMac::macPduUnmake - pduUnmaker extracted SDU" << endl;

        // store descriptor for the incoming connection, if not already stored
        auto lteInfo = upPkt->getTag<FlowControlInfo>();
        MacNodeId senderId = lteInfo->getSourceId();
        LogicalCid lcid = lteInfo->getLcid();
        MacCid cid = idToMacCid(senderId, lcid);
        if (connDescIn_.find(cid) == connDescIn_.end())
        {
            FlowControlInfo toStore(*lteInfo);
            connDescIn_[cid] = toStore;
        }

        sendUpperPackets(upPkt);
    }

    while (macPkt->hasCe())
    {
        // Extract CE
        // TODO: vedere se   per cid o lcid
        MacBsr* bsr = check_and_cast<MacBsr*>(macPkt->popCe());
        auto lteInfo = pkt->getTag<UserControlInfo>();
        LogicalCid lcid = lteInfo->getLcid();  // one of SHORT_BSR or D2D_MULTI_SHORT_BSR

        MacCid cid = idToMacCid(lteInfo->getSourceId(), lcid); // this way, different connections from the same UE (e.g. one UL and one D2D)
                                                               // obtain different CIDs. With the inverse operation, you can get
                                                               // the LCID and discover if the connection is UL or D2D
        bufferizeBsr(bsr, cid);
    }
    pkt->insertAtFront(macPkt);

    delete pkt;
}

void GnbMac::bufferizeBsr(MacBsr* bsr, MacCid cid)
{
    LteMacBufferMap::iterator it = bsrbuf_.find(cid);
    if (it == bsrbuf_.end())
    {
        if (bsr->getSize() > 0)
        {
            // Queue not found for this cid: create
            LteMacBuffer* bsrqueue = new LteMacBuffer();

            PacketInfo vpkt(bsr->getSize(), bsr->getTimestamp());
            bsrqueue->pushBack(vpkt);
            bsrbuf_[cid] = bsrqueue;

            EV << "GnbMac::bufferizeBsr - LteBsrBuffers : Added new BSR buffer for node: "
               << MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid)
               << " Current BSR size: " << bsr->getSize() << "\n";

            // signal backlog to Uplink scheduler
            gnbSchedulerUl_->backlog(cid);
        }
        // do not store if BSR size = 0
    }
    else
    {
        // Found
        LteMacBuffer* bsrqueue = it->second;
        if (bsr->getSize() > 0)
        {
            // update buffer
            PacketInfo queuedBsr;
            if (!bsrqueue->isEmpty())
                queuedBsr = bsrqueue->popFront();

            queuedBsr.first = bsr->getSize();
            queuedBsr.second = bsr->getTimestamp();
            bsrqueue->pushBack(queuedBsr);

            EV << "GnbMac::bufferizeBsr - LteBsrBuffers : Using old buffer for node: " << MacCidToNodeId(
                cid) << " for Lcid: " << MacCidToLcid(cid)
               << " Current BSR size: " << bsr->getSize() << "\n";

            // signal backlog to Uplink scheduler
            gnbSchedulerUl_->backlog(cid);
        }
        else
        {
            // the UE has no backlog, remove BSR
            if (!bsrqueue->isEmpty())
                bsrqueue->popFront();

            EV << "GnbMac::bufferizeBsr - LteBsrBuffers : Using old buffer for node: " << MacCidToNodeId(
                cid) << " for Lcid: " << MacCidToLcid(cid)
               << " - now empty" << "\n";
        }
    }
}

/*
 * Lower layer handler
 */
void GnbMac::fromPhy(cPacket *pktAux)
{
    EV << "GnbMac::fromPhy - received packet " << pktAux->getName() << endl;
    
    // TODO: harq test (comment fromPhy: it has only to pass pdus to proper rx buffer and
    // to manage H-ARQ feedback)
    auto pkt = check_and_cast<inet::Packet*> (pktAux);
    auto userInfo = pkt->getTag<UserControlInfo>();

    MacNodeId src = userInfo->getSourceId();
    double carrierFreq = userInfo->getCarrierFrequency();

    if (userInfo->getFrameType() == HARQPKT)
    {
        // this feedback refers to a mirrored H-ARQ buffer
        auto hfbpkt = pkt->peekAtFront<LteHarqFeedback>();
        if (!hfbpkt->getD2dFeedback())   // this is not a mirror feedback
        {
            // ========== LteMacBase BEGIN==========
            if (harqTxBuffers_.find(carrierFreq) == harqTxBuffers_.end())
            {
                HarqTxBuffers newTxBuffs;
                harqTxBuffers_[carrierFreq] = newTxBuffs;
            }

            // H-ARQ feedback, send it to TX buffer of source
            HarqTxBuffers::iterator htit = harqTxBuffers_[carrierFreq].find(src);
            EV << NOW << "GnbMac::fromPhy - node " << nodeId_ << " Received HARQ Feedback pkt" << endl;
            if (htit == harqTxBuffers_[carrierFreq].end())
            {
                // if a feedback arrives, a tx buffer must exists (unless it is an handover scenario
                // where the harq buffer was deleted but a feedback was in transit)
                // this case must be taken care of

                if (binder_->hasUeHandoverTriggered(nodeId_) || binder_->hasUeHandoverTriggered(src))
                    return;

                throw cRuntimeError("GnbMac::fromPhy - Received feedback for an unexisting H-ARQ tx buffer");
            }

            // auto hfbpkt = pkt->peekAtFront<LteHarqFeedback>();   // not used here
            htit->second->receiveHarqFeedback(pkt);

            // ========== LteMacBase END==========

            return;
        }

        // H-ARQ feedback, send it to mirror buffer of the D2D pair
        auto mfbpkt = pkt->peekAtFront<LteHarqFeedbackMirror>();
        MacNodeId d2dSender = mfbpkt->getD2dSenderId();
        MacNodeId d2dReceiver = mfbpkt->getD2dReceiverId();
        D2DPair pair(d2dSender, d2dReceiver);
        HarqBuffersMirrorD2D::iterator hit = harqBuffersMirrorD2D_[carrierFreq].find(pair);
        EV << NOW << " GnbMac::fromPhy - node " << nodeId_ << " Received HARQ Feedback pkt (mirrored)" << endl;
        if (hit == harqBuffersMirrorD2D_[carrierFreq].end())
        {
            // if a feedback arrives, a buffer should exists (unless it is an handover scenario
            // where the harq buffer was deleted but a feedback was in transit)
            // this case must be taken care of
            if (binder_->hasUeHandoverTriggered(src))
                return;

            // create buffer
            LteHarqBufferMirrorD2D* hb = new LteHarqBufferMirrorD2D((unsigned int) UE_TX_HARQ_PROCESSES, (unsigned char)par("maxHarqRtx"), this);
            harqBuffersMirrorD2D_[carrierFreq][pair] = hb;
            hb->receiveHarqFeedback(pkt);
        }
        else
        {
            hit->second->receiveHarqFeedback(pkt);
        }
    }
    else    // ========== LteMacBase ==========
    {
        if (userInfo->getFrameType() == FEEDBACKPKT)
        {
            //Feedback pkt
            EV << NOW << " GnbMac::fromPhy - node " << nodeId_ << " Received feedback pkt" << endl;
            macHandleFeedbackPkt(pkt);
        }
        else if (userInfo->getFrameType()==GRANTPKT)
        {
            //Scheduling Grant
            EV << NOW << " GnbMac::fromPhy - node " << nodeId_ << " Received Scheduling Grant pkt" << endl;
            macHandleGrant(pkt);
        }
        else if(userInfo->getFrameType() == DATAPKT)
        {
            // data packet: insert in proper rx buffer
            EV << NOW << " GnbMac::fromPhy - node " << nodeId_ << " Received DATA packet" << endl;

            // ========= Newly Added ==========
            unsigned short portId = userInfo->getLcid();
            AppId appId = idToMacCid(src, portId);
            // ========= Newly Added ==========

            if (resAllocateMode_)
            {
                // here we do not consider waiting time for the packet correctness check
                auto pduAux = pkt->peekAtFront<LteMacPdu>();
                auto pdu = pkt;
                appPduList_[appId] = pdu;

                if (flushAppPduList_ != nullptr && !flushAppPduList_->isScheduled())
                    scheduleAt(NOW, flushAppPduList_);
            }
            else
            {
                auto pduAux = pkt->peekAtFront<LteMacPdu>();
                auto pdu = pkt;
                Codeword cw = userInfo->getCw();
    
                if (harqRxBuffers_.find(carrierFreq) == harqRxBuffers_.end())
                {
                    HarqRxBuffers newRxBuffs;
                    harqRxBuffers_[carrierFreq] = newRxBuffs;
                }
    
                HarqRxBuffers::iterator hrit = harqRxBuffers_[carrierFreq].find(src);
                if (hrit != harqRxBuffers_[carrierFreq].end())
                {
                    hrit->second->insertPdu(cw,pdu);
                }
                else
                {
                    // FIXME: possible memory leak
                    LteHarqBufferRx *hrb;
                    if (userInfo->getDirection() == DL || userInfo->getDirection() == UL)
                        hrb = new LteHarqBufferRx(ENB_RX_HARQ_PROCESSES, this,src);
                    else // D2D
                        hrb = new LteHarqBufferRxD2D(ENB_RX_HARQ_PROCESSES, this,src, (userInfo->getDirection() == D2D_MULTI) );
    
                    harqRxBuffers_[carrierFreq][src] = hrb;
                    hrb->insertPdu(cw,pdu);
    
                    /***
                     * TODO: because the mac selfMessage is triggered before the phy stack, the buffered LteMacPdu in harqRxBuffers_
                     * will not be handled until next TTI; we can do something here to reduce time waste
                     */
                }
            }
        }
        else if (userInfo->getFrameType() == RACPKT)
        {
            EV << NOW << " GnbMac::fromPhy - node " << nodeId_ << " Received RAC packet" << endl;
            macHandleRac(pkt);
        }
        else
        {
            throw cRuntimeError("Unknown packet type %d", (int)userInfo->getFrameType());
        }
    }
}


void GnbMac::macHandleRac(cPacket* pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);

    if(!strcmp(pkt->getName(), "SrvReq"))
    {
        auto uinfo = pkt->getTagForUpdate<UserControlInfo>();
        MacNodeId sourceId = uinfo->getSourceId();

        EV << NOW << " GnbMac::macHandleRac - Received Vehicle Service Request from node " << sourceId << endl;
        sendUpperPackets(pkt);
    }
    else
    {
        EV << NOW << " GnbMac::macHandleRac - Received LteRac." << endl;
        auto racPkt = pkt->removeAtFront<LteRac>();
        auto uinfo = pkt->getTagForUpdate<UserControlInfo>();

        gnbSchedulerUl_->signalRac(uinfo->getSourceId(), uinfo->getCarrierFrequency());

        // TODO all RACs are marked are successful
        racPkt->setSuccess(true);
        pkt->insertAtFront(racPkt);

        uinfo->setDestId(uinfo->getSourceId());
        uinfo->setSourceId(nodeId_);
        uinfo->setDirection(DL);

        sendLowerPackets(pkt);
    }

}


void GnbMac::macHandleFeedbackPkt(cPacket *pktAux)
{
    EV << "GnbMac::macHandleFeedbackPkt - handle feedback packet." << endl;

    auto pkt = check_and_cast<Packet *>(pktAux);
    auto fb = pkt->peekAtFront<LteFeedbackPkt>();
    auto lteInfo = pkt->getTag<UserControlInfo>();
    double carrierFreq = lteInfo->getCarrierFrequency();

    std::map<MacNodeId, LteFeedbackDoubleVector> fbMapD2D = fb->getLteFeedbackDoubleVectorD2D();

    // skip if no D2D CQI has been reported
    if (!fbMapD2D.empty())
    {
        EV << "GnbMac::macHandleFeedbackPkt - fbMapD2D is not empty, extract feedback for D2D links." << endl;

        //get Source Node Id<
        MacNodeId id = fb->getSourceNodeId();
        std::map<MacNodeId, LteFeedbackDoubleVector>::iterator mapIt;
        LteFeedbackDoubleVector::iterator it;
        LteFeedbackVector::iterator jt;

        // extract feedback for D2D links
        for (mapIt = fbMapD2D.begin(); mapIt != fbMapD2D.end(); ++mapIt)
        {
            MacNodeId peerId = mapIt->first;
            for (it = mapIt->second.begin(); it != mapIt->second.end(); ++it)
            {
                for (jt = it->begin(); jt != it->end(); ++jt)
                {
                    if (!jt->isEmptyFeedback())
                    {
                        amc_->pushFeedbackD2D(id, (*jt), peerId, carrierFreq);
                    }
                }
            }
        }
    }

    // ========= LteMacEnb =========
    //LteFeedbackPkt* fb = check_and_cast<LteFeedbackPkt*>(pkt);
    LteFeedbackDoubleVector fbMapDl = fb->getLteFeedbackDoubleVectorDl();
    LteFeedbackDoubleVector fbMapUl = fb->getLteFeedbackDoubleVectorUl();
    //get Source Node Id<
    MacNodeId id = fb->getSourceNodeId();

    LteFeedbackDoubleVector::iterator it;
    LteFeedbackVector::iterator jt;

    for (it = fbMapDl.begin(); it != fbMapDl.end(); ++it)
    {
        unsigned int i = 0;
        for (jt = it->begin(); jt != it->end(); ++jt)
        {
            //            TxMode rx=(TxMode)i;
            if (!jt->isEmptyFeedback())
            {
                amc_->pushFeedback(id, DL, (*jt), carrierFreq);
            }
            i++;
        }
    }
    for (it = fbMapUl.begin(); it != fbMapUl.end(); ++it)
    {
        for (jt = it->begin(); jt != it->end(); ++jt)
        {
            if (!jt->isEmptyFeedback())
                amc_->pushFeedback(id, UL, (*jt), carrierFreq);
        }
    }

    ueCarrierFreq_[id] = carrierFreq;
    double distance = phy_->getCoord().distance(lteInfo->getCoord());
    vecUpdateRsuFeedback(carrierFreq, id, lteInfo->isBroadcast(), distance);

    delete pkt;
}

void GnbMac::vecUpdateRsuFeedback(double carrierFreq, MacNodeId ueId, bool isBroadcast, double distance)
{
    EV << "GnbMac::vecUpdateRsuFeedback - update RSU status to Scheduler" << endl;

    // std::set<Band>& availableBands = carriersStatus_->getCarrierStatus(carrierFreq).availBands;
    if (rbManagerUl_->getFrequency() != carrierFreq)
    {
        EV << "GnbMac::vecUpdateRsuFeedback - multiple carriers are not supported yet" << endl;
        return;
    }

    // ===== use the same rate for all bands =====
    // std::set<Band>& availableBands = resAllocatorUl_->getAvailableBands();
    // UsableBands useableBands = UsableBands(availableBands.begin(), availableBands.end());
    // amc_->setPilotUsableBands(ueId, useableBands);

    // amc_->pushFeedback() in macHandleFeedbackPkt() has already reset the transmission parameters

    // TODO: check distance
    int bytePerBand = 0;
    int oldBytePerBand = rbManagerUl_->getVehDataRate(ueId);
    if (srsDistanceCheck_ && distance > srsDistance_)
    {
        EV << "GnbMac::vecUpdateRsuFeedback - distance " << distance << " is larger than SRS distance " << srsDistance_ 
            << ", set data rate to 0 for vehicle " << ueId << endl;
        rbManagerUl_->setVehDataRate(ueId, 0);
    }
    else
    {
        bytePerBand = amc_->computeBytesOnNRbs(ueId, Band(0), rbPerBand_, UL,carrierFreq);   // byte rate per TTI
        // int satisfiedBands = amc_->getTxParams(ueId, UL,carrierFreq).readBands().size();
        // EV << "GnbMac::vecUpdateRsuFeedback - number of satisfied bands: " << satisfiedBands << endl;
        // bytePerBand = bytePerBand * satisfiedBands / resAllocatorUl_->getNumBands();
        rbManagerUl_->setVehDataRate(ueId, bytePerBand);
        EV << "GnbMac::vecUpdateRsuFeedback - byte rate per each band per TTI: " << bytePerBand << endl;
    }

    // ===== handle broadcast feedback =====
    if (isBroadcast)
    {
        /***
         * in scheduleAll mode, all service are stopped before the broadcast feedback
         * in scheduleRemain mode, active apps are not stopped, all paused apps are terminated
         */
        EV << "GnbMac::vecUpdateRsuFeedback - broadcast feedback from vehicle " << ueId << endl;

        // first check all active apps
        set<AppId> activeSrv = rbManagerUl_->getScheduledApp();
        for (AppId appId : activeSrv)
        {
            if (MacCidToNodeId(appId) == ueId)
            {
                // broadcast feedback, service still running means in scheduleRemain mode
                int oldBands = rbManagerUl_->getAppAllocatedBands(appId);
                bool result = rbManagerUl_->scheduleActiveApp(appId);
                if (result)    // the granted bands are enough for the app
                {
                    vecServiceFeedback(appId, true);
                    int newBands = rbManagerUl_->getAppAllocatedBands(appId);
                    // only update offload grant to rsu if the data rate or band allocation has changed
                    if ((bytePerBand != oldBytePerBand) || (oldBands != newBands))  
                    {
                        vecSendGrantToVeh(appId, false, true, false, false);    // isNewGrant, isUpdate, isStop, isPause
                    }
                }
                else
                {
                    EV << "GnbMac::vecUpdateRsuFeedback - broadcast feedback, active app " << appId 
                        << " cannot be scheduled, terminate it." << endl;
                    terminateService(appId);
                }
            }
        }

        // stop all apps in appToBeInitialized_ (have not been initialized yet)
        set<AppId> appToBeInitialized = rbManagerUl_->getAppToBeInitialized();
        for (AppId appId : appToBeInitialized)
        {
            EV << "GnbMac::vecUpdateRsuFeedback - broadcast feedback, app " << appId 
                << " has not been initialized, terminate it." << endl;
            terminateService(appId);
        }

        // stop all paused apps
        set<AppId> pausedApps = rbManagerUl_->getPausedApp();
        for (AppId appId : pausedApps)
        {
            EV << "GnbMac::vecUpdateRsuFeedback - broadcast feedback, paused app " << appId 
                << " is terminated." << endl;
            terminateService(appId);
        }

        // if the data rate is not 0, update the latest data rate to the scheduler
        if (bytePerBand > 0)
        {
            Packet* packet = new Packet("RsuFD");
            auto rsuFd = makeShared<RsuFeedback>();
            rsuFd->setVehId(ueId);
            rsuFd->setGnbId(nodeId_);
            rsuFd->setServerPort(serverPort_);
            rsuFd->setFrequency(carrierFreq);
            rsuFd->setAvailBands(rbManagerUl_->getAvailableBands());
            rsuFd->setTotalBands(rbManagerUl_->getNumBands());
            rsuFd->setBytePerBand(rbManagerUl_->getVehDataRate(ueId));
            rsuFd->setBandUpdateTime(simTime());
            rsuFd->addTag<CreationTimeTag>()->setCreationTime(simTime());
            packet->insertAtBack(rsuFd);

            vecSendDataToServer(packet, ueId, serverPort_, gnbAddress_);
        }

        return;
    }

    /**
     * check the influence to the scheduled apps
     */
    set<AppId> activeSrv = rbManagerUl_->getScheduledApp();
    set<AppId> active2PausedSrv;
    for (AppId appId : activeSrv)
    {
        if (MacCidToNodeId(appId) == ueId)
        {
            int oldBands = rbManagerUl_->getAppAllocatedBands(appId);
            bool result = rbManagerUl_->scheduleActiveApp(appId);
            // if not broadcast, do not update information to the server and scheduler, do local adjustment only
            if (result)     // the granted bands are enough for the app
            {
                int newBands = rbManagerUl_->getAppAllocatedBands(appId);
                if ((bytePerBand != oldBytePerBand) || (oldBands != newBands))
                {
                    vecSendGrantToVeh(appId, false, true, false, false);    // isNewGrant, isUpdate, isStop, isPause
                }
            }
            else
            {
                // we want to check if the app can still be scheduled when considering the flexible bands later
                active2PausedSrv.insert(appId);
            }
        }
    }

    /***
     * check apps in appToBeInitialized_
     */
    set<AppId> appToBeInitialized = rbManagerUl_->getAppToBeInitialized();
    for (AppId appId : appToBeInitialized)
    {
        if (MacCidToNodeId(appId) == ueId)
        {
            bool result = rbManagerUl_->scheduleGrantedApp(appId);
            if (result)    // the granted bands are enough for the app
            {
                vecSendGrantToVeh(appId, false, true, false, false);    // isNewGrant, isUpdate, isStop, isPause
            }
        }
    }

    /**
     * check the influence to the paused apps
     * first check apps in active2PausedSrv, then check other paused apps
     */
    for (AppId appId : active2PausedSrv)
    {
        // when it comes to here, means not broadcast feedback (otherwise it is terminated in the above loop)
        bool result = rbManagerUl_->schedulePausedApp(appId);
        if (result)    // the granted bands are enough for the app
        {
            vecSendGrantToVeh(appId, false, true, false, false);    // isNewGrant, isUpdate, isStop, isPause
        }
        else
        {
            vecSendGrantToVeh(appId, false, false, false, true);    // isNewGrant, isUpdate, isStop, isPause
        }
    }
    // check other paused apps
    set<AppId> pausedApps = rbManagerUl_->getPausedApp();
    for (AppId appId : pausedApps)
    {
        // next schedule apps other than the active2PausedSrv
        if (active2PausedSrv.find(appId) == active2PausedSrv.end())
        {
            bool result = rbManagerUl_->schedulePausedApp(appId);
            if (result) // the granted bands are enough for the app
            {
                vecSendGrantToVeh(appId, false, true, false, false);    // isNewGrant, isUpdate, isStop, isPause
            }
        }
    }
}


void GnbMac::terminateService(AppId appId)
{
    EV << "GnbMac::terminateApp - terminate app " << appId << endl;

    // remove the app from the scheduler
    rbManagerUl_->terminateAppService(appId);
    vecServiceFeedback(appId, false);
    vecSendGrantToVeh(appId, false, false, true, false);    // isNewGrant, isUpdate, isStop, isPause

    rbManagerUl_->removeAppGrantInfo(appId);
    // appUdpHeader_.erase(appId);
    // appIpv4Header_.erase(appId);
}


void GnbMac::vecServiceFeedback(AppId appId, bool isSuccess)
{
    // TODO: implement
    EV << "GnbMac::vecNotifyServiceStatus - service for app " << appId << " is " << (isSuccess? "alive" : "stopped") << ", notify scheduler" << endl;
    AppGrantInfo & appGrantInfo = rbManagerUl_->getAppGrantInfo(appId);

    Packet* packet = new Packet("SrvFD");
    auto srvStatus = makeShared<ServiceStatus>();
    srvStatus->setAppId(appId);
    srvStatus->setOffloadGnbId(nodeId_);
    srvStatus->setProcessGnbId(appGrantInfo.processGnbId);
    srvStatus->setProcessGnbPort(appGrantInfo.processGnbPort);
    srvStatus->setSuccess(isSuccess);
    srvStatus->setAvailBand(rbManagerUl_->getAvailableBands());
    srvStatus->setOffloadGnbRbUpdateTime(simTime());
    if (isSuccess)
        srvStatus->setUsedBand(rbManagerUl_->getAppAllocatedBands(appId));
    else
        srvStatus->setUsedBand(0);
    srvStatus->addTag<CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtFront(srvStatus);

    MacNodeId ueId = MacCidToNodeId(appId);
    vecSendDataToServer(packet, ueId, appGrantInfo.processGnbPort, appGrantInfo.processGnbAddr);

    availableBands_ = rbManagerUl_->getAvailableBands();
}


void GnbMac::vecSendDataToServer(Packet* packet, MacNodeId ueId, int port, L3Address targetAddr)
{
    // manually create the udp header and ipv4 header in order
    // to transfer this packet to units outside the 5G core network
    inet::Ptr<UdpHeader> udpHeader = makeShared<UdpHeader>();
    udpHeader->setDestinationPort(port);
    udpHeader->setTotalLengthField(udpHeader->getChunkLength() + packet->getTotalLength());
    udpHeader->setCrcMode(CRC_DECLARED_CORRECT);
    udpHeader->setCrc(0xC00D);
    packet->insertAtFront(udpHeader);

    const auto& ipv4Header = makeShared<Ipv4Header>();
    ipv4Header->setProtocolId(IP_PROT_UDP);
    ipv4Header->setDestAddress(targetAddr.toIpv4());   // gnb address
    ipv4Header->setSrcAddress(binder_->getIPv4Address(ueId));   // vehicle address
    ipv4Header->addChunkLength(B(20));
    ipv4Header->setHeaderLength(B(20));
    ipv4Header->setTotalLengthField(ipv4Header->getChunkLength() + packet->getDataLength());
    packet->insertAtFront(ipv4Header);

    packet->addTagIfAbsent<FlowControlInfo>()->setRlcType(UM);

    sendUpperPackets(packet);
}

void GnbMac::handleUpperMessage(cPacket* pktAux)
{
    EV << "GnbMac::handleUpperMessage - handle packet from rlc stack." << endl;

    if (!strcmp(pktAux->getName(), "VehGrant"))
    {
        grantList_.push_back(pktAux);

        if (!ttiTick_->isScheduled())
        {
            int timeInt = ceil(NOW.dbl() / ttiPeriod_);
            scheduleAt(timeInt * ttiPeriod_, ttiTick_);
        }
        return;
    }
    
    auto pkt = check_and_cast<Packet *>(pktAux);
    auto lteInfo = pkt->getTag<FlowControlInfo>();
    MacCid cid = idToMacCid(lteInfo->getDestId(), lteInfo->getLcid());

    bool isLteRlcPduNewData = checkIfHeaderType<LteRlcPduNewData>(pkt);

    bool packetIsBuffered = bufferizePacket(pkt);  // will buffer (or destroy if queue is full)

    if(!isLteRlcPduNewData && packetIsBuffered) {
        // new MAC SDU has been received (was requested by MAC, no need to notify scheduler)
        // creates pdus from schedule list and puts them in harq buffers
        macPduMake(cid);
    } else if (isLteRlcPduNewData) {
        // new data - inform scheduler of active connection
        gnbSchedulerDl_->backlog(cid);
    }
}

void GnbMac::vecHandleGrantFromRsu(omnetpp::cPacket* pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    // auto ipv4Header = pkt->removeAtFront<Ipv4Header>();
    // auto udpHeader = pkt->removeAtFront<UdpHeader>();
    auto grant = pkt->peekAtFront<Grant2Veh>();
    AppId appId = grant->getAppId();
    MacNodeId vehId = MacCidToNodeId(appId);

    // check if this is the right offload gNB
    if (grant->getOffloadGnbId() != nodeId_)
    {
        // add simulation time as well
        throw cRuntimeError("GnbMac::handleGrantFromRsu - received grant for app %d but the offload gNB ID does not match, expected %d, received %d at time %s",
            appId, nodeId_, grant->getOffloadGnbId(), NOW.str().c_str());
    }

    // check if the this is a service stop grant
    if(grant->getGrantStop())
    {
        EV << "GnbMac::handleGrantFromRsu - received stop grant for app " << appId << endl;
        rbManagerUl_->terminateAppService(appId);
        vecServiceFeedback(appId, false);
        vecSendGrantToVeh(appId, false, false, true, false);    // isNewGrant, isUpdate, isStop, isPause

        rbManagerUl_->removeAppGrantInfo(appId);
        // appIpv4Header_.erase(appId);
        // appUdpHeader_.erase(appId);
        return;
    }

    omnetpp::simtime_t grantTime = grant->getMaxOffloadTime();
    int grantedNumBands = grant->getBands();
    EV << "GnbMac::handleGrantFromRsu - received grant for app " << appId << " with max offload time " 
            << grantTime << " and " << grantedNumBands << " bands." << endl;

    // store grant information
    AppGrantInfo appGrant;
    appGrant.appId = appId;
    appGrant.maxOffloadTime = grantTime;
    appGrant.numGrantedBands = grantedNumBands;
    appGrant.inputSize = grant->getInputSize();
    appGrant.outputSize = grant->getOutputSize();
    appGrant.ueId = vehId;
    appGrant.processGnbPort = grant->getProcessGnbPort();
    appGrant.offloadGnbId = grant->getOffloadGnbId();
    appGrant.processGnbId = grant->getProcessGnbId();
    // get the offloading gNodeB address and its server port
    cModule *processGnb = binder_->getModuleByMacNodeId(appGrant.processGnbId);
    appGrant.processGnbAddr = L3AddressResolver().resolve(processGnb->getFullName());

    rbManagerUl_->setAppGrantInfo(appId, appGrant);

    // check if the RSU has enough resources to grant the request
    bool schedulable = rbManagerUl_->scheduleGrantedApp(appId);
    // appIpv4Header_[appId] = makeShared<Ipv4Header>(*ipv4Header);
    // appUdpHeader_[appId] = makeShared<UdpHeader>(*udpHeader);
    vecServiceFeedback(appId, true);

    // ipv4Header = nullptr;
    // udpHeader = nullptr;

    if(!schedulable)    // send grant to veh but pause offloading temporarily
    {
        EV << "GnbMac::handleGrantFromRsu - grant for app " << appId 
            << " is not schedulable, added to the appToBeInitialized list." << endl;
        vecSendGrantToVeh(appId, false, false, false, true);    // isNewGrant, isUpdate, isStop, isPause
        rbManagerUl_->addAppToBeInitialized(appId);     // add the app to the appToBeInitialized list
    }
    else    // send grant to veh if the grant can be satisfied
    {
        EV << "GnbMac::handleGrantFromRsu - grant for app " << appId << " is schedulable, notify Veh." << endl;
        vecSendGrantToVeh(appId, true, false, false, false);    // isNewGrant, isUpdate, isStop, isPause
    }
}

void GnbMac::vecSendGrantToVeh(AppId appId, bool isNewGrant, bool isUpdate, bool isStop, bool isPause)
{
    EV << "GnbMac::vecSendGrantToVeh - send grant to vehicle, app " << appId << ", is new grant " << isNewGrant
        << ", is update " << isUpdate << ", is stop " << isStop << ", is pause " << isPause << endl;
    AppGrantInfo& srv = rbManagerUl_->getAppGrantInfo(appId);
    MacNodeId ueId = srv.ueId;
    
    Packet* pkt = new Packet("VehGrant");
    auto grant = makeShared<Grant2Veh>();
    grant->setAppId(appId);
    grant->setProcessGnbId(srv.processGnbId);
    grant->setOffloadGnbId(srv.offloadGnbId);
    grant->setProcessGnbPort(srv.processGnbPort);
    grant->setMaxOffloadTime(srv.maxOffloadTime);
    grant->setBands(srv.numGrantedBands);
    grant->setInputSize(srv.inputSize);
    grant->setOutputSize(srv.outputSize);
    grant->setNewGrant(isNewGrant);
    grant->setGrantUpdate(isUpdate);
    grant->setGrantStop(isStop);
    grant->setPause(isPause);

    EV << "\t processGnbId: " << srv.processGnbId << ", offloadGnbId: " << srv.offloadGnbId
       << ", processGnbPort: " << srv.processGnbPort << ", maxOffloadTime: " << srv.maxOffloadTime
       << ", grantedBands: " << srv.numGrantedBands << endl;

    if (isNewGrant || isUpdate)
    {
        int bytePerTTI = rbManagerUl_->getVehDataRate(ueId);
        int grantedBands = srv.grantedBandSet.size() + srv.tempBands.size();
        grant->setBytePerTTI(bytePerTTI * grantedBands);

        std::map<Band, unsigned int> rbMap;
        rbManagerUl_->readAppRbOccupation(appId, rbMap);
        grant->setGrantedBlocks(rbMap);
    }

    if(packetFlowManager_ != nullptr)
        packetFlowManager_->grantSent(ueId, grant->getChunkId());

    pkt->insertAtFront(grant);

    // manually create the udp header and ipv4 header in order
    unsigned int appPort = MacCidToLcid(appId);
    auto udpHeader = makeShared<UdpHeader>();
    udpHeader->setDestinationPort(appPort);
    udpHeader->setTotalLengthField(udpHeader->getChunkLength() + pkt->getTotalLength());
    udpHeader->setCrcMode(CRC_DECLARED_CORRECT);
    udpHeader->setCrc(0xC00D);
    pkt->insertAtFront(udpHeader);

    L3Address destAddr = binder_->getIPv4Address(ueId);
    auto ipv4Header = makeShared<Ipv4Header>();
    ipv4Header->setProtocolId(IP_PROT_UDP);
    ipv4Header->setDestAddress(destAddr.toIpv4());
    ipv4Header->addChunkLength(B(20));
    ipv4Header->setHeaderLength(B(20));
    ipv4Header->setTotalLengthField(ipv4Header->getChunkLength() + pkt->getDataLength());
    pkt->insertAtFront(ipv4Header);

    // add user control informatino to packet
    auto uinfo = pkt->addTag<UserControlInfo>();
    uinfo->setDestId(ueId);
    uinfo->setSourceId(nodeId_);
    uinfo->setDirection(UL);
    uinfo->setFrameType(GRANTPKT);
    uinfo->setCarrierFrequency(ueCarrierFreq_[ueId]);

    sendLowerPackets(pkt);
}

bool GnbMac::bufferizePacket(cPacket* pktAux)
{
    EV << "GnbMac::bufferizePacket - bufferize packet from rlc stack." << endl;

    auto pkt = check_and_cast<Packet *>(pktAux);

    if (pkt->getBitLength() <= 1) { // no data in this packet
        delete pktAux;
        return false;
    }

    pkt->setTimestamp();        // Add timestamp with current time to packet

    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    // obtain the cid from the packet informations
    MacCid cid = ctrlInfoToMacCid(lteInfo);

    // this packet is used to signal the arrival of new data in the RLC buffers
    if (checkIfHeaderType<LteRlcPduNewData>(pkt))
    {
        // update the virtual buffer for this connection

        // build the virtual packet corresponding to this incoming packet
        pkt->popAtFront<LteRlcPduNewData>();
        auto rlcSdu = pkt->peekAtFront<LteRlcSdu>();
        PacketInfo vpkt(rlcSdu->getLengthMainPacket(), pkt->getTimestamp());

        LteMacBufferMap::iterator it = macBuffers_.find(cid);
        if (it == macBuffers_.end())
        {
            LteMacBuffer* vqueue = new LteMacBuffer();
            vqueue->pushBack(vpkt);
            macBuffers_[cid] = vqueue;

            // make a copy of lte control info and store it to traffic descriptors map
            FlowControlInfo toStore(*lteInfo);
            connDesc_[cid] = toStore;
            // register connection to lcg map.
            LteTrafficClass tClass = (LteTrafficClass) lteInfo->getTraffic();

            lcgMap_.insert(LcgPair(tClass, CidBufferPair(cid, macBuffers_[cid])));

            EV << "\t LteMacBuffers : Using new buffer on node: " <<
            MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Bytes in the Queue: " <<
            vqueue->getQueueOccupancy() << "\n";
        }
        else
        {
            LteMacBuffer* vqueue = NULL;
            LteMacBufferMap::iterator it = macBuffers_.find(cid);
            if (it != macBuffers_.end())
                vqueue = it->second;

            if (vqueue != NULL)
            {
                vqueue->pushBack(vpkt);

                EV << "\t LteMacBuffers : Using old buffer on node: " <<
                MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
                vqueue->getQueueOccupancy() << "\n";
            }
            else
                throw cRuntimeError("GnbMac::bufferizePacket - cannot find mac buffer for cid %d", cid);
        }

        delete pkt;
        pkt = nullptr;

        return true; // this is only a new packet indication - only buffered in virtual queue
    }

    // this is a MAC SDU, bufferize it in the MAC buffer

    LteMacBuffers::iterator it = mbuf_.find(cid);
    if (it == mbuf_.end())
    {
        // Queue not found for this cid: create
        LteMacQueue* queue = new LteMacQueue(queueSize_);

        queue->pushBack(pkt);

        mbuf_[cid] = queue;

        EV << "\t LteMacBuffers : Using new buffer on node: " <<
        MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
        queue->getQueueSize() - queue->getByteLength() << "\n";
    }
    else
    {
        // Found
        LteMacQueue* queue = it->second;

        if (!queue->pushBack(pkt))
        {
            // unable to buffer packet (packet is not enqueued and will be dropped): update statistics
            EV << "\t LteMacBuffers : queue" << cid << " is full - cannot buffer packet " << pkt->getId()<< "\n";

            totalOverflowedBytes_ += pkt->getByteLength();
            double sample = (double)totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());

            if (lteInfo->getDirection() == DL)
                emit(macBufferOverflowDl_,sample);
            else
                emit(macBufferOverflowUl_,sample);

            EV << "\t LteMacBuffers : Dropped packet: queue" << cid << " is full\n";
            // @author Alessandro Noferi
            // discard the RLC
            if(packetFlowManager_ != nullptr)
            {
                unsigned int rlcSno = check_and_cast<LteRlcUmDataPdu *>(pkt)->getPduSequenceNumber();
                packetFlowManager_->discardRlcPdu(lteInfo->getLcid(),rlcSno);
            }

            // TODO add delete pkt (memory leak?)
            delete pkt;
            pkt = nullptr;
        }

        EV << "\t LteMacBuffers : Using old buffer on node: " <<
        MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
        queue->getQueueSize() - queue->getByteLength() << "\n";
    }

    return true;
}


void GnbMac::macPduMake(MacCid cid)
{
    EV << "----- START GnbMac::macPduMake -----\n";
    // Finalizes the scheduling decisions according to the schedule list,
    // detaching sdus from real buffers.

    macPduList_.clear();

    //  Build a MAC pdu for each scheduled user on each codeword
    std::map<double, LteMacScheduleList>::iterator cit = scheduleListDl_->begin();
    for (; cit != scheduleListDl_->end(); ++cit)
    {
        double carrierFreq = cit->first;
        LteMacScheduleList::const_iterator it;
        for (it = cit->second.begin(); it != cit->second.end(); it++)
        {
            Packet *macPacket = nullptr;
            MacCid destCid = it->first.first;

            if (destCid != cid)
                continue;

            // check whether the RLC has sent some data. If not, skip
            // (e.g. because the size of the MAC PDU would contain only MAC header - MAC SDU requested size = 0B)
            if (mbuf_[destCid]->getQueueLength() == 0)
                break;

            Codeword cw = it->first.second;
            MacNodeId destId = MacCidToNodeId(destCid);
            std::pair<MacNodeId, Codeword> pktId = std::pair<MacNodeId, Codeword>(destId, cw);
            unsigned int sduPerCid = it->second;
            unsigned int grantedBlocks = 0;
            TxMode txmode;

            if (macPduList_.find(carrierFreq) == macPduList_.end())
            {
                MacPduList newList;
                macPduList_[carrierFreq] = newList;
            }

            // Add SDUs to PDU
            auto pit = macPduList_[carrierFreq].find(pktId);

            // No packets for this user on this codeword
            if (pit == macPduList_[carrierFreq].end())
            {
                auto pkt = new Packet("LteMacPdu");
                pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
                pkt->addTagIfAbsent<UserControlInfo>()->setDestId(destId);
                pkt->addTagIfAbsent<UserControlInfo>()->setDirection(DL);
                pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFreq);

                const UserTxParams& txInfo = amc_->computeTxParams(destId, DL, carrierFreq);

                UserTxParams* txPara = new UserTxParams(txInfo);

                pkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(txPara);
                txmode = txInfo.readTxMode();
                RbMap rbMap;

                pkt->addTagIfAbsent<UserControlInfo>()->setTxMode(txmode);
                pkt->addTagIfAbsent<UserControlInfo>()->setCw(cw);

                grantedBlocks = gnbSchedulerDl_->readRbOccupation(destId, carrierFreq, rbMap);

                pkt->addTagIfAbsent<UserControlInfo>()->setGrantedBlocks(rbMap);
                pkt->addTagIfAbsent<UserControlInfo>()->setTotalGrantedBlocks(grantedBlocks);
                macPacket = pkt;

                auto macPkt = makeShared<LteMacPdu>();
                macPkt->setHeaderLength(MAC_HEADER);
                macPkt->addTagIfAbsent<CreationTimeTag>()->setCreationTime(NOW);
                macPacket->insertAtFront(macPkt);
                macPduList_[carrierFreq][pktId] = macPacket;
            }
            else
            {
                macPacket = pit->second;
            }

            while (sduPerCid > 0)
            {
                if ((mbuf_[destCid]->getQueueLength()) < (int) sduPerCid)
                {
                    throw cRuntimeError("Abnormal queue length detected while building MAC PDU for cid %d "
                        "Queue real SDU length is %d  while scheduled SDUs are %d",
                        destCid, mbuf_[destCid]->getQueueLength(), sduPerCid);
                }

                auto pkt = check_and_cast<Packet *>(mbuf_[destCid]->popFront());
                ASSERT(pkt != nullptr);

                drop(pkt);
                auto macPkt =  macPacket->removeAtFront<LteMacPdu>();
                macPkt->pushSdu(pkt);
                macPacket->insertAtFront(macPkt);
                sduPerCid--;
            }
        }
    }

    std::map<double, MacPduList>::iterator lit;
    for (lit = macPduList_.begin(); lit != macPduList_.end(); ++lit)
    {
        double carrierFreq = lit->first;
        if (harqTxBuffers_.find(carrierFreq) == harqTxBuffers_.end())
        {
            HarqTxBuffers newHarqTxBuffers;
            harqTxBuffers_[carrierFreq] = newHarqTxBuffers;
        }
        HarqTxBuffers& harqTxBuffers = harqTxBuffers_[carrierFreq];

        MacPduList::iterator pit;
        for (pit = lit->second.begin(); pit != lit->second.end(); ++pit)
        {
            MacNodeId destId = pit->first.first;
            Codeword cw = pit->first.second;

            LteHarqBufferTx* txBuf;
            auto hit = harqTxBuffers.find(destId);
            if (hit != harqTxBuffers.end())
            {
                txBuf = hit->second;
            }
            else
            {
                // FIXME: possible memory leak
                LteHarqBufferTx* hb = new LteHarqBufferTx(ENB_TX_HARQ_PROCESSES,
                    this,(LteMacBase*)getMacUe(destId));
                harqTxBuffers[destId] = hb;
                txBuf = hb;
            }
            UnitList txList = (txBuf->firstAvailable());

            auto macPacket = pit->second;
            auto header = macPacket->peekAtFront<LteMacPdu>();
            EV << "GnbMac::macPduMake - created PDU: " << macPacket->str() << endl;

            // pdu transmission here (if any)
            if (txList.second.empty())
            {
                EV << "macPduMake() : no available process for this MAC pdu in TxHarqBuffer" << endl;
                delete macPacket;
            }
            else
            {
                if (txList.first == HARQ_NONE)
                    throw cRuntimeError("GnbMac::macPduMake - sending to uncorrect void H-ARQ process. Aborting");
                txBuf->insertPdu(txList.first, cw, macPacket);
            }
        }
    }
    EV << "------ END GnbMac::macPduMake ------\n";
}


void GnbMac::sendGrants(std::map<double, LteMacScheduleList>* scheduleList)
{
    EV << NOW << " GnbMac::sendGrants " << endl;

    std::map<double, LteMacScheduleList>::iterator cit = scheduleList->begin();
    for (; cit != scheduleList->end(); ++cit)
    {
        LteMacScheduleList& carrierScheduleList = cit->second;
        while (!carrierScheduleList.empty())
        {
            LteMacScheduleList::iterator it, ot;
            it = carrierScheduleList.begin();

            Codeword cw = it->first.second;
            Codeword otherCw = MAX_CODEWORDS - cw;
            MacCid cid = it->first.first;
            LogicalCid lcid = MacCidToLcid(cid);
            MacNodeId nodeId = MacCidToNodeId(cid);
            unsigned int granted = it->second;
            unsigned int codewords = 0;

            // removing visited element from scheduleList.
            carrierScheduleList.erase(it);

            if (granted > 0)
            {
                // increment number of allocated Cw
                ++codewords;
            }
            else
            {
                // active cw becomes the "other one"
                cw = otherCw;
            }

            std::pair<unsigned int, Codeword> otherPair(nodeId, otherCw);

            if ((ot = (carrierScheduleList.find(otherPair))) != (carrierScheduleList.end()))
            {
                // increment number of allocated Cw
                ++codewords;

                // removing visited element from scheduleList.
                carrierScheduleList.erase(ot);
            }

            if (granted == 0)
                continue; // avoiding transmission of 0 grant (0 grant should not be created)

            EV << NOW << " GnbMac::sendGrants Node[" << getMacNodeId() << "] - "
               << granted << " blocks to grant for user " << nodeId << " on "
               << codewords << " codewords. CW[" << cw << "\\" << otherCw << "] carrier[" << cit->first << "]" << endl;

            // get the direction of the grant, depending on which connection has been scheduled by the eNB
            Direction dir = (lcid == D2D_MULTI_SHORT_BSR) ? D2D_MULTI : ((lcid == D2D_SHORT_BSR) ? D2D : UL);

            // TODO Grant is set aperiodic as default
            // TODO: change to tag instead of header
            auto pkt = new Packet("LteGrant");
            auto grant = makeShared<LteSchedulingGrant>();
            grant->setDirection(dir);
            grant->setCodewords(codewords);

            // set total granted blocks
            grant->setTotalGrantedBlocks(granted);
            grant->setChunkLength(b(1));

            pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
            pkt->addTagIfAbsent<UserControlInfo>()->setDestId(nodeId);
            pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(GRANTPKT);
            pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(cit->first);

            const UserTxParams& ui = getAmc()->computeTxParams(nodeId, dir, cit->first);
            UserTxParams* txPara = new UserTxParams(ui);
            // FIXME: possible memory leak
            grant->setUserTxParams(txPara);

            // acquiring remote antennas set from user info
            const std::set<Remote>& antennas = ui.readAntennaSet();
            std::set<Remote>::const_iterator antenna_it = antennas.begin(),
            antenna_et = antennas.end();

            // get bands for this carrier
            const unsigned int firstBand = cellInfo_->getCarrierStartingBand(cit->first);
            const unsigned int lastBand = cellInfo_->getCarrierLastBand(cit->first);

            //  HANDLE MULTICW
            for (; cw < codewords; ++cw)
            {
                unsigned int grantedBytes = 0;

                for (Band b = firstBand; b <= lastBand; ++b)
                {
                    unsigned int bandAllocatedBlocks = 0;
                   // for (; antenna_it != antenna_et; ++antenna_it) // OLD FOR
                   for (antenna_it = antennas.begin(); antenna_it != antenna_et; ++antenna_it)
                   {
                       bandAllocatedBlocks += gnbSchedulerUl_->readPerUeAllocatedBlocks(nodeId,*antenna_it, b);
                   }
                   grantedBytes += amc_->computeBytesOnNRbs(nodeId, b, cw, bandAllocatedBlocks, dir, cit->first);
                }

                grant->setGrantedCwBytes(cw, grantedBytes);
                EV << NOW << " GnbMac::sendGrants - granting " << grantedBytes << " on cw " << cw << endl;
            }
            RbMap map;

            gnbSchedulerUl_->readRbOccupation(nodeId, cit->first, map);

            grant->setGrantedBlocks(map);

            /*
             * @author Alessandro Noferi
             * Notify the pfm about the successful arrival of a TB from a UE.
             * From ETSI TS 138314 V16.0.0 (2020-07)
             *   tSched: the point in time when the UL MAC SDU i is scheduled as
             *   per the scheduling grant provided
             */
            if(packetFlowManager_ != nullptr)
                packetFlowManager_->grantSent(nodeId, grant->getGrandId());

            // send grant to PHY layer
            pkt->insertAtFront(grant);
            sendLowerPackets(pkt);
        }
    }
}


void GnbMac::macSduRequest()
{
    EV << "----- START GnbMac::macSduRequest -----\n";

    // Ask for a MAC sdu for each scheduled user on each carrier and each codeword
    std::map<double, LteMacScheduleList>::iterator cit;
    for (cit = scheduleListDl_->begin(); cit != scheduleListDl_->end(); cit++)
    {   // loop on carriers

        LteMacScheduleList::const_iterator it;
        for (it = cit->second.begin(); it != cit->second.end(); it++)
        {   // loop on cids
            MacCid destCid = it->first.first;
            // Codeword cw = it->first.second;
            MacNodeId destId = MacCidToNodeId(destCid);

            // for each band, count the number of bytes allocated for this ue (dovrebbe essere per cid)
            unsigned int allocatedBytes = 0;
            int numBands = cellInfo_->getNumBands();
            for (Band b=0; b < numBands; b++)
            {
                // get the number of bytes allocated to this connection
                // (this represents the MAC PDU size)
                allocatedBytes += gnbSchedulerDl_->allocator_->getBytes(MACRO,b,destId);
            }

            // send the request message to the upper layer
            auto pkt = new Packet("LteMacSduRequest");
            auto macSduRequest = makeShared<LteMacSduRequest>();
            macSduRequest->setUeId(destId);
            macSduRequest->setChunkLength(b(1)); // TODO: should be 0
            macSduRequest->setUeId(destId);
            macSduRequest->setLcid(MacCidToLcid(destCid));
            macSduRequest->setSduSize(allocatedBytes - MAC_HEADER);    // do not consider MAC header size
            pkt->insertAtFront(macSduRequest);
            if (queueSize_ != 0 && queueSize_ < macSduRequest->getSduSize()) {
                throw cRuntimeError("GnbMac::macSduRequest: configured queueSize too low - requested SDU will not fit in queue!"
                        " (queue size: %d, sdu request requires: %d)", queueSize_, macSduRequest->getSduSize());
            }
            auto tag = pkt->addTag<FlowControlInfo>();
            *tag = connDesc_[destCid];
            sendUpperPackets(pkt);
        }
    }
    EV << "------ END GnbMac::macSduRequest ------\n";
}


void GnbMac::flushHarqBuffers()
{
    EV << "GnbMac::flushHarqBuffers - selfMessage flushHarqMsg." << endl;

    std::map<double, HarqTxBuffers>::iterator mit = harqTxBuffers_.begin();
    std::map<double, HarqTxBuffers>::iterator met = harqTxBuffers_.end();
    for (; mit != met; mit++)
    {
        HarqTxBuffers::iterator it;
        for (it = mit->second.begin(); it != mit->second.end(); it++)
            it->second->sendSelectedDown();
    }

    // flush mirror buffer
    std::map<double, HarqBuffersMirrorD2D>::iterator mirr_mit = harqBuffersMirrorD2D_.begin();
    std::map<double, HarqBuffersMirrorD2D>::iterator mirr_met = harqBuffersMirrorD2D_.end();
    for (; mirr_mit != mirr_met; mirr_mit++)
    {
        HarqBuffersMirrorD2D::iterator mirr_it;
        for (mirr_it = mirr_mit->second.begin(); mirr_it != mirr_mit->second.end(); mirr_it++)
            mirr_it->second->markSelectedAsWaiting();
    }
}

void GnbMac::flushAppPduList()
{
    EV << "GnbMac::flushAppPduList - selfMessage flushAppPduList." << endl;

    for (auto it = appPduList_.begin(); it != appPduList_.end(); ++it)
    {
        // AppId appId = it->first;
        // MacNodeId ueId = MacCidToNodeId(appId);
        // double carrierFreq = ueCarrierFreq_[ueId];
        // if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
        //     continue;

        macPduUnmake(it->second);
    }

    appPduList_.clear();
}

void GnbMac::signalProcessForRtx(MacNodeId nodeId, double carrierFrequency, Direction dir, bool rtx)
{
    std::map<double, int>* needRtx = (dir == DL) ? &needRtxDl_ : (dir == UL) ? &needRtxUl_ :
            (dir == D2D) ? &needRtxD2D_ : throw cRuntimeError("GnbMac::signalProcessForRtx - direction %d not valid\n", dir);

    if (needRtx->find(carrierFrequency) == needRtx->end())
    {
        if (!rtx)
            return;

        std::pair<double,int> p(carrierFrequency, 0);
        needRtx->insert(p);
    }

    if (!rtx)
        (*needRtx)[carrierFrequency]--;
    else
        (*needRtx)[carrierFrequency]++;
}

int GnbMac::getProcessForRtx(double carrierFrequency, Direction dir)
{
    std::map<double, int>* needRtx = (dir == DL) ? &needRtxDl_ : (dir == UL) ? &needRtxUl_ :
            (dir == D2D) ? &needRtxD2D_ : throw cRuntimeError("GnbMac::getProcessForRtx - direction %d not valid\n", dir);

    if (needRtx->find(carrierFrequency) == needRtx->end())
        return 0;

    return needRtx->at(carrierFrequency);
}


