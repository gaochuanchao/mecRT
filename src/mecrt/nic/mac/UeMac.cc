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

#include "mecrt/nic/mac/UeMac.h"
#include "mecrt/packets/apps/VecPacket_m.h"
#include "mecrt/packets/nic/VecDataInfo_m.h"
#include "mecrt/nic/phy/UePhy.h"

#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/NetworkInterface.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"

#include "stack/rlc/packet/LteRlcDataPdu.h"

#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/buffer/harq_d2d/LteHarqBufferRxD2D.h"
#include "stack/mac/buffer/LteMacQueue.h"
#include "stack/mac/packet/LteSchedulingGrant.h"
#include "stack/mac/packet/LteRac_m.h"
#include "stack/mac/packet/LteMacSduRequest.h"
#include "stack/mac/scheduler/LteSchedulerUeUl.h"

#include "stack/packetFlowManager/PacketFlowManagerBase.h"
#include "corenetwork/statsCollector/UeStatsCollector.h"

Define_Module(UeMac);

using namespace omnetpp;
using namespace inet;

UeMac::UeMac() 
{ 
    ttiTick_ = nullptr; 
    enableInitDebug_ = false;
    nodeInfo_ = nullptr;
}

UeMac::~UeMac()
{
    if (enableInitDebug_)
        std::cout << "UeMac::~UeMac - destroying MAC protocol" << std::endl;

    if (ttiTick_)
    {
        cancelAndDelete(ttiTick_);
        ttiTick_ = nullptr;
    }
    
    if (enableInitDebug_)
        std::cout << "UeMac::~UeMac - destroying MAC protocol done!\n";
}

void UeMac::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
        if (enableInitDebug_)
            std::cout << "UeMac::initialize - stage: INITSTAGE_LOCAL - begins" << std::endl;

        /* Gates initialization */
        up_[IN_GATE] = gate("RLC_to_MAC");
        up_[OUT_GATE] = gate("MAC_to_RLC");
        down_[IN_GATE] = gate("PHY_to_MAC");
        down_[OUT_GATE] = gate("MAC_to_PHY");

        /* Create buffers */
        queueSize_ = par("queueSize");  // default(2MiB);  // MAC Buffers queue size

        /* Get reference to binder */
        binder_ = getBinder();

        /* Set The MAC MIB */

        muMimo_ = par("muMimo");    // default(true)
        harqProcesses_ = par("harqProcesses");  // default(8)

        /* statistics */
        statDisplay_ = par("statDisplay");  // default(false)

        totalOverflowedBytes_ = 0;
        nrFromUpper_ = 0;
        nrFromLower_ = 0;
        nrToUpper_ = 0;
        nrToLower_ = 0;

        if(strcmp(this->getName(), "nrMac") == 0)
        {
            if(getParentModule()->findSubmodule("nrPacketFlowManager") != -1)
            {
                EV << "UeMac::initialize - MAC layer is NRMac, cast the packetFlowManager to NR" << endl;
                packetFlowManager_ = check_and_cast<PacketFlowManagerBase *>(getParentModule()->getSubmodule("nrPacketFlowManager"));
            }
        }
        else{
            if(getParentModule()->findSubmodule("packetFlowManager") != -1)
            {
                EV << "UeMac::initialize - MAC layer, nodeType: UE" << endl;
                packetFlowManager_ = check_and_cast<PacketFlowManagerBase *>(getParentModule()->getSubmodule("packetFlowManager"));
            }
        }

        /* register signals */
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
        WATCH_MAP(mbuf_);
        WATCH_MAP(macBuffers_);

        // =========== LteMacUe ===========
        cqiDlMuMimo0_ = registerSignal("cqiDlMuMimo0");
        cqiDlMuMimo1_ = registerSignal("cqiDlMuMimo1");
        cqiDlMuMimo2_ = registerSignal("cqiDlMuMimo2");
        cqiDlMuMimo3_ = registerSignal("cqiDlMuMimo3");
        cqiDlMuMimo4_ = registerSignal("cqiDlMuMimo4");

        cqiDlTxDiv0_ = registerSignal("cqiDlTxDiv0");
        cqiDlTxDiv1_ = registerSignal("cqiDlTxDiv1");
        cqiDlTxDiv2_ = registerSignal("cqiDlTxDiv2");
        cqiDlTxDiv3_ = registerSignal("cqiDlTxDiv3");
        cqiDlTxDiv4_ = registerSignal("cqiDlTxDiv4");

        cqiDlSpmux0_ = registerSignal("cqiDlSpmux0");
        cqiDlSpmux1_ = registerSignal("cqiDlSpmux1");
        cqiDlSpmux2_ = registerSignal("cqiDlSpmux2");
        cqiDlSpmux3_ = registerSignal("cqiDlSpmux3");
        cqiDlSpmux4_ = registerSignal("cqiDlSpmux4");

        cqiDlSiso0_ = registerSignal("cqiDlSiso0");
        cqiDlSiso1_ = registerSignal("cqiDlSiso1");
        cqiDlSiso2_ = registerSignal("cqiDlSiso2");
        cqiDlSiso3_ = registerSignal("cqiDlSiso3");
        cqiDlSiso4_ = registerSignal("cqiDlSiso4");

        // =========== LteMacUeD2D ===========
        // check the RLC module type: if it is not "D2D", abort simulation
        cModule* rlc = getParentModule()->getSubmodule("rlc");  // LteRlc
        bool rlcD2dCapable = rlc->par("d2dCapable").boolValue();    // default(true), specified in UeNic.ned
        std::string rlcUmType = rlc->par("umType").stdstringValue();    // d2dCapable ? "LteRlcUmD2D" : LteRlcUmType;
        if (rlcUmType.compare("LteRlcUmD2D") != 0 || !rlcD2dCapable)
            throw cRuntimeError("UeMac::initialize - %s module found, must be LteRlcUmD2D. Aborting", rlcUmType.c_str());

        std::string pdcpType = getParentModule()->par("LtePdcpRrcType").stdstringValue();   // default("NRPdcpRrcUe")
        if (pdcpType.compare("LtePdcpRrcUeD2D") != 0 && pdcpType.compare("NRPdcpRrcUe") != 0 )
            throw cRuntimeError("UeMac::initialize - %s module found, must be LtePdcpRrcUeD2D or NRPdcpRrcUe. Aborting", pdcpType.c_str());

        rcvdD2DModeSwitchNotification_ = registerSignal("rcvdD2DModeSwitchNotification");

        if (enableInitDebug_)
            std::cout << "UeMac::initialize - stage: INITSTAGE_LOCAL - ends" << std::endl;
    }
    else if (stage == INITSTAGE_LINK_LAYER)
    {
        if (enableInitDebug_)
            std::cout << "UeMac::initialize - stage: INITSTAGE_LINK_LAYER - begins" << std::endl;
        
        // =========== LteMacUe ===========
        EV << "UeMac::initialize - MAC layer, stage INITSTAGE_LINK_LAYER" << endl;

        nodeInfo_ = getModuleFromPar<NodeInfo>(getAncestorPar("nodeInfoModulePath"), this);

        resAllocateMode_ = par("resAllocateMode");

        /***
         * the "nrMasterId" refers to the macNodeId of the gNB that this UE linked to.
         * the default value of "nrMasterId" is 0, specified in NRUe.ned;
         * this value is override in omnetpp.ini, with value nrMasterId = 1
         */
        if (strcmp(getFullName(),"nrMac") == 0)
            cellId_ = getAncestorPar("nrMasterId");
        else
            cellId_ = getAncestorPar("masterId");

        if (enableInitDebug_)
            std::cout << "UeMac::initialize - stage: INITSTAGE_LINK_LAYER - ends" << std::endl;
    }
    else if (stage == INITSTAGE_NETWORK_LAYER)
    {
        if (enableInitDebug_)
            std::cout << "UeMac::initialize - stage: INITSTAGE_NETWORK_LAYER - begins" << std::endl;

        // =========== LteMacUe ===========
        /***
         * the default value of NRUe->par("nrMacNodeId") is 0, specified in NRUe.ned;
         * its value is updated in IP2Nic::initialize(), which calls Binder::registerNode().
         * in Binder::registerNode(), the nrMacNodeId of NRUe is set started from value 2049, i.e., 2049, 2050, ...;
         * if there is only one NRUe, its corresponding NRUe->par("nrMacNodeId") will be set as 2049.
         * because IP2Nic is initialized before UeMac, the value of nodeId_ will be 2049.
         */
        if (strcmp(getFullName(),"nrMac") == 0)
            nodeId_ = getAncestorPar("nrMacNodeId");
        else
            nodeId_ = getAncestorPar("macNodeId");

        nodeInfo_->setNodeId(nodeId_);

        /* Insert UeInfo in the Binder */
        UeInfo* info = new UeInfo();
        info->id = nodeId_;            // local mac ID
        info->cellId = cellId_;        // cell ID
        info->init = false;            // flag for phy initialization
        info->ue = this->getParentModule()->getParentModule();  // reference to the UE module

        // get the reference to the PHY layer
        if (isNrUe(nodeId_))
            info->phy = check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("nrPhy"));
        else
            info->phy = check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("phy"));

        phy_ = info->phy;
        binder_->addUeInfo(info);

        if (resAllocateMode_)
        {
            std::vector<EnbInfo *>* gnbList = binder_->getEnbList();
            for (auto it = gnbList->begin(); it != gnbList->end(); ++it)
            {
                MacNodeId gnbId = (*it)->id;   // refers to the base station macNodeId
                attachToGnb(gnbId);
            }
        }
        else
        {
            attachToGnb(cellId_);
        }

        // find interface entry and use its address
        IInterfaceTable *interfaceTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        NetworkInterface * iface = interfaceTable->findInterfaceByName(par("interfaceName").stringValue());
        if(iface == nullptr)
            throw new cRuntimeError("no interface entry for lte interface - cannot bind node %i", nodeId_);

        auto ipv4if = iface->getProtocolData<Ipv4InterfaceData>();
        if(ipv4if == nullptr)
            throw new cRuntimeError("no Ipv4 interface data - cannot bind node %i", nodeId_);
        binder_->setMacNodeId(ipv4if->getIPAddress(), nodeId_);
        nodeInfo_->setNodeAddr(ipv4if->getIPAddress());
        
        // for emulation mode
        const char* extHostAddress = getAncestorPar("extHostAddress").stringValue();
        if (strcmp(extHostAddress, "") != 0)
        {
            // register the address of the external host to enable forwarding
            binder_->setMacNodeId(Ipv4Address(extHostAddress), nodeId_);
        }

        // =========== LteMacUeD2D ===========
        // get parameters
        usePreconfiguredTxParams_ = par("usePreconfiguredTxParams");

        if (cellId_ > 0)
        {
            preconfiguredTxParams_ = getPreconfiguredTxParams();
            // get the reference to the eNB
            enb_ = check_and_cast<LteMacEnbD2D*>(getMacByMacNodeId(cellId_));
        }
        else
            enb_ = NULL;

        if (enableInitDebug_)
            std::cout << "UeMac::initialize - stage: INITSTAGE_NETWORK_LAYER - ends" << std::endl;
    }
    else if (stage == inet::INITSTAGE_TRANSPORT_LAYER)
    {
        if (enableInitDebug_)
            std::cout << "UeMac::initialize - stage: INITSTAGE_TRANSPORT_LAYER - begins" << std::endl;

        // =========== LteMacUe ===========
        const std::map<double, LteChannelModel*>* channelModels = phy_->getChannelModels();
        std::map<double, LteChannelModel*>::const_iterator it = channelModels->begin();
        for (; it != channelModels->end(); ++it)
        {
            lcgScheduler_[it->first] = new LteSchedulerUeUl(this, it->first);
        }

        if (enableInitDebug_)
            std::cout << "UeMac::initialize - stage: INITSTAGE_TRANSPORT_LAYER - ends" << std::endl;
    }
    else if (stage == inet::INITSTAGE_LAST)
    {
        if (enableInitDebug_)
            std::cout << "UeMac::initialize - stage: INITSTAGE_LAST - begins" << std::endl;

        // =========== LteMacUe ===========
        /* Start TTI tick */
        ttiTick_ = new cMessage("ttiTick_");
        ttiTick_->setSchedulingPriority(1);    // TTI TICK after other messages

        if (!isNrUe(nodeId_))
        {
            // if this MAC layer refers to the LTE side of the UE, then the TTI is equal to 1ms
            ttiPeriod_ = TTI;
        }
        else
        {
            // otherwise, the period is equal to the minimum period according to the numerologies used by the carriers in this NR node
            ttiPeriod_ = binder_->getSlotDurationFromNumerologyIndex(binder_->getUeMaxNumerologyIndex(nodeId_));

            // for each numerology available in this UE, set the corresponding timers
            const std::set<NumerologyIndex>* numerologyIndexSet = binder_->getUeNumerologyIndex(nodeId_);
            if (numerologyIndexSet != NULL)
            {
                std::set<NumerologyIndex>::const_iterator it = numerologyIndexSet->begin();
                for ( ; it != numerologyIndexSet->end(); ++it)
                {
                    // set periodicity for this carrier according to its numerology
                    NumerologyPeriodCounter info;
                    info.max = 1 << (binder_->getUeMaxNumerologyIndex(nodeId_) - *it); // 2^(maxNumerologyIndex - numerologyIndex)
                    info.current = info.max - 1;
                    numerologyPeriodCounter_[*it] = info;
                }
            }
        }

        mobility_ = check_and_cast<MecMobility*>(getParentModule()->getParentModule()->getSubmodule("mobility"));
        moveStartTime_ = mobility_->getMoveStartTime();
        moveStoptime_ = mobility_->getMoveStopTime();

        if (enableInitDebug_)
            std::cout << "UeMac::initialize - stage: INITSTAGE_LAST - ends" << std::endl;
    }
}


void UeMac::attachToGnb(MacNodeId gnbId)
{
    if (gnbId > 0)
    {
        LteAmc *amc = check_and_cast<LteMacEnb *>(getMacByMacNodeId(gnbId))->getAmc();
        amc->attachUser(nodeId_, UL);
        amc->attachUser(nodeId_, DL);
        amc->attachUser(nodeId_, D2D);

        /*
        * @autor Alessandro Noferi
        *
        * This piece of code connects the UeCollector to the relative base station Collector.
        * It checks the NIC, i.e. Lte or NR and chooses the correct UeCollector to connect.             *
        */

        cModule *module = binder_->getModuleByPath(binder_->getModuleNameByMacNodeId(gnbId));
        std::string nodeType;
        if(module->hasPar("nodeType"))
            nodeType = module->par("nodeType").stdstringValue();

        RanNodeType eNBType = binder_->getBaseStationTypeById(gnbId);

        if(isNrUe(nodeId_) &&  eNBType == GNODEB)
        {
            EV << "I am a NR Ue with node id: " << nodeId_ << " connected to gnb with id: "<< gnbId << endl;
            if(getParentModule()->getParentModule()->findSubmodule("NRueCollector") != -1)
            {
                UeStatsCollector *ue = check_and_cast<UeStatsCollector *> (getParentModule()->getParentModule()->getSubmodule("NRueCollector"));
                binder_->addUeCollectorToEnodeB(nodeId_, ue,gnbId);
            }
        }
        else if (!isNrUe(nodeId_) && eNBType == ENODEB)
        {
            EV << "I am an LTE Ue with node id: " << nodeId_ << " connected to gnb with id: "<< gnbId << endl;
            if(getParentModule()->getParentModule()->findSubmodule("ueCollector") != -1)
            {
                UeStatsCollector *ue = check_and_cast<UeStatsCollector *> (getParentModule()->getParentModule()->getSubmodule("ueCollector"));
                binder_->addUeCollectorToEnodeB(nodeId_, ue, gnbId);
            }
        }
        else
        {
            EV << "I am a UE with node id: " << nodeId_ << " and the base station with id: "<< gnbId << " has a different type" <<  endl;
        }

        ////
    }
}


void UeMac::handleMessage(cMessage* msg)
{
    if (msg->isSelfMessage())
    {
        if (simTime() >= moveStoptime_)
        {
            EV << "UeMac::handleMessage - stop traffic for node " << nodeId_ << "!" << endl;
            return;
        }

        // ========== LteMacUe ==========
        if (strcmp(msg->getName(), "flushHarqMsg") == 0)
        {
            EV << "UeMac::handleMessage - selfMessage: flushHarqMsg" << endl;

            flushHarqBuffers();
            delete msg;
            return;
        }
        
        if (strcmp(msg->getName(), "flushAppMsg") == 0)
        {
            EV << "UeMac::handleMessage - selfMessage: flushAppMsg" << endl;
            vecFlushAppPduList();
            delete msg;
            return;
        }

        // ========== LteMacBase ==========
        // if (resAllocateMode_)
        //     vecHandleSelfMessage();
        // else
        //     handleSelfMessage();

        // scheduleAt(NOW + ttiPeriod_, ttiTick_);

        vecHandleSelfMessage();

        return;
    }

    // ========== LteMacUeD2D ==========
    auto pkt = check_and_cast<inet::Packet *>(msg);
    EV << "UeMac::handleMessage - Received packet " << pkt->getName() <<
        " from port " << pkt->getArrivalGate()->getName() << endl;

    if (simTime() >= moveStoptime_)
    {
        EV << "UeMac::handleMessage - stop traffic for node " << nodeId_ << "!" << endl;
        delete msg;
        return;
    }

    cGate* incoming = pkt->getArrivalGate();

    if (incoming == down_[IN_GATE])
    {
        auto userInfo = pkt->getTag<UserControlInfo>();

        if (userInfo->getFrameType() == D2DMODESWITCHPKT)
        {
            EV << "UeMac::handleMessage - Received frame type: D2DMODESWITCHPKT" << endl;

            // message from PHY_to_MAC gate (from lower layer)
            emit(receivedPacketFromLowerLayer, pkt);

            // call handler
            macHandleD2DModeSwitch(pkt);

            return;
        }
    }

    // ========== LteMacBase ==========
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
        handleUpperMessage(pkt);
        // fromRlc(pkt);
    }
}

void UeMac::flushHarqBuffers()
{
    EV << "UeMac::flushHarqBuffers - flushing hardTxbuffer" << endl;

    // send the selected units to lower layers
    std::map<double, HarqTxBuffers>::iterator mtit;
    for (mtit = harqTxBuffers_.begin(); mtit != harqTxBuffers_.end(); ++mtit)
    {
        HarqTxBuffers::iterator it2;
        for(it2 = mtit->second.begin(); it2 != mtit->second.end(); it2++)
            it2->second->sendSelectedDown();
    }

    // deleting non-periodic grant
    auto git = schedulingGrant_.begin();
    for (; git != schedulingGrant_.end(); ++git)
    {
        if (git->second != nullptr && !(git->second->getPeriodic()))
        {
            git->second=nullptr;
        }
    }
}

void UeMac::vecFlushAppPduList()
{
    EV << "UeMac::vecFlushAppPduList - flushing app pdu list" << endl;

    for (auto it = appPduList_.begin(); it != appPduList_.end(); ++it)
    {
        sendLowerPackets(it->second);
    }

    appPduList_.clear();
}


/*
 * Lower layer handler
 */
void UeMac::fromPhy(cPacket *pktAux)
{
    // TODO: harq test (comment fromPhy: it has only to pass pdus to proper rx buffer and
    // to manage H-ARQ feedback)

    auto pkt = check_and_cast<inet::Packet*> (pktAux);
    auto userInfo = pkt->getTag<UserControlInfo>();

    MacNodeId src = userInfo->getSourceId();
    double carrierFreq = userInfo->getCarrierFrequency();

    if (userInfo->getFrameType() == HARQPKT)
    {
        if (harqTxBuffers_.find(carrierFreq) == harqTxBuffers_.end())
        {
            HarqTxBuffers newTxBuffs;
            harqTxBuffers_[carrierFreq] = newTxBuffs;
        }

        // H-ARQ feedback, send it to TX buffer of source
        HarqTxBuffers::iterator htit = harqTxBuffers_[carrierFreq].find(src);
        EV << NOW << " UeMac::fromPhy - node " << nodeId_ << " Received HARQ Feedback pkt" << endl;
        if (htit == harqTxBuffers_[carrierFreq].end())
        {
            // if a feedback arrives, a tx buffer must exists (unless it is an handover scenario
            // where the harq buffer was deleted but a feedback was in transit)
            // this case must be taken care of

            if (binder_->hasUeHandoverTriggered(nodeId_) || binder_->hasUeHandoverTriggered(src))
                return;

            throw cRuntimeError("Mac::fromPhy(): Received feedback for an unexisting H-ARQ tx buffer");
        }

        auto hfbpkt = pkt->peekAtFront<LteHarqFeedback>();
        htit->second->receiveHarqFeedback(pkt);
    }
    else if (userInfo->getFrameType() == FEEDBACKPKT)
    {
        //Feedback pkt
        EV << NOW << " UeMac::fromPhy - node " << nodeId_ << " Received feedback pkt" << endl;
        macHandleFeedbackPkt(pkt);
    }
    else if (userInfo->getFrameType()==GRANTPKT)
    {
        //Scheduling Grant
        if (strcmp(pkt->getName(), "VehGrant") == 0)
        {
            EV << NOW << " UeMac::fromPhy - node " << nodeId_ << " Received Vehicular Scheduling Grant pkt" << endl;
            vecHandleVehicularGrant(pkt);
        }
        else
        {
            EV << NOW << " UeMac::fromPhy - node " << nodeId_ << " Received Scheduling Grant pkt" << endl;
            macHandleGrant(pkt);
        }
    }
    else if(userInfo->getFrameType() == DATAPKT)
    {
        // data packet: insert in proper rx buffer
        EV << NOW << " UeMac::fromPhy - node " << nodeId_ << " Received DATA packet" << endl;

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
        }
    }
    else if (userInfo->getFrameType() == RACPKT)
    {
        EV << NOW << " UeMac::fromPhy - node " << nodeId_ << " Received RAC packet" << endl;
        macHandleRac(pkt);
    }
    else
    {
        throw cRuntimeError("Unknown packet type %d", (int)userInfo->getFrameType());
    }
}

void UeMac::vecHandleVehicularGrant(cPacket* pktAux)
{
    // extract grant
    auto pkt = check_and_cast<inet::Packet*> (pktAux);
    auto ipv4Header = pkt->removeAtFront<Ipv4Header>();
    auto udpHeader = pkt->removeAtFront<UdpHeader>();
    auto grant = pkt->peekAtFront<Grant2Veh>();
    AppId appId = grant->getAppId();

    EV << NOW << " UeMac::vecHandleVehicularGrant - UE [" << nodeId_ << "] - Vehicular Grant received from RSU [" 
        << grant->getOffloadGnbId() <<"] for app [" << appId << "]" << endl;

    if (grant->getNewGrant())   // new grant for the application
    {
        grantedApp_.insert(appId);
        // store received grant
        auto userInfo = pkt->getTag<UserControlInfo>();
        double carrierFrequency = userInfo->getCarrierFrequency();
        vecGrant_[appId] = makeShared<Grant2Veh>(*grant);
        grantFrequency_[appId] = carrierFrequency;

        check_and_cast<UePhy*>(phy_)->addGrantedRsu(grant->getOffloadGnbId());
        Direction dir = static_cast<Direction>(userInfo->getDirection());
        EV << "\t New grant received! Byte rate per TTI: " << grant->getBytePerTTI() << ", Direction: " << dir << endl;
    }
    else if (grant->getGrantUpdate())   // check if it is a grant update
    {
        // because the airFrame always arrives before other events start at current TTI
        // the grant will arrive the mac stack before any new data is generated by the app
        auto userInfo = pkt->getTag<UserControlInfo>();
        double carrierFrequency = userInfo->getCarrierFrequency();
        vecGrant_[appId] = makeShared<Grant2Veh>(*grant);
        grantFrequency_[appId] = carrierFrequency;

        Direction dir = static_cast<Direction>(userInfo->getDirection());
        EV << "\t Grant update received! New byte rate per TTI: " << grant->getBytePerTTI() << ", Direction: " << dir << endl;
    }
    else if (grant->getPause())
    {
        // if the grant does not exist in the grantedApp_ set, it means this is the first time the grant is received
        if (grantedApp_.find(appId) == grantedApp_.end())
        {
            EV << "\t First time receives the grant for AppId: " << appId << ". Current CQI is low, pause the grant first" << endl;
            grantedApp_.insert(appId);
            // ensure the PHY stack will keep sending feedback to the RSU
            check_and_cast<UePhy*>(phy_)->addGrantedRsu(grant->getOffloadGnbId());
            
            delete pkt;
            pkt = nullptr;
            return;
        }
        else
        {
            EV << "\t Pause the grant for AppId: " << appId << endl;
        }
    }
    else if (grant->getGrantStop())  // check if the grant is stopped
    {
        EV << "\t Stop the grant for AppId: " << appId << endl;
        // because the airFrame always arrives before other events start at current TTI
        // the grant will arrive the mac stack before any new data is generated by the app
        grantedApp_.erase(appId);
        vecGrant_.erase(appId);     // inet::IntrusivePtr is a smart pointer, so it will delete the object automatically
        grantFrequency_.erase(appId);
        requiredTtiCount_.erase(appId);

        check_and_cast<UePhy*>(phy_)->removeGrantedRsu(grant->getOffloadGnbId());
    }
    else
    {
        EV << NOW << " UeMac::vecHandleVehicularGrant - Vehicular Grant not recognized" << endl;
        delete pkt;
        pkt = nullptr;
        return;
    }


    // send the grant packet to the upper layer
    pkt->insertAtFront(udpHeader);
    pkt->insertAtFront(ipv4Header);

    // add flowcontrolinfo to the packet
    auto flowControlInfo = pkt->addTagIfAbsent<FlowControlInfo>();
    flowControlInfo->setApplication(CBR);
    flowControlInfo->setTraffic(BACKGROUND);
    flowControlInfo->setRlcType(UM);
    flowControlInfo->setHeaderSize(28);  // IPv4 + UDP header size

    sendUpperPackets(pkt);
}

void UeMac::macHandleGrant(cPacket* pktAux)
{
    EV << NOW << " UeMac::macHandleGrant - UE [" << nodeId_ << "] - Grant received " << endl;

    // extract grant
    auto pkt = check_and_cast<inet::Packet*> (pktAux);
    auto grant = pkt->popAtFront<LteSchedulingGrant>();

    auto userInfo = pkt->getTag<UserControlInfo>();
    double carrierFrequency = userInfo->getCarrierFrequency();
    EV << NOW << " UeMac::macHandleGrant - Direction: " << dirToA(grant->getDirection()) << " Carrier: " << carrierFrequency << endl;

    // delete old grant
    if (schedulingGrant_.find(carrierFrequency) != schedulingGrant_.end() && schedulingGrant_[carrierFrequency]!=nullptr)
    {
        schedulingGrant_[carrierFrequency] = nullptr;
    }

    // store received grant
    schedulingGrant_[carrierFrequency] = grant;
    if (grant->getPeriodic())
    {
        periodCounter_[carrierFrequency] = grant->getPeriod();
        expirationCounter_[carrierFrequency] = grant->getExpiration();
    }

    EV << NOW << "Node " << nodeId_ << " received grant of blocks " << grant->getTotalGrantedBlocks()
       << ", bytes " << grant->getGrantedCwBytes(0) <<" Direction: "<<dirToA(grant->getDirection()) << endl;

    // clearing pending RAC requests
    racRequested_=false;
    racD2DMulticastRequested_=false;

    delete pkt;
}


void UeMac::handleSelfMessage()
{
    EV << NOW << " UeMac::handleSelfMessage " << endl;
    EV << "----- UE MAIN LOOP -----" << endl;

    // extract pdus from all harqrxbuffers and pass them to unmaker
    std::map<double, HarqRxBuffers>::iterator mit = harqRxBuffers_.begin();
    std::map<double, HarqRxBuffers>::iterator met = harqRxBuffers_.end();
    for (; mit != met; mit++)
    {
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(mit->first)) > 0)
        {
            EV << "\t numerologyPeriodCounter > 0, not my turn to extract pdus." << endl;
            continue;
        }

        HarqRxBuffers::iterator hit = mit->second.begin();
        HarqRxBuffers::iterator het = mit->second.end();

        std::list<Packet*> pduList;
        for (; hit != het; ++hit)
        {
            pduList = hit->second->extractCorrectPdus();
            while (! pduList.empty())
            {
                auto pdu = pduList.front();
                pduList.pop_front();
                macPduUnmake(pdu);
            }
        }
    }

    EV << NOW << " UeMac::handleSelfMessage " << nodeId_ << " - HARQ process " << (unsigned int)currentHarq_ << endl;

    // no grant available - if user has backlogged data, it will trigger scheduling request
    // no harq counter is updated since no transmission is sent.

    bool noSchedulingGrants = true;
    auto git = schedulingGrant_.begin();
    auto get = schedulingGrant_.end();
    for (; git != get; ++git)
    {
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(git->first)) > 0)
            continue;

        if (git->second != NULL)
            noSchedulingGrants = false;
    }

    if (noSchedulingGrants)
    {
        EV << NOW << " UeMac::handleSelfMessage " << nodeId_ << " NO configured grant" << endl;
        checkRAC();
        // TODO ensure all operations done  before return ( i.e. move H-ARQ rx purge before this point)
    }
    else
    {
        bool periodicGrant = false;
        bool checkRac = false;
        bool skip = false;
        for (git = schedulingGrant_.begin(); git != get; ++git)
        {
            if (git->second != NULL && git->second->getPeriodic())
            {
                periodicGrant = true;
                double carrierFreq = git->first;

                // Periodic checks
                if(--expirationCounter_[carrierFreq] < 0)
                {
                    // Periodic grant is expired
                    git->second = nullptr;
                    checkRac = true;
                }
                else if (--periodCounter_[carrierFreq]>0)
                {
                    skip = true;
                }
                else
                {
                    // resetting grant period
                    periodCounter_[carrierFreq]=git->second->getPeriod();
                    // this is periodic grant TTI - continue with frame sending
                    checkRac = false;
                    skip = false;
                    break;
                }
            }
        }
        if (periodicGrant)
        {
            if (checkRac)
                checkRAC();
            else
            {
                if (skip)
                    return;
            }
        }
    }

    scheduleList_.clear();
    requestedSdus_ = 0;
    if (!noSchedulingGrants) // if a grant is configured
    {
        EV << NOW << " UeMac::handleSelfMessage " << nodeId_ << " entered scheduling" << endl;

        bool retx = false;

        if(!firstTx)
        {
            EV << "\t currentHarq_ counter initialized " << endl;
            firstTx=true;
            // the gNb will receive the first pdu in 2 TTI, thus initializing acid to 0
            currentHarq_ = UE_TX_HARQ_PROCESSES - 2;
        }

        // --------------------------- RETRANSMISSION START --------------------------------
        HarqTxBuffers::iterator it2;
        LteHarqBufferTx * currHarq;
        std::map<double, HarqTxBuffers>::iterator mtit;
        for (mtit = harqTxBuffers_.begin(); mtit != harqTxBuffers_.end(); ++mtit)
        {
            double carrierFrequency = mtit->first;

            // skip if this is not the turn of this carrier
            if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFrequency)) > 0)
                continue;

            // skip if no grant is configured for this carrier
            if (schedulingGrant_.find(carrierFrequency) == schedulingGrant_.end() || schedulingGrant_[carrierFrequency] == NULL)
                continue;

            for(it2 = mtit->second.begin(); it2 != mtit->second.end(); it2++)
            {
                currHarq = it2->second;
                unsigned int numProcesses = currHarq->getNumProcesses();

                for (unsigned int proc = 0; proc < numProcesses; proc++)
                {
                    LteHarqProcessTx* currProc = currHarq->getProcess(proc);

                    // check if the current process has unit ready for retx
                    bool ready = currProc->hasReadyUnits();
                    CwList cwListRetx = currProc->readyUnitsIds();

                    EV << "\t [process=" << proc << "] , [retx=" << ((ready)?"true":"false") << "] , [n=" << cwListRetx.size() << "]" << endl;

                    // check if one 'ready' unit has the same direction of the grant
                    bool checkDir = false;
                    CwList::iterator cit = cwListRetx.begin();
                    for (; cit != cwListRetx.end(); ++cit)
                    {
                        Codeword cw = *cit;
                        auto info = currProc->getPdu(cw)->getTag<UserControlInfo>();
                        if (info->getDirection() == schedulingGrant_[carrierFrequency]->getDirection())
                        {
                            checkDir = true;
                            break;
                        }
                    }

                    // if a retransmission is needed
                    if(ready && checkDir)
                    {
                        UnitList signal;
                        signal.first = proc;
                        signal.second = cwListRetx;
                        currHarq->markSelected(signal,schedulingGrant_[carrierFrequency]->getUserTxParams()->getLayers().size());
                        retx = true;
                        break;
                    }
                }
            }
        }
        // --------------------------- RETRANSMISSION END --------------------------------
        
        // if no retx is needed, proceed with normal scheduling
        if(!retx)
        {
            emptyScheduleList_ = true;
            std::map<double, LteSchedulerUeUl*>::iterator sit;
            for (sit = lcgScheduler_.begin(); sit != lcgScheduler_.end(); ++sit)
            {
                double carrierFrequency = sit->first;

                // skip if this is not the turn of this carrier
                if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFrequency)) > 0)
                    continue;

                LteSchedulerUeUl* carrierLcgScheduler = sit->second;

                EV << "UeMac::handleSelfMessage - running LCG scheduler for carrier [" << carrierFrequency << "]" << endl;
                LteMacScheduleList* carrierScheduleList = carrierLcgScheduler->schedule();
                EV << "UeMac::handleSelfMessage - scheduled " << carrierScheduleList->size() << " connections on carrier " << carrierFrequency << endl;
                scheduleList_[carrierFrequency] = carrierScheduleList;
                if (!carrierScheduleList->empty())
                    emptyScheduleList_ = false;
            }

            if ((bsrTriggered_ || bsrD2DMulticastTriggered_) && emptyScheduleList_)
            {
                // no connection scheduled, but we can use this grant to send a BSR to the eNB
                macPduMake();
            }
            else
            {
                requestedSdus_ = macSduRequest(); // returns an integer
            }

        }

        // Message that triggers flushing of Tx H-ARQ buffers for all users
        // This way, flushing is performed after the (possible) reception of new MAC PDUs
        cMessage* flushHarqMsg = new cMessage("flushHarqMsg");
        flushHarqMsg->setSchedulingPriority(1);        // after other messages
        scheduleAt(NOW, flushHarqMsg);
    }

    //============================ DEBUG ==========================
    /***
     * debugHarq_ is false by default, set in the constructor LteMacUe::LteMacUe()
     */
    if (debugHarq_)
    {
        std::map<double, HarqTxBuffers>::iterator mtit;
        for (mtit = harqTxBuffers_.begin(); mtit != harqTxBuffers_.end(); ++mtit)
        {
            EV << "\n carrier[ " << mtit->first << "] htxbuf.size " << mtit->second.size() << endl;

            HarqTxBuffers::iterator it;

            EV << "\n htxbuf.size " << harqTxBuffers_.size() << endl;

            int cntOuter = 0;
            int cntInner = 0;
            for(it = mtit->second.begin(); it != mtit->second.end(); it++)
            {
                LteHarqBufferTx* currHarq = it->second;
                BufferStatus harqStatus = currHarq->getBufferStatus();
                BufferStatus::iterator jt = harqStatus.begin(), jet= harqStatus.end();

                EV << "\t cicloOuter " << cntOuter << " - bufferStatus.size=" << harqStatus.size() << endl;
                for(; jt != jet; ++jt)
                {
                    EV << "\t\t cicloInner " << cntInner << " - jt->size=" << jt->size()
                       << " - statusCw(0/1)=" << jt->at(0).second << "/" << jt->at(1).second << endl;
                }
            }
        }
    }
    //======================== END DEBUG ==========================

    // update current harq process id, if needed
    if (requestedSdus_ == 0)
    {
        EV << NOW << " UeMac::handleSelfMessage - incrementing counter for HARQ processes " << (unsigned int)currentHarq_ << " --> " << (currentHarq_+1)%harqProcesses_ << endl;
        currentHarq_ = (currentHarq_+1) % harqProcesses_;
    }

    decreaseNumerologyPeriodCounter();

    EV << "--- END UE MAIN LOOP ---" << endl;
}

void UeMac::vecHandleSelfMessage()
{
    if (grantedApp_.empty())
    {
        EV << NOW << " UeMac::vecHandleSelfMessage - UE [" << nodeId_ << "] - no granted applications" << endl;
        return;
    }

    // if the UE is in the resource allocation mode, it checks vecGrant_
    requestedSdus_ = vecRequestBufferedData();

    if (requestedSdus_ > 0)
    {
        // Message that triggers flushing of Tx H-ARQ buffers for all users
        // This way, flushing is performed after the (possible) reception of new MAC PDUs
        cMessage* flushAppMsg = new cMessage("flushAppMsg");
        flushAppMsg->setSchedulingPriority(1);        // after other messages
        scheduleAt(NOW, flushAppMsg);

        EV << NOW << " UeMac::vecHandleSelfMessage - UE [" << nodeId_ << "] - requested " << requestedSdus_ << " SDUs" << endl;
    }
    else
    {
        EV << NOW << " UeMac::vecHandleSelfMessage - UE [" << nodeId_ << "] - no SDUs requested" << endl;
    }
}

int UeMac::vecRequestBufferedData()
{
    EV << NOW << " UeMac::vecRequestBufferedData - UE [" << nodeId_ << "] - check buffered data for granted applications " << endl;

    requestedSdus_ = 0;

    // check granted apps one by one, here appId = cid
    for(auto appId : grantedApp_)
    {
        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(grantFrequency_[appId])) > 0)
            continue;
        
        // if no buffer is available, the data has not arrived the RLC yet
        if (macBuffers_.find(appId) == macBuffers_.end())
            continue;
        
        // if the buffer is empty, no need to request a MAC pdu
        LteMacBuffer* vQueue = macBuffers_[appId];
        if (vQueue->isEmpty())
            continue;

        inet::IntrusivePtr<const Grant2Veh>& grant = vecGrant_[appId];
        MacNodeId destId = MacCidToNodeId(appId); // macNodeId of the nrMac

        // get the number of bytes to serve and the number of bytes available in the grant per TTI
        int toServe = vQueue->getQueueOccupancy();
        toServe += MAC_HEADER;
        if (connDesc_[appId].getRlcType() == UM)
            toServe += RLC_HEADER_UM;
        else if (connDesc_[appId].getRlcType() == AM)
            toServe += RLC_HEADER_AM;

        double dataRate = grant->getBytePerTTI(); // bytes per TTI
        requiredTtiCount_[appId] = ceil(toServe / dataRate);
        // determine the number of bytes to serve
        int sentData = toServe - MAC_HEADER;    // bytes actually allocated, 20 bytes for testing
        // remove SDU from virtual buffer. there should be only one SDU in the buffer, since we offload the whole data packet
        while (!vQueue->isEmpty())
        {
            // remove SDUs from virtual buffer
            vQueue->popFront();
        }

        // vQueue->popFront();
        // if (!vQueue->isEmpty())
        // {
        //     std::cout << "UeMac::vecRequestBufferedData - Remaining Queue size: " << vQueue->getQueueOccupancy() << std::endl;
        //     throw cRuntimeError("UeMac::vecRequestBufferedData - the virtual buffer should be empty after serving the SDU");
        // }
            
        
        // if (toServe <= availableBytes)
        // {
        //     // remove SDU from virtual buffer
        //     vQueue->popFront();

        //     sentData = toServe - MAC_HEADER;    // bytes actually allocated

        //     while (!vQueue->isEmpty())
        //     {
        //         // remove SDUs from virtual buffer
        //         vQueue->popFront();
        //     }
        // }
        // else
        // {
        //     int alloc = availableBytes - MAC_HEADER;    // bytes actually allocated
        //     sentData = alloc;

        //     if (connDesc_[appId].getRlcType() == UM)
        //         alloc -= RLC_HEADER_UM;
        //     else if (connDesc_[appId].getRlcType() == AM)
        //         alloc -= RLC_HEADER_AM;
        //     // update buffer
        //     while (alloc > 0)
        //     {
        //         // update pkt info
        //         PacketInfo newPktInfo = vQueue->popFront();
        //         if (newPktInfo.first > alloc)
        //         {
        //             newPktInfo.first = newPktInfo.first - alloc;
        //             vQueue->pushFront(newPktInfo);
        //             alloc = 0;
        //         }
        //         else
        //         {
        //             alloc -= newPktInfo.first;
        //         }
        //     }
        // }

        EV << NOW << " UeMac::vecOffloadPendingData - AppId [" << appId << "] has pending data, request " 
            << sentData << " bytes data from RLC. Remaining queue size: " << vQueue->getQueueOccupancy() << endl;

        // send the request message to the upper layer
        // TODO: Replace by tag
        auto pkt = new Packet("LteMacSduRequest");
        auto macSduRequest = makeShared<LteMacSduRequest>();
        macSduRequest->setChunkLength(b(1)); // TODO: should be 0
        macSduRequest->setUeId(destId);
        macSduRequest->setLcid(MacCidToLcid(appId));
        macSduRequest->setSduSize(sentData);
        pkt->insertAtFront(macSduRequest);
        *(pkt->addTag<FlowControlInfo>()) = connDesc_[appId];
        sendUpperPackets(pkt);

        requestedSdus_++;
    }

    return requestedSdus_;
}


int UeMac::macSduRequest()
{
    EV << "----- START UeMac::macSduRequest -----\n";
    int numRequestedSdus = 0;

    // get the number of granted bytes for each codeword
    std::vector<unsigned int> allocatedBytes;

    auto git = schedulingGrant_.begin();
    for (; git != schedulingGrant_.end(); ++git)
    {
        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(git->first)) > 0)
            continue;

        if (git->second == NULL)
            continue;

        for (int cw=0; cw<git->second->getGrantedCwBytesArraySize(); cw++)
            allocatedBytes.push_back(git->second->getGrantedCwBytes(cw));
    }

    // Ask for a MAC sdu for each scheduled user on each codeword
    std::map<double, LteMacScheduleList*>::iterator cit = scheduleList_.begin();
    for (; cit != scheduleList_.end(); ++cit)
    {
        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(cit->first)) > 0)
            continue;

        LteMacScheduleList::const_iterator it;
        for (it = cit->second->begin(); it != cit->second->end(); it++)
        {
            MacCid destCid = it->first.first;
            Codeword cw = it->first.second;
            MacNodeId destId = MacCidToNodeId(destCid); // macNodeId of the nrMac

            std::pair<MacCid,Codeword> key(destCid, cw);
            LteMacScheduleList* scheduledBytesList = lcgScheduler_[cit->first]->getScheduledBytesList();
            LteMacScheduleList::const_iterator bit = scheduledBytesList->find(key);

            // consume bytes on this codeword
            if (bit == scheduledBytesList->end())
                throw cRuntimeError("UeMac::macSduRequest - cannot find entry in scheduledBytesList");
            else
            {
                allocatedBytes[cw] -= bit->second;

                EV << NOW <<" UeMac::macSduRequest - cid[" << destCid << "] - sdu size[" << bit->second << "B] - " << allocatedBytes[cw] << " bytes left on codeword " << cw << endl;

                // send the request message to the upper layer
                // TODO: Replace by tag
                auto pkt = new Packet("LteMacSduRequest");
                auto macSduRequest = makeShared<LteMacSduRequest>();
                macSduRequest->setChunkLength(b(1)); // TODO: should be 0
                macSduRequest->setUeId(destId);
                macSduRequest->setLcid(MacCidToLcid(destCid));
                macSduRequest->setSduSize(bit->second);
                pkt->insertAtFront(macSduRequest);
                *(pkt->addTag<FlowControlInfo>()) = connDesc_[destCid];
                sendUpperPackets(pkt);

                numRequestedSdus++;
            }
        }
    }

    EV << "------ END UeMac::macSduRequest ------\n";
    return numRequestedSdus;
}

void UeMac::macPduUnmake(cPacket* pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    auto macPkt = pkt->removeAtFront<LteMacPdu>();
    while (macPkt->hasSdu())
    {
        // Extract and send SDU
        auto upPkt = macPkt->popSdu();
        take(upPkt);

        EV << "UeMac::macPduUnmake - extracted SDU" << endl;

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

    pkt->insertAtFront(macPkt);

    ASSERT(pkt->getOwner() == this);
    delete pkt;
}

void UeMac::handleUpperMessage(cPacket* pktAux)
{
    EV << "UeMac::handleUpperMessage - handle message from RLC stack" << endl;

    auto pkt = check_and_cast<Packet *>(pktAux);

    if(!strcmp(pkt->getName(), "SrvReq"))
    {
        EV << "UeMac::handleUpperMessage - vehicle service request received, send to PHY stack" << endl;

        double carrierFrequency = phy_->getPrimaryChannelModel()->getCarrierFrequency();
        pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFrequency);
        pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDestId(getMacCellId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDirection(UL);
        pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(RACPKT);

        sendLowerPackets(pkt);
        return;
    }

    bool isNotifyNewData = checkIfHeaderType<LteRlcPduNewData>(pkt);    // check if this is a notification for new arriving data
    // bufferize packet
    bufferizePacket(pkt);

    if(!isNotifyNewData){
        requestedSdus_--;
        ASSERT(requestedSdus_ >= 0);

        auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();
        requestedApps_.insert(ctrlInfoToMacCid(lteInfo));

        // build a MAC PDU only after all MAC SDUs have been received from RLC
        if (requestedSdus_ == 0)
        {
            vecMacPduMake();
            
            // if (resAllocateMode_)
            // {
            //     vecMacPduMake();
            // }
            // else
            // {
            //     // make PDU and BSR (if necessary)
            //     macPduMake();

            //     // update current harq process id
            //     EV << NOW << " LteMacUe::handleMessage - incrementing counter for HARQ processes " << (unsigned int)currentHarq_ << " --> " << (currentHarq_+1)%harqProcesses_ << endl;
            //     currentHarq_ = (currentHarq_+1) % harqProcesses_;
            // }
        }
    }
    else    // is notify new data, start the tti tick
    {
        if (ttiTick_ != nullptr && !ttiTick_->isScheduled())
        {
            int intTime = ceil(NOW.dbl() / ttiPeriod_);
            scheduleAt(intTime * ttiPeriod_, ttiTick_);
        }
    }
}


bool UeMac::bufferizePacket(cPacket* pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);

    if (pkt->getBitLength() <= 1) { // no data in this packet - should not be buffered
        delete pkt;
        pkt = nullptr;
        return false;
    }

    pkt->setTimestamp();           // add time-stamp with current time to packet

    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    // obtain the cid from the packet informations
    MacCid cid = ctrlInfoToMacCid(lteInfo);

    // this packet is used to signal the arrival of new data in the RLC buffers
    if (checkIfHeaderType<LteRlcPduNewData>(pkt))
    {
        EV << "UeMac::bufferizePacket - notifying message for the arrival of new data in the RLC buffers" << endl;

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

            EV << "UeMac::bufferizePacket - Using new buffer on node: " <<
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

                EV << "UeMac::bufferizePacket - Using old buffer on node: " <<
                MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
                vqueue->getQueueOccupancy() << "\n";
            }
            else
                throw cRuntimeError("LteMacUe::bufferizePacket - cannot find mac buffer for cid %d", cid);
        }

        delete pkt;
        pkt = nullptr;
        return true;    // notify the activation of the connection
    }

    // this is a MAC SDU, bufferize it in the MAC buffer
    EV << "UeMac::bufferizePacket - MAC SDU from RLC stack, bufferize it in the MAC buffer" << endl;

    LteMacBuffers::iterator it = mbuf_.find(cid);
    if (it == mbuf_.end())
    {
        // Queue not found for this cid: create
        LteMacQueue* queue = new LteMacQueue(queueSize_);

        queue->pushBack(pkt);

        mbuf_[cid] = queue;

        EV << "UeMac::bufferizePacket - Using new buffer on node: " <<
        MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
        queue->getQueueSize() - queue->getByteLength() << "\n";
    }
    else
    {
        // Found
        LteMacQueue* queue = it->second;
        if (!queue->pushBack(pkt))
        {
            totalOverflowedBytes_ += pkt->getByteLength();
            double sample = (double)totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());
            if (lteInfo->getDirection()==DL)
            {
                emit(macBufferOverflowDl_,sample);
            }
            else
            {
                emit(macBufferOverflowUl_,sample);
            }

            EV << "UeMac::bufferizePacket - Dropped packet: queue" << cid << " is full\n";

            // @author Alessandro Noferi
            // discard the RLC
            if(packetFlowManager_ != nullptr)
            {
                unsigned int rlcSno = check_and_cast<LteRlcUmDataPdu *>(pkt)->getPduSequenceNumber();
                packetFlowManager_->discardRlcPdu(lteInfo->getLcid(),rlcSno);
            }

            delete pkt;
            pkt = nullptr;
            return false;
        }

        EV << "UeMac::bufferizePacket - Using old buffer on node: " <<
        MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << "(cid: " << cid << "), Space left in the Queue: " <<
        queue->getQueueSize() - queue->getByteLength() << "\n";
    }

    return true;
}

void UeMac::sendUpperPackets(cPacket* pkt)
{
    EV << NOW << " UeMac::sendUpperPackets - Sending packet " << pkt->getName() << " on port MAC_to_RLC\n";
    // Send message
    send(pkt,up_[OUT_GATE]);
    nrToUpper_++;
    emit(sentPacketToUpperLayer, pkt);
}

void UeMac::sendLowerPackets(cPacket* pkt)
{
    EV << NOW << "UeMac::sendLowerPackets, Sending packet " << pkt->getName() << " on port MAC_to_PHY\n";
    // Send message
    if (!resAllocateMode_)
        updateUserTxParam(pkt);
    
    send(pkt,down_[OUT_GATE]);
    nrToLower_++;
    emit(sentPacketToLowerLayer, pkt);
}

void UeMac::vecMacPduMake(MacCid cid)
{
    for (AppId appId : requestedApps_)
    {
        double carrierFreq = grantFrequency_[appId];
        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
            continue;

        auto grant = vecGrant_[appId];

        // create a MAC PDU
        Packet* macPkt = new Packet("LteMacPdu");
        auto header = makeShared<LteMacPdu>();
        header->setHeaderLength(MAC_HEADER);
        macPkt->insertAtFront(header);

        macPkt->addTagIfAbsent<CreationTimeTag>()->setCreationTime(NOW);
        macPkt->addTagIfAbsent<UserControlInfo>()->setSourceId(nodeId_);
        macPkt->addTagIfAbsent<UserControlInfo>()->setDestId(grant->getOffloadGnbId());
        macPkt->addTagIfAbsent<UserControlInfo>()->setDirection(UL);    // ue only uploads when making pdu
        macPkt->addTagIfAbsent<UserControlInfo>()->setLcid(MacCidToLcid(appId));
        macPkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFreq);
        macPkt->addTagIfAbsent<UserControlInfo>()->setGrantId(grant->getChunkId());
        macPkt->addTagIfAbsent<UserControlInfo>()->setTxNumber(1);
        macPkt->addTagIfAbsent<UserControlInfo>()->setNdi(true);

        // add resource block mapping
        RbMap rbMap = std::map<Remote, std::map<Band, unsigned int>>();
        rbMap[MACRO] = std::map<Band, unsigned int>(grant->getGrantedBlocks());
        macPkt->addTagIfAbsent<UserControlInfo>()->setGrantedBlocks(rbMap);
        macPkt->addTagIfAbsent<UserControlInfo>()->setTotalGrantedBlocks(grant->getGrantedBlocks().size());

        macPkt->addTagIfAbsent<VecDataInfo>()->setDuration(requiredTtiCount_[appId] * ttiPeriod_);

        /**
         * note that when add the pkt into the header, the FlowControlInfo is also stored together with the pkt
         * when the gNB call the function macPduUnmake, the FlowControlInfo becomes visible for the pkt
         */
        auto pkt = check_and_cast<Packet *>(mbuf_[appId]->popFront());
        header = macPkt->removeAtFront<LteMacPdu>();
        header->pushSdu(pkt);
        macPkt->insertAtFront(header);

        appPduList_[appId] = macPkt;

        insertMacPdu(macPkt);
    }

    requestedApps_.clear();
}

void UeMac::macPduMake(MacCid cid)
{
    EV << NOW << " UeMac::macPduMake - Building PDU for cid " << cid << endl;
    int64_t size = 0;

    macPduList_.clear();

    bool bsrAlreadyMade = false;
    // UE is in D2D-mode but it received an UL grant (for BSR)
    auto git = schedulingGrant_.begin();
    for (; git != schedulingGrant_.end(); ++git)
    {
        double carrierFreq = git->first;

        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
            continue;

        if (git->second != nullptr && git->second->getDirection() == UL && emptyScheduleList_)
        {
            if (bsrTriggered_ || bsrD2DMulticastTriggered_)
            {
                // Compute BSR size taking into account only DM flows
                int sizeBsr = 0;
                LteMacBufferMap::const_iterator itbsr;
                for (itbsr = macBuffers_.begin(); itbsr != macBuffers_.end(); itbsr++)
                {
                    MacCid cid = itbsr->first;
                    Direction connDir = (Direction)connDesc_[cid].getDirection();

                    // if the bsr was triggered by D2D (D2D_MULTI), only account for D2D (D2D_MULTI) connections
                    if (bsrTriggered_ && connDir != D2D)
                        continue;
                    if (bsrD2DMulticastTriggered_ && connDir != D2D_MULTI)
                        continue;

                    sizeBsr += itbsr->second->getQueueOccupancy();

                    // take into account the RLC header size
                    if (sizeBsr > 0)
                    {
                        if (connDesc_[cid].getRlcType() == UM)
                            sizeBsr += RLC_HEADER_UM;
                        else if (connDesc_[cid].getRlcType() == AM)
                            sizeBsr += RLC_HEADER_AM;
                    }
                }

                if (sizeBsr > 0)
                {
                    // Call the appropriate function for make a BSR for a D2D communication
                    Packet* macPktBsr = makeBsr(sizeBsr);
                    auto info = macPktBsr->getTagForUpdate<UserControlInfo>();
                    if (info != NULL)
                    {
                        info->setCarrierFrequency(carrierFreq);
                        info->setUserTxParams(git->second->getUserTxParams()->dup());
                        if (bsrD2DMulticastTriggered_)
                        {
                            info->setLcid(D2D_MULTI_SHORT_BSR);
                            bsrD2DMulticastTriggered_ = false;
                        }
                        else
                            info->setLcid(D2D_SHORT_BSR);
                    }

                    // Add the created BSR to the PDU List
                    if( macPktBsr != NULL )
                    {
                        LteChannelModel* channelModel = phy_->getChannelModel();
                        if (channelModel == NULL)
                            throw cRuntimeError("UeMac::macPduMake - channel model is a null pointer. Abort.");
                        else
                            macPduList_[channelModel->getCarrierFrequency()][ std::pair<MacNodeId, Codeword>( getMacCellId(), 0) ] = macPktBsr;
                        bsrAlreadyMade = true;
                        EV << "UeMac::macPduMake - BSR D2D created with size " << sizeBsr << "created" << endl;
                    }

                    bsrRtxTimer_ = bsrRtxTimerStart_;  // this prevent the UE to send an unnecessary RAC request
                }
                else
                {
                    bsrD2DMulticastTriggered_ = false;
                    bsrTriggered_ = false;
                    bsrRtxTimer_ = 0;
                }
            }
            break;
        }
    }

    if(!bsrAlreadyMade)
    {
        // In a D2D communication if BSR was created above this part isn't executed
        // Build a MAC PDU for each scheduled user on each codeword
        std::map<double, LteMacScheduleList*>::iterator cit = scheduleList_.begin();
        for (; cit != scheduleList_.end(); ++cit)
        {
            double carrierFreq = cit->first;

            // skip if this is not the turn of this carrier
            if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
                continue;

            LteMacScheduleList::const_iterator it;
            for (it = cit->second->begin(); it != cit->second->end(); it++)
            {
                Packet* macPkt;

                MacCid destCid = it->first.first;
                Codeword cw = it->first.second;

                // get the direction (UL/D2D/D2D_MULTI) and the corresponding destination ID
                FlowControlInfo* lteInfo = &(connDesc_.at(destCid));
                MacNodeId destId = lteInfo->getDestId();
                Direction dir = (Direction)lteInfo->getDirection();

                std::pair<MacNodeId, Codeword> pktId = std::pair<MacNodeId, Codeword>(destId, cw);
                unsigned int sduPerCid = it->second;

                if (sduPerCid == 0 && !bsrTriggered_ && !bsrD2DMulticastTriggered_)
                    continue;

                if (macPduList_.find(carrierFreq) == macPduList_.end())
                {
                    MacPduList newList;
                    macPduList_[carrierFreq] = newList;
                }
                MacPduList::iterator pit = macPduList_[carrierFreq].find(pktId);

                // No packets for this user on this codeword
                if (pit == macPduList_[carrierFreq].end())
                {
                    // Create a PDU
                    macPkt = new Packet("LteMacPdu");
                    auto header = makeShared<LteMacPdu>();
                    header->setHeaderLength(MAC_HEADER);
                    macPkt->insertAtFront(header);

                    macPkt->addTagIfAbsent<CreationTimeTag>()->setCreationTime(NOW);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
                    macPkt->addTagIfAbsent<UserControlInfo>()->setDestId(destId);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setDirection(dir);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setLcid(MacCidToLcid(SHORT_BSR));
                    macPkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFreq);

                    macPkt->addTagIfAbsent<UserControlInfo>()->setGrantId(schedulingGrant_[carrierFreq]->getGrandId());

                    if (usePreconfiguredTxParams_)
                        macPkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(preconfiguredTxParams_->dup());
                    else
                        macPkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(schedulingGrant_[carrierFreq]->getUserTxParams()->dup());

                    macPduList_[carrierFreq][pktId] = macPkt;
                }
                else
                {
                    // Never goes here because of the macPduList_.clear() at the beginning
                    macPkt = pit->second;
                }

                while (sduPerCid > 0)
                {
                    // Add SDU to PDU
                    // Find Mac Pkt
                    if (mbuf_.find(destCid) == mbuf_.end())
                        throw cRuntimeError("Unable to find mac buffer for cid %d", destCid);

                    if (mbuf_[destCid]->isEmpty())
                        throw cRuntimeError("Empty buffer for cid %d, while expected SDUs were %d", destCid, sduPerCid);

                    auto pkt = check_and_cast<Packet *>(mbuf_[destCid]->popFront());
                    if (pkt != NULL)
                    {
                        // multicast support
                        // this trick gets the group ID from the MAC SDU and sets it in the MAC PDU
                        auto infoVec = getTagsWithInherit<LteControlInfo>(pkt);
                        if (infoVec.empty())
                            throw cRuntimeError("No tag of type LteControlInfo found");

                        int32_t groupId =  infoVec.front().getMulticastGroupId();
                        if (groupId >= 0) // for unicast, group id is -1
                            macPkt->getTagForUpdate<UserControlInfo>()->setMulticastGroupId(groupId);

                        drop(pkt);

                        auto header = macPkt->removeAtFront<LteMacPdu>();
                        header->pushSdu(pkt);
                        macPkt->insertAtFront(header);
                        sduPerCid--;
                    }
                    else
                        throw cRuntimeError("UeMac::macPduMake - extracted SDU is NULL. Abort.");
                }

                // consider virtual buffers to compute BSR size
                size += macBuffers_[destCid]->getQueueOccupancy();

                if (size > 0)
                {
                    // take into account the RLC header size
                    if (connDesc_[destCid].getRlcType() == UM)
                        size += RLC_HEADER_UM;
                    else if (connDesc_[destCid].getRlcType() == AM)
                        size += RLC_HEADER_AM;
                }
            }
        }
    }

    // Put MAC PDUs in H-ARQ buffers
    std::map<double, MacPduList>::iterator lit;
    for (lit = macPduList_.begin(); lit != macPduList_.end(); ++lit)
    {
        double carrierFreq = lit->first;
        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
            continue;

        if (harqTxBuffers_.find(carrierFreq) == harqTxBuffers_.end())
        {
            HarqTxBuffers newHarqTxBuffers;
            harqTxBuffers_[carrierFreq] = newHarqTxBuffers;
        }
        HarqTxBuffers& harqTxBuffers = harqTxBuffers_[carrierFreq];

        MacPduList::iterator pit;
        for (pit = lit->second.begin(); pit != lit->second.end(); pit++)
        {
            MacNodeId destId = pit->first.first;
            Codeword cw = pit->first.second;
            // Check if the HarqTx buffer already exists for the destId
            // Get a reference for the destId TXBuffer
            LteHarqBufferTx* txBuf;
            HarqTxBuffers::iterator hit = harqTxBuffers.find(destId);
            if ( hit != harqTxBuffers.end() )
            {
                // The tx buffer already exists
                txBuf = hit->second;
            }
            else
            {
                // The tx buffer does not exist yet for this mac node id, create one
                LteHarqBufferTx* hb;
                // FIXME: hb is never deleted
                auto info = pit->second->getTag<UserControlInfo>();
                if (info->getDirection() == UL)
                    hb = new LteHarqBufferTx((unsigned int) ENB_TX_HARQ_PROCESSES, this, (LteMacBase*) getMacByMacNodeId(destId));
                else // D2D or D2D_MULTI
                    hb = new LteHarqBufferTxD2D((unsigned int) ENB_TX_HARQ_PROCESSES, this, (LteMacBase*) getMacByMacNodeId(destId));
                harqTxBuffers[destId] = hb;
                txBuf = hb;
            }

            // search for an empty unit within the first available process
            UnitList txList = (pit->second->getTag<UserControlInfo>()->getDirection() == D2D_MULTI) ? txBuf->getEmptyUnits(currentHarq_) : txBuf->firstAvailable();
            EV << "UeMac::macPduMake - [Used Acid=" << (unsigned int)txList.first << "]" << endl;

            //Get a reference of the LteMacPdu from pit pointer (extract Pdu from the MAP)
            auto macPkt = pit->second;

            /* BSR related operations

            // according to the TS 36.321 v8.7.0, when there are uplink resources assigned to the UE, a BSR
            // has to be send even if there is no data in the user's queues. In few words, a BSR is always
            // triggered and has to be send when there are enough resources

            // TODO implement differentiated BSR attach
            //
            //            // if there's enough space for a LONG BSR, send it
            //            if( (availableBytes >= LONG_BSR_SIZE) ) {
            //                // Create a PDU if data were not scheduled
            //                if (pdu==0)
            //                    pdu = new LteMacPdu();
            //
            //                if(LteDebug::trace("LteSchedulerUeUl::schedule") || LteDebug::trace("LteSchedulerUeUl::schedule@bsrTracing"))
            //                    fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - Node %d, sending a Long BSR...\n",NOW,nodeId);
            //
            //                // create a full BSR
            //                pdu->ctrlPush(fullBufferStatusReport());
            //
            //                // do not reset BSR flag
            //                mac_->bsrTriggered() = true;
            //
            //                availableBytes -= LONG_BSR_SIZE;
            //
            //            }
            //
            //            // if there's space only for a SHORT BSR and there are scheduled flows, send it
            //            else if( (mac_->bsrTriggered() == true) && (availableBytes >= SHORT_BSR_SIZE) && (highestBackloggedFlow != -1) ) {
            //
            //                // Create a PDU if data were not scheduled
            //                if (pdu==0)
            //                    pdu = new LteMacPdu();
            //
            //                if(LteDebug::trace("LteSchedulerUeUl::schedule") || LteDebug::trace("LteSchedulerUeUl::schedule@bsrTracing"))
            //                    fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - Node %d, sending a Short/Truncated BSR...\n",NOW,nodeId);
            //
            //                // create a short BSR
            //                pdu->ctrlPush(shortBufferStatusReport(highestBackloggedFlow));
            //
            //                // do not reset BSR flag
            //                mac_->bsrTriggered() = true;
            //
            //                availableBytes -= SHORT_BSR_SIZE;
            //
            //            }
            //            // if there's a BSR triggered but there's not enough space, collect the appropriate statistic
            //            else if(availableBytes < SHORT_BSR_SIZE && availableBytes < LONG_BSR_SIZE) {
            //                Stat::put(LTE_BSR_SUPPRESSED_NODE,nodeId,1.0);
            //                Stat::put(LTE_BSR_SUPPRESSED_CELL,mac_->cellId(),1.0);
            //            }
            //            Stat::put (LTE_GRANT_WASTED_BYTES_UL, nodeId, availableBytes);
            //        }
            //
            //        // 4) PDU creation
            //
            //        if (pdu!=0) {
            //
            //            pdu->cellId() = mac_->cellId();
            //            pdu->nodeId() = nodeId;
            //            pdu->direction() = mac::UL;
            //            pdu->error() = false;
            //
            //            if(LteDebug::trace("LteSchedulerUeUl::schedule"))
            //                fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - Node %d, creating uplink PDU.\n", NOW, nodeId);
            //
            //        } */

            auto header = macPkt->removeAtFront<LteMacPdu>();
            // Attach BSR to PDU if RAC is won and wasn't already made
            if ((bsrTriggered_ || bsrD2DMulticastTriggered_) && !bsrAlreadyMade && size > 0)
            {
                MacBsr* bsr = new MacBsr();
                bsr->setTimestamp(simTime().dbl());
                bsr->setSize(size);
                header->pushCe(bsr);
                bsrTriggered_ = false;
                bsrD2DMulticastTriggered_ = false;
                bsrAlreadyMade = true;
                EV << "UeMac::macPduMake - BSR created with size " << size << endl;
            }

            if (bsrAlreadyMade && size > 0) // this prevent the UE to send an unnecessary RAC request
            {
                /***
                 * bsrRtxTimerStart_ = 40; set in constructor LteMacUe::LteMacUe()
                 * see standard 38.331, RetxBSR-Timer
                 */
                bsrRtxTimer_ = bsrRtxTimerStart_;
            }
            else
                bsrRtxTimer_ = 0;

            macPkt->insertAtFront(header);

            EV << "UeMac: pduMaker created PDU: " << macPkt->str() << endl;

            // TODO: harq test
            // pdu transmission here (if any)
            // txAcid has HARQ_NONE for non-fillable codeword, acid otherwise
            if (txList.second.empty())
            {
                EV << "UeMac() : no available process for this MAC pdu in TxHarqBuffer" << endl;
                delete macPkt;
                macPkt = nullptr;
            }
            else
            {
                //Insert PDU in the Harq Tx Buffer
                //txList.first is the acid
                txBuf->insertPdu(txList.first,cw, macPkt);
            }
        }
    }
}

void UeMac::checkRAC()
{
    EV << NOW << " UeMac::checkRAC - Ue  " << nodeId_ << ", racTimer : " << racBackoffTimer_ << " maxRacTryOuts : " << maxRacTryouts_
       << ", raRespTimer:" << raRespTimer_ << endl;

    if (racBackoffTimer_>0)
    {
        racBackoffTimer_--;
        return;
    }

    if(raRespTimer_>0)
    {
        // decrease RAC response timer
        raRespTimer_--;
        EV << NOW << " UeMac::checkRAC - waiting for previous RAC requests to complete (timer=" << raRespTimer_ << ")" << endl;
        return;
    }

    if (bsrRtxTimer_>0)
    {
        // decrease BSR timer
        bsrRtxTimer_--;
        EV << NOW << " UeMac::checkRAC - waiting for a grant, BSR rtx timer has not expired yet (timer=" << bsrRtxTimer_ << ")" << endl;

        return;
    }

    // Avoids double requests whithin same TTI window
    if (racRequested_)
    {
        EV << NOW << " UeMac::checkRAC - double RAC request" << endl;
        racRequested_=false;
        return;
    }
    if (racD2DMulticastRequested_)
    {
        EV << NOW << " UeMac::checkRAC - double RAC request" << endl;
        racD2DMulticastRequested_=false;
        return;
    }

    bool trigger=false;
    bool triggerD2DMulticast=false;

    LteMacBufferMap::const_iterator it;

    for (it = macBuffers_.begin(); it!=macBuffers_.end();++it)
    {
        if (!(it->second->isEmpty()))
        {
            MacCid cid = it->first;
            if (connDesc_.at(cid).getDirection() == D2D_MULTI)
                triggerD2DMulticast = true;
            else
                trigger = true;
            break;
        }
    }

    if (!trigger && !triggerD2DMulticast)
    {
        EV << NOW << " UeMac::checkRAC - Ue " << nodeId_ << ",RAC aborted, no data in queues " << endl;
    }

    if ((racRequested_=trigger) || (racD2DMulticastRequested_=triggerD2DMulticast))
    {
        auto pkt = new Packet("RacRequest");
        double carrierFrequency = phy_->getPrimaryChannelModel()->getCarrierFrequency();
        pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFrequency);
        pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDestId(getMacCellId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDirection(UL);
        pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(RACPKT);

        auto racReq = makeShared<LteRac>();

        pkt->insertAtFront(racReq);
        sendLowerPackets(pkt);

        EV << NOW << " Ue  " << nodeId_ << " cell " << cellId_ << " ,RAC request sent to PHY " << endl;

        // wait at least  "raRespWinStart_" TTIs before another RAC request
        // raRespWinStart_ is defined in LteMacUe::LteMacUe(), default value is 3
        raRespTimer_ = raRespWinStart_;
    }
}


void UeMac::macHandleRac(cPacket* pktAux)
{
    auto pkt = check_and_cast<inet::Packet*> (pktAux);
    auto racPkt = pkt->peekAtFront<LteRac>();

    if (racPkt->getSuccess())
    {
        EV << "UeMac::macHandleRac - Ue " << nodeId_ << " won RAC" << endl;
        // is RAC is won, BSR has to be sent
        if (racD2DMulticastRequested_)
            bsrD2DMulticastTriggered_=true;
        else
            bsrTriggered_ = true;

        // reset RAC counter
        currentRacTry_=0;
        //reset RAC backoff timer
        racBackoffTimer_=0;
    }
    else
    {
        // RAC has failed
        if (++currentRacTry_ >= maxRacTryouts_)
        {
            EV << NOW << " Ue " << nodeId_ << ", RAC reached max attempts : " << currentRacTry_ << endl;
            // no more RAC allowed
            //! TODO flush all buffers here
            //reset RAC counter
            currentRacTry_=0;
            //reset RAC backoff timer
            racBackoffTimer_=0;
        }
        else
        {
            // recompute backoff timer
            racBackoffTimer_= uniform(minRacBackoff_,maxRacBackoff_);
            EV << NOW << " Ue " << nodeId_ << " RAC attempt failed, backoff extracted : " << racBackoffTimer_ << endl;
        }
    }
    delete pkt;
    pkt = nullptr;
}
