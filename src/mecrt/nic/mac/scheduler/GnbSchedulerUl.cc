//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// a single layer of NRSchedulerGnbUl class in simu5g
// simulate the MAC stack of the NIC module of gNB
// LteSchedulerEnb --> LteSchedulerEnbUl --> NRSchedulerGnbUl
//

#include "mecrt/nic/mac/scheduler/GnbSchedulerUl.h"
#include "mecrt/nic/mac/scheme/FDSchemeUl.h"

#include "common/LteCommon.h"

#include "stack/mac/allocator/LteAllocationModuleFrequencyReuse.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/scheduler/LteScheduler.h"
#include "stack/mac/scheduling_modules/LteDrr.h"
#include "stack/mac/scheduling_modules/LteMaxCi.h"
#include "stack/mac/scheduling_modules/LtePf.h"
#include "stack/mac/scheduling_modules/LteMaxCiMultiband.h"
#include "stack/mac/scheduling_modules/LteMaxCiOptMB.h"
#include "stack/mac/scheduling_modules/LteMaxCiComp.h"
#include "stack/mac/scheduling_modules/LteAllocatorBestFit.h"
#include "stack/phy/layer/LtePhyBase.h"

using namespace omnetpp;

void GnbSchedulerUl::initialize(Direction dir, LteMacEnb* mac)
{
    EV << "GnbSchedulerUl::initialize - initialize the downlink scheduler." << endl;

    direction_ = dir;
    mac_ = mac;

    rbPerBand_ = check_and_cast<GnbMac*>(mac_)->getRbPerBand();

    binder_ = getBinder();

    vbuf_ = mac_->getMacBuffers();
    bsrbuf_ = mac_->getBsrVirtualBuffers();

    harqTxBuffers_ = mac_->getHarqTxBuffers();
    harqRxBuffers_ = mac_->getHarqRxBuffers();

    // Create LteScheduler. One per carrier
    // SchedDiscipline discipline = mac_->getSchedDiscipline(direction_);
    std::string discipline_name = mac_->par("schedulingDisciplineUl").stdstringValue();
    SchedDiscipline discipline = getSchedDiscipline(discipline_name);

    LteScheduler* newSched = NULL;
    const CarrierInfoMap* carriers = mac_->getCellInfo()->getCarrierInfoMap();
    CarrierInfoMap::const_iterator it = carriers->begin();
    for ( ; it != carriers->end(); ++it)
    {
        newSched = getScheduler(discipline, discipline_name);
        newSched->setEnbScheduler(this);
        newSched->setCarrierFrequency(it->second.carrierFrequency);
        newSched->setNumerologyIndex(it->second.numerologyIndex);     // set periodicity for this scheduler according to numerology
        newSched->initializeBandLimit();
        scheduler_.push_back(newSched);
    }

    // Create Allocator
    //if (discipline == ALLOCATOR_BESTFIT)   // NOTE: create this type of allocator for every scheduler using Frequency Reuse
    //    allocator_ = new LteAllocationModuleFrequencyReuse(mac_, direction_);
    //else
    //    allocator_ = new LteAllocationModule(mac_, direction_);
    allocator_ = new GnbAllocationModule(mac_, direction_);
    this->LteSchedulerEnb::allocator_ = allocator_;

    // initialize the allocator
    /***
     * the value of resourceBlocks_ is set in GnbMac::initialize() after initializing GnbSchedulerDl.
     * resourceBlocks_ = mac_->getCellInfo()->getNumBands();
     *      origin: initializeAllocator();
     */
    allocator_->init(resourceBlocks_, mac_->getCellInfo()->getNumBands());

    // Initialize statistics
    avgServedBlocksDl_ = mac_->registerSignal("avgServedBlocksDl");
    avgServedBlocksUl_ = mac_->registerSignal("avgServedBlocksUl");
}


SchedDiscipline GnbSchedulerUl::getSchedDiscipline(std::string name)
{
    int i = 0;
    while (disciplines[i].discipline != UNKNOWN_DISCIPLINE)
    {
        if (disciplines[i].disciplineName == name)
            return disciplines[i].discipline;
        i++;
    }

    return UNKNOWN_DISCIPLINE;
}


/***
 * add this function such that we do not need to modify the SchedDiscipline in
 * file LteCommonEnum_m.h when we add new scheduling scheme
 */
LteScheduler* GnbSchedulerUl::getScheduler(SchedDiscipline discipline, std::string discipline_name)
{
    if ((discipline == UNKNOWN_DISCIPLINE) && (discipline_name == "FDSchemeUl"))
    {
        EV << "GnbSchedulerUl::getScheduler - Creating gNB downlink scheduler FDSchemeUl" << endl;
        FDSchemeUl* newSchedule = new FDSchemeUl();
        /* add the mac pointer is because the new scheme is not a friend class of LteSchedulerEnb,
         * and we want to avoid changing the original file
         */
        newSchedule->setGnbSchedulerUl(this);

        return newSchedule;
    }
    else
        return getScheduler(discipline);
}


LteScheduler* GnbSchedulerUl::getScheduler(SchedDiscipline discipline)
{
    EV << "GnbSchedulerUl::getScheduler - Creating LteScheduler " << schedDisciplineToA(discipline) << endl;

    switch(discipline)
    {
        case DRR:
        return new LteDrr();
        case PF:
        return new LtePf(mac_->par("pfAlpha").doubleValue());
        case MAXCI:
        return new LteMaxCi();
        case MAXCI_MB:
        return new LteMaxCiMultiband();
        case MAXCI_OPT_MB:
        return new LteMaxCiOptMB();
        case MAXCI_COMP:
        return new LteMaxCiComp();
        case ALLOCATOR_BESTFIT:
        return new LteAllocatorBestFit();

        default:
        throw cRuntimeError("LteScheduler not recognized");
        return nullptr;
    }
}


bool GnbSchedulerUl::racschedule(double carrierFrequency, BandLimitVector* bandLim)
{
    EV << NOW << " GnbSchedulerUl::racschedule --------------------::[ START RAC-SCHEDULE ]::--------------------" << endl;
    EV << NOW << " GnbSchedulerUl::racschedule - eNodeB: " << mac_->getMacCellId() << " Direction: " << (direction_ == UL ? "UL" : "DL") << endl;

    // Get number of logical bands
    unsigned int numBands = mac_->getCellInfo()->getNumBands();
    unsigned int racAllocatedBlocks = 0;

    std::map<double, RacStatus>::iterator map_it = racStatus_.find(carrierFrequency);
    if (map_it != racStatus_.end())
    {
        RacStatus& racStatus = map_it->second;
        RacStatus::iterator it=racStatus.begin() , et=racStatus.end();
        for (;it!=et;++it)
        {
            // get current nodeId
            MacNodeId nodeId = it->first;
            EV << NOW << " GnbSchedulerUl::racschedule - handling RAC for node " << nodeId << endl;

            const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, UL, carrierFrequency);    // get the user info
            const std::set<Band>& allowedBands = txParams.readBands();
            BandLimitVector tempBandLim;
            tempBandLim.clear();
            std::string bands_msg = "BAND_LIMIT_SPECIFIED";
            if (bandLim == nullptr)
            {
                // Create a vector of band limit using all bands
                // FIXME: bandlim is never deleted

                // for each band of the band vector provided
                for (unsigned int i = 0; i < numBands; i++)
                {
                    BandLimit elem;
                    // copy the band
                    elem.band_ = Band(i);
                    EV << "Putting band " << i << endl;
                    for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
                    {
                        if( allowedBands.find(elem.band_)!= allowedBands.end() )
                        {
//                            EV << "\t" << i << " " << "yes" << endl;
                            elem.limit_[j]=-1;
                        }
                        else
                        {
//                            EV << "\t" << i << " " << "no" << endl;
                            elem.limit_[j]=-2;
                        }
                    }
                    tempBandLim.push_back(elem);
                }
                bandLim = &tempBandLim;
            }
            else
            {
                // for each band of the band vector provided
                for (unsigned int i = 0; i < numBands; i++)
                {
                    BandLimit& elem = bandLim->at(i);
                    for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
                    {
                        if (elem.limit_[j] == -2)
                            continue;

                        if (allowedBands.find(elem.band_)!= allowedBands.end() )
                        {
//                            EV << "\t" << i << " " << "yes" << endl;
                            elem.limit_[j]=-1;
                        }
                        else
                        {
//                            EV << "\t" << i << " " << "no" << endl;
                            elem.limit_[j]=-2;
                        }
                    }
                }
            }

            // FIXME default behavior
            //try to allocate one block to selected UE on at least one logical band of MACRO antenna, first codeword

            const unsigned int cw =0;
            // as we consider band as the smallest resource unit for allocation, change the default value
            // const unsigned int blocks = 1;
            const unsigned int blocks = rbPerBand_;

            bool allocation=false;

            unsigned int size = bandLim->size();
            for (Band b=0;b<size;++b)
            {
                // if the limit flag is set to skip, jump off
                int limit = bandLim->at(b).limit_.at(cw);
                if (limit == -2)
                {
                    EV << "GnbSchedulerUl::racschedule - skipping logical band according to limit value" << endl;
                    continue;
                }

                if ( allocator_->availableBlocks(nodeId,MACRO,b) >0)
                {
                    unsigned int bytes = mac_->getAmc()->computeBytesOnNRbs(nodeId,b,cw,blocks,UL,carrierFrequency);
                    if (bytes > 0)
                    {

                        allocator_->addBlocks(MACRO,b,nodeId,1,bytes);
                        racAllocatedBlocks += blocks;

                        EV << NOW << "GnbSchedulerUl::racschedule - UE: " << nodeId << "Handled RAC on band: " << b << endl;

                        allocation=true;
                        break;
                    }
                }
            }

            if (allocation)
            {
                // create scList id for current cid/codeword
                MacCid cid = idToMacCid(nodeId, SHORT_BSR);  // build the cid. Since this grant will be used for a BSR,
                                                             // we use the LCID corresponding to the SHORT_BSR
                std::pair<unsigned int,Codeword> scListId = std::pair<unsigned int,Codeword>(cid,cw);
                scheduleList_[carrierFrequency][scListId]=blocks;
            }
        }

        // clean up all requests
        racStatus.clear();
    }

    if (racAllocatedBlocks < resourceBlocks_)
    {
        // serve RAC for background UEs
        racscheduleBackground(racAllocatedBlocks, carrierFrequency, bandLim);
    }

    // update available blocks
    unsigned int availableBlocks = resourceBlocks_ - racAllocatedBlocks;

    EV << NOW << " GnbSchedulerUl::racschedule --------------------::[  END RAC-SCHEDULE  ]::--------------------" << endl;

    return (availableBlocks==0);
}

void GnbSchedulerUl::racscheduleBackground(unsigned int& racAllocatedBlocks, double carrierFrequency, BandLimitVector* bandLim)
{
    EV << NOW << " GnbSchedulerUl::racscheduleBackground - scheduling RAC for background UEs" << endl;

    std::list<MacNodeId> servedRac;

    BackgroundTrafficManager* bgTrafficManager = mac_->getBackgroundTrafficManager(carrierFrequency);
    std::list<int>::const_iterator it = bgTrafficManager->getWaitingForRacUesBegin(),
                                   et = bgTrafficManager->getWaitingForRacUesEnd();


    // Get number of logical bands
    unsigned int numBands = mac_->getCellInfo()->getNumBands();

    for (;it!=et;++it)
    {
        // get current nodeId
        MacNodeId bgUeId = *it + BGUE_MIN_ID;

        EV << NOW << " GnbSchedulerUl::racscheduleBackground handling RAC for node " << bgUeId << endl;

        BandLimitVector tempBandLim;
        tempBandLim.clear();
        std::string bands_msg = "BAND_LIMIT_SPECIFIED";
        if (bandLim == nullptr)
        {
            // Create a vector of band limit using all bands

            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++)
            {
                BandLimit elem;
                // copy the band
                elem.band_ = Band(i);
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
                {
                    elem.limit_[j]=-1;
                }
                tempBandLim.push_back(elem);
            }
            bandLim = &tempBandLim;
        }

        // FIXME default behavior
        //try to allocate one block to selected UE on at least one logical band of MACRO antenna, first codeword

        const unsigned int cw =0;
        // as we consider band as the smallest resource unit for allocation, change the default value
        // const unsigned int blocks = 1;
        const unsigned int blocks = rbPerBand_;

        unsigned int size = bandLim->size();
        for (Band b=0;b<size;++b)
        {
            // if the limit flag is set to skip, jump off
            int limit = bandLim->at(b).limit_.at(cw);
            if (limit == -2)
            {
                EV << "GnbSchedulerUl::racscheduleBackground - skipping logical band according to limit value" << endl;
                continue;
            }

            if ( allocator_->availableBlocks(bgUeId,MACRO,b) >0)
            {
                unsigned int bytes = blocks * (bgTrafficManager->getBackloggedUeBytesPerBlock(bgUeId, UL));
                if (bytes > 0)
                {
                    allocator_->addBlocks(MACRO,b,bgUeId,blocks,bytes);
                    racAllocatedBlocks += blocks;

                    servedRac.push_back(bgUeId);

                    EV << NOW << "GnbSchedulerUl::racscheduleBackground UE: " << bgUeId << "Handled RAC on band: " << b << endl;

                    break;
                }
            }
        }
    }

    while (!servedRac.empty())
    {
        // notify the traffic manager that the RAC for this UE has been served
        bgTrafficManager->racHandled(servedRac.front());
        servedRac.pop_front();
    }
}



/***
 * Not a real override.
 */
void GnbSchedulerUl::backlog(MacCid cid)
{
    EV << "GnbSchedulerUl::backlog - backlogged data for Logical Cid " << cid << endl;
    if(cid == 1)
        return;

    EV << NOW << "GnbSchedulerUl::backlog CID notified " << cid << endl;
    activeConnectionSet_.insert(cid);

    std::vector<LteScheduler*>::iterator it = scheduler_.begin();
    for ( ; it != scheduler_.end(); ++it)
        (*it)->notifyActiveConnection(cid);
}

std::map<double, LteMacScheduleList>* GnbSchedulerUl::schedule()
{
    EV << "GnbSchedulerUl::schedule - performed by Node: " << mac_->getMacNodeId() << endl;

    // clearing structures for new scheduling
    std::map<double, LteMacScheduleList>::iterator lit = scheduleList_.begin();
    for (; lit != scheduleList_.end(); ++lit)
        lit->second.clear();
    allocatedCws_.clear();

    // clean the allocator
    //resetAllocator();
    allocator_->reset(resourceBlocks_, mac_->getCellInfo()->getNumBands());

    // schedule one carrier at a time
    LteScheduler* scheduler = NULL;
    std::vector<LteScheduler*>::iterator it = scheduler_.begin();
    for ( ; it != scheduler_.end(); ++it)
    {
        scheduler = *it;
        EV << "GnbSchedulerUl::schedule - carrier [" << scheduler->getCarrierFrequency() << "]" << endl;

        unsigned int counter = scheduler->decreaseSchedulerPeriodCounter();
        if (counter > 0)
        {
            EV << " GnbSchedulerUl::schedule - not my turn (counter=" << counter << ")"<< endl;
            continue;
        }

        // scheduling of rac requests, retransmissions and transmissions
        EV << "________________________start RAC+RTX _______________________________" << endl;
        if ( !(scheduler->scheduleRacRequests()) && !(scheduler->scheduleRetransmissions()) )
        {
            EV << "___________________________end RAC+RTX ________________________________" << endl;
            EV << "___________________________start SCHED ________________________________" << endl;
            scheduler->updateSchedulingInfo();
            scheduler->schedule();
            EV << "____________________________ end SCHED ________________________________" << endl;
        }
    }

    // record assigned resource blocks statistics
    resourceBlockStatistics();

    return &scheduleList_;
}

bool GnbSchedulerUl::rtxschedule(double carrierFrequency, BandLimitVector* bandLim)
{
    try
    {
        EV << NOW << " GnbSchedulerUl::rtxschedule --------------------::[ START RTX-SCHEDULE ]::--------------------" << endl;
        EV << NOW << " GnbSchedulerUl::rtxschedule - gNodeB: " << mac_->getMacCellId() << " - Direction: " << (direction_ == UL ? "UL" : "DL") << endl;

        // retrieving reference to HARQ entities
        HarqRxBuffers* harqQueues = mac_->getHarqRxBuffers(carrierFrequency);
        if (harqQueues != NULL)
        {
            HarqRxBuffers::iterator it = harqQueues->begin();
            HarqRxBuffers::iterator et = harqQueues->end();

            for(; it != et; ++it)
            {
                // get current nodeId
                MacNodeId nodeId = it->first;

                if(nodeId == 0){
                    // UE has left the simulation - erase queue and continue
                    harqRxBuffers_->at(carrierFrequency).erase(nodeId);
                    continue;
                }
                OmnetId id = binder_->getOmnetId(nodeId);
                if(id == 0){
                    harqRxBuffers_->at(carrierFrequency).erase(nodeId);
                    continue;
                }

                LteHarqBufferRx* currHarq = it->second;

                // Get user transmission parameters
                const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, direction_, carrierFrequency);// get the user info
                // TODO SK Get the number of codewords - FIX with correct mapping
                // TODO is there a way to get codewords without calling computeTxParams??
                unsigned int codewords = txParams.getLayers().size();// get the number of available codewords

                EV << NOW << " GnbSchedulerUl::rtxschedule - UE: " << nodeId << endl;

                // get the number of HARQ processes
                unsigned int process=0;
                unsigned int maxProcesses = currHarq->getProcesses();

                for(process = 0; process < maxProcesses; ++process )
                {
                    // for each HARQ process
                    LteHarqProcessRx * currProc = currHarq->getProcess(process);

                    if (allocatedCws_[nodeId] == codewords)
                        break;

                    unsigned int allocatedBytes =0;
                    for(Codeword cw = 0; (cw < MAX_CODEWORDS) && (codewords>0); ++cw)
                    {
                        if (allocatedCws_[nodeId]==codewords)
                            break;

                        // skip processes which are not in rtx status
                        if (currProc->getUnitStatus(cw) != RXHARQ_PDU_CORRUPTED)
                        {
                            EV << NOW << " GnbSchedulerUl::rtxschedule - UE " << nodeId << " - detected Acid: " << process << " in status " << currProc->getUnitStatus(cw) << endl;
                            continue;
                        }

                        // if the process is in CORRUPTED state, then schedule a retransmission for this process

                        unsigned int rtxBytes=0;
                        // FIXME PERFORMANCE: check for rtx status before calling rtxAcid

                        // perform a retransmission on available codewords for the selected acid
                        rtxBytes=schedulePerAcidRtx(nodeId, carrierFrequency, cw,process,bandLim);
                        if (rtxBytes>0)
                        {
                            --codewords;
                            allocatedBytes+=rtxBytes;

                            check_and_cast<LteMacEnb*>(mac_)->signalProcessForRtx(nodeId, carrierFrequency, UL, false);
                        }
                    }
                    EV << NOW << "GnbSchedulerUl::rtxschedule - UE " << nodeId << " - allocated bytes : " << allocatedBytes << endl;
                 }
             }
        }
        if (mac_->isD2DCapable())
        {
            // --- START Schedule D2D retransmissions --- //
            Direction dir = D2D;
            HarqBuffersMirrorD2D* harqBuffersMirrorD2D = check_and_cast<LteMacEnbD2D*>(mac_)->getHarqBuffersMirrorD2D(carrierFrequency);
            if (harqBuffersMirrorD2D != NULL)
            {
                HarqBuffersMirrorD2D::iterator it_d2d = harqBuffersMirrorD2D->begin() , et_d2d=harqBuffersMirrorD2D->end();
                while (it_d2d != et_d2d)
                {

                    // get current nodeIDs
                    MacNodeId senderId = (it_d2d->first).first; // Transmitter
                    MacNodeId destId = (it_d2d->first).second;  // Receiver

                    if(senderId == 0 || binder_->getOmnetId(senderId) == 0) {
                        // UE has left the simulation - erase queue and continue
                        harqBuffersMirrorD2D->erase(it_d2d++);
                        continue;
                    }
                    if(destId == 0 || binder_->getOmnetId(destId) == 0) {
                        // UE has left the simulation - erase queue and continue
                        harqBuffersMirrorD2D->erase(it_d2d++);
                        continue;
                    }

                    LteHarqBufferMirrorD2D* currHarq = it_d2d->second;

                    // Get user transmission parameters
                    const UserTxParams& txParams = mac_->getAmc()->computeTxParams(senderId, dir,carrierFrequency);// get the user info

                    unsigned int codewords = txParams.getLayers().size();// get the number of available codewords
                    unsigned int allocatedBytes =0;

                    // TODO handle the codewords join case (size of(cw0+cw1) < currentTbs && currentLayers ==1)

                    EV << NOW << " GnbSchedulerUl::rtxschedule - D2D TX UE: " << senderId << " - RX UE: " << destId <<endl;

                    // get the number of HARQ processes
                    unsigned int process=0;
                    unsigned int maxProcesses = currHarq->getProcesses();

                    for(process = 0; process < maxProcesses; ++process )
                    {
                        // for each HARQ process
                        LteHarqProcessMirrorD2D* currProc = currHarq->getProcess(process);

                        if (allocatedCws_[senderId] == codewords)
                            break;

                        for(Codeword cw = 0; (cw < MAX_CODEWORDS) && (codewords>0); ++cw)
                        {
                            EV << NOW << " GnbSchedulerUl::rtxschedule - process " << process << endl;
                            EV << NOW << " GnbSchedulerUl::rtxschedule - ------- CODEWORD " << cw << endl;

                            // skip processes which are not in rtx status
                            if (currProc->getUnitStatus(cw) != TXHARQ_PDU_BUFFERED)
                            {
                                EV << NOW << " GnbSchedulerUl::rtxschedule - D2D UE: " << senderId << " detected Acid: " << process << " in status " << currProc->getUnitStatus(cw) << endl;
                                continue;
                            }

                            unsigned int rtxBytes=0;
                            // FIXME PERFORMANCE: check for rtx status before calling rtxAcid

                            // perform a retransmission on available codewords for the selected acid
                            rtxBytes = schedulePerAcidRtxD2D(destId, senderId,  carrierFrequency, cw, process, bandLim);
                            if (rtxBytes>0)
                            {
                                --codewords;
                                allocatedBytes+=rtxBytes;

                                check_and_cast<LteMacEnb*>(mac_)->signalProcessForRtx(senderId, carrierFrequency, D2D, false);
                            }
                        }
                        EV << NOW << " GnbSchedulerUl::rtxschedule - D2D UE: " << senderId << " allocated bytes : " << allocatedBytes << endl;
                    }
                    ++it_d2d;
                }
            }
            // --- END Schedule D2D retransmissions --- //
        }

        int availableBlocks = allocator_->computeTotalRbs();

        EV << NOW << " GnbSchedulerUl::rtxschedule - residual OFDM Space: " << availableBlocks << endl;
        EV << NOW << " GnbSchedulerUl::rtxschedule --------------------::[  END RTX-SCHEDULE  ]::--------------------" << endl;

        return (availableBlocks == 0);
    }
    catch(std::exception& e)
    {
        throw cRuntimeError("Exception in GnbSchedulerUl::rtxschedule(): %s", e.what());
    }
    return 0;
}


bool GnbSchedulerUl::rtxscheduleBackground(double carrierFrequency, BandLimitVector* bandLim)
{
    try
    {
        EV << NOW << " GnbSchedulerUl::rtxscheduleBackground --------------------::[ START RTX-SCHEDULE-BACKGROUND ]::--------------------" << endl;
        EV << NOW << " GnbSchedulerUl::rtxscheduleBackground eNodeB: " << mac_->getMacCellId() << " Direction: " << (direction_ == UL ? "UL" : "DL") << endl;

        // --- Schedule RTX for background UEs --- //
        std::map<int, unsigned int> bgScheduledRtx;
        BackgroundTrafficManager* bgTrafficManager = mac_->getBackgroundTrafficManager(carrierFrequency);
        std::list<int>::const_iterator it = bgTrafficManager->getBackloggedUesBegin(direction_, true),
                                       et = bgTrafficManager->getBackloggedUesEnd(direction_, true);
        for (; it != et; ++it)
        {
            int bgUeIndex = *it;
            MacNodeId bgUeId = BGUE_MIN_ID + bgUeIndex;

            unsigned cw = 0;
            unsigned int rtxBytes = scheduleBgRtx(bgUeId, carrierFrequency, cw, bandLim);
            if (rtxBytes > 0)
                bgScheduledRtx[bgUeId] = rtxBytes;
            EV << NOW << "GnbSchedulerUl::rtxscheduleBackground BG UE " << bgUeId << " - allocated bytes : " << rtxBytes << endl;
        }

        // consume bytes
        for (auto it = bgScheduledRtx.begin(); it != bgScheduledRtx.end(); ++it)
            bgTrafficManager->consumeBackloggedUeBytes(it->first, it->second, direction_, true); // in bytes

        int availableBlocks = allocator_->computeTotalRbs();

        EV << NOW << " GnbSchedulerUl::rtxscheduleBackground residual OFDM Space: " << availableBlocks << endl;

        EV << NOW << " GnbSchedulerUl::rtxscheduleBackground --------------------::[  END RTX-SCHEDULE-BACKGROUND ]::--------------------" << endl;

        return (availableBlocks == 0);
    }
    catch(std::exception& e)
    {
        throw cRuntimeError("Exception in GnbSchedulerUl::rtxscheduleBackground(): %s", e.what());
    }
    return 0;
}

unsigned int GnbSchedulerUl::schedulePerAcidRtx(MacNodeId nodeId, double carrierFrequency, Codeword cw, unsigned char acid,
    std::vector<BandLimit>* bandLim, Remote antenna, bool limitBl)
{
    try
    {
        const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, direction_,carrierFrequency);    // get the user info
        const std::set<Band>& allowedBands = txParams.readBands();
        BandLimitVector tempBandLim;
        tempBandLim.clear();
        std::string bands_msg = "BAND_LIMIT_SPECIFIED";
        if (bandLim == nullptr)
        {
            // Create a vector of band limit using all bands
            // FIXME: bandlim is never deleted

            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++)
            {
                BandLimit elem;
                // copy the band
                elem.band_ = Band(i);
                EV << "Putting band " << i << endl;
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
                {
                    if( allowedBands.find(elem.band_)!= allowedBands.end() )
                    {
//                        EV << "\t" << i << " " << "yes" << endl;
                        elem.limit_[j]=-1;
                    }
                    else
                    {
//                        EV << "\t" << i << " " << "no" << endl;
                        elem.limit_[j]=-2;
                    }
                }
                tempBandLim.push_back(elem);
            }
            bandLim = &tempBandLim;
        }
        else
        {
            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++)
            {
                BandLimit& elem = bandLim->at(i);
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
                {
                    if (elem.limit_[j] == -2)
                        continue;

                    if (allowedBands.find(elem.band_)!= allowedBands.end() )
                    {
//                        EV << "\t" << i << " " << "yes" << endl;
                        elem.limit_[j]=-1;
                    }
                    else
                    {
//                        EV << "\t" << i << " " << "no" << endl;
                        elem.limit_[j]=-2;
                    }
                }
            }
        }

        EV << NOW << " GnbSchedulerUl::schedulePerAcidRtx - Node[" << mac_->getMacNodeId() << ", User[" << nodeId << ", Codeword[ " << cw << "], ACID[" << (unsigned int)acid << "] " << endl;

        LteHarqProcessRx* currentProcess = harqRxBuffers_->at(carrierFrequency).at(nodeId)->getProcess(acid);

        if (currentProcess->getUnitStatus(cw) != RXHARQ_PDU_CORRUPTED)
        {
            // exit if the current active HARQ process is not ready for retransmission
            EV << NOW << " GnbSchedulerUl::schedulePerAcidRtx - User is on ACID " << (unsigned int)acid << " HARQ process is IDLE. No RTX scheduled ." << endl;
            return 0;
        }

        Codeword allocatedCw = 0;
        // search for already allocated codeword
        // create "mirror" scList ID for other codeword than current
        std::pair<unsigned int, Codeword> scListMirrorId = std::pair<unsigned int, Codeword>(idToMacCid(nodeId,SHORT_BSR), MAX_CODEWORDS - cw - 1);
        if (scheduleList_.find(carrierFrequency) != scheduleList_.end())
        {
            if (scheduleList_[carrierFrequency].find(scListMirrorId) != scheduleList_[carrierFrequency].end())
            {
                allocatedCw = MAX_CODEWORDS - cw - 1;
            }
        }
        // get current process buffered PDU byte length
        unsigned int bytes = currentProcess->getByteLength(cw);
        // bytes to serve
        unsigned int toServe = bytes;
        // blocks to allocate for each band
        std::vector<unsigned int> assignedBlocks;
        // bytes which blocks from the preceding vector are supposed to satisfy
        std::vector<unsigned int> assignedBytes;

        // end loop signal [same as bytes>0, but more secure]
        bool finish = false;
        // for each band
        unsigned int size = bandLim->size();
        for (unsigned int i = 0; (i < size) && (!finish); ++i)
        {
            // save the band and the relative limit
            Band b = bandLim->at(i).band_;
            int limit = bandLim->at(i).limit_.at(cw);

            // TODO add support to multi CW
//            unsigned int bandAvailableBytes = // if a codeword has been already scheduled for retransmission, limit available blocks to what's been  allocated on that codeword
//                    ((allocatedCw == MAX_CODEWORDS) ? availableBytes(nodeId,antenna, b, cw) : mac_->getAmc()->blocks2bytes(nodeId, b, cw, allocator_->getBlocks(antenna,b,nodeId) , direction_));    // available space
            unsigned int bandAvailableBytes = availableBytes(nodeId, antenna, b, cw, direction_, carrierFrequency);

            // use the provided limit as cap for available bytes, if it is not set to unlimited
            if (limit >= 0)
                bandAvailableBytes = limit < (int) bandAvailableBytes ? limit : bandAvailableBytes;

            EV << NOW << " GnbSchedulerUl::schedulePerAcidRtx - BAND " << b << endl;
            EV << NOW << " GnbSchedulerUl::schedulePerAcidRtx - total bytes:" << bytes << " still to serve: " << toServe << " bytes" << endl;
            EV << NOW << " GnbSchedulerUl::schedulePerAcidRtx - Available: " << bandAvailableBytes << " bytes" << endl;

            unsigned int servedBytes = 0;
            // there's no room on current band for serving the entire request
            if (bandAvailableBytes < toServe)
            {
                // record the amount of served bytes
                servedBytes = bandAvailableBytes;
                // the request can be fully satisfied
            }
            else
            {
                // record the amount of served bytes
                servedBytes = toServe;
                // signal end loop - all data have been serviced
                finish = true;
            }
            unsigned int servedBlocks = (servedBytes == 0) ? 0 : rbPerBand_;
            // update the bytes counter
            toServe -= servedBytes;
            // update the structures
            assignedBlocks.push_back(servedBlocks);
            assignedBytes.push_back(servedBytes);
        }

        if (toServe > 0)
        {
            // process couldn't be served - no sufficient space on available bands
            EV << NOW << " GnbSchedulerUl::schedulePerAcidRtx - Unavailable space for serving node " << nodeId << " ,HARQ Process " << (unsigned int)acid << " on codeword " << cw << endl;
            return 0;
        }
        else
        {
            // record the allocation
            unsigned int size = assignedBlocks.size();
            unsigned int cwAllocatedBlocks =0;

            // create scList id for current node/codeword
            std::pair<unsigned int,Codeword> scListId = std::pair<unsigned int,Codeword>(idToMacCid(nodeId, SHORT_BSR), cw);

            for(unsigned int i = 0; i < size; ++i)
            {
                // For each LB for which blocks have been allocated
                Band b = bandLim->at(i).band_;

                cwAllocatedBlocks +=assignedBlocks.at(i);
                EV << "\t Cw->" << allocatedCw << "/" << MAX_CODEWORDS << endl;
                //! handle multi-codeword allocation
                if (allocatedCw!=MAX_CODEWORDS)
                {
                    EV << NOW << " GnbSchedulerUl::schedulePerAcidRtx - adding " << assignedBlocks.at(i) << " to band " << i << endl;
                    allocator_->addBlocks(antenna,b,nodeId,assignedBlocks.at(i),assignedBytes.at(i));
                }
                //! TODO check if ok bandLim->at.limit_.at(cw) = assignedBytes.at(i);
            }

            // signal a retransmission
            // schedule list contains number of granted blocks

            scheduleList_[carrierFrequency][scListId] = cwAllocatedBlocks;
            // mark codeword as used
            if (allocatedCws_.find(nodeId)!=allocatedCws_.end())
            {
                allocatedCws_.at(nodeId)++;
            }
            else
            {
                allocatedCws_[nodeId]=1;
            }

            EV << NOW << " GnbSchedulerUl::schedulePerAcidRtx - HARQ Process " << (unsigned int)acid << " : " << bytes << " bytes served! " << endl;

            return bytes;
        }
    }
    catch(std::exception& e)
    {
        throw cRuntimeError("Exception in GnbSchedulerUl::schedulePerAcidRtx(): %s", e.what());
    }
    return 0;
}

unsigned int GnbSchedulerUl::schedulePerAcidRtxD2D(MacNodeId destId,MacNodeId senderId, double carrierFrequency, Codeword cw, unsigned char acid,
    std::vector<BandLimit>* bandLim, Remote antenna, bool limitBl)
{
    Direction dir = D2D;
    try
    {
        const UserTxParams& txParams = mac_->getAmc()->computeTxParams(senderId, dir,carrierFrequency);    // get the user info
        const std::set<Band>& allowedBands = txParams.readBands();
        BandLimitVector tempBandLim;
        tempBandLim.clear();
        std::string bands_msg = "BAND_LIMIT_SPECIFIED";
        if (bandLim == nullptr)
        {
            // Create a vector of band limit using all bands
            // FIXME: bandlim is never deleted

            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++)
            {
                BandLimit elem;
                // copy the band
                elem.band_ = Band(i);
                EV << "Putting band " << i << endl;
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
                {
                    if( allowedBands.find(elem.band_)!= allowedBands.end() )
                    {
                        EV << "\t" << i << " " << "yes" << endl;
                        elem.limit_[j]=-1;
                    }
                    else
                    {
                        EV << "\t" << i << " " << "no" << endl;
                        elem.limit_[j]=-2;
                    }
                }
                tempBandLim.push_back(elem);
            }
            bandLim = &tempBandLim;
        }
        else
        {
            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++)
            {
                BandLimit& elem = bandLim->at(i);
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
                {
                    if (elem.limit_[j] == -2)
                        continue;

                    if (allowedBands.find(elem.band_)!= allowedBands.end() )
                    {
                        EV << "\t" << i << " " << "yes" << endl;
                        elem.limit_[j]=-1;
                    }
                    else
                    {
                        EV << "\t" << i << " " << "no" << endl;
                        elem.limit_[j]=-2;
                    }
                }
            }
        }

        EV << NOW << " GnbSchedulerUl::schedulePerAcidRtxD2D - Node[" << mac_->getMacNodeId() << ", User[" << senderId << ", Codeword[ " << cw << "], ACID[" << (unsigned int)acid << "] " << endl;

        D2DPair pair(senderId, destId);

        // Get the current active HARQ process
        HarqBuffersMirrorD2D* harqBuffersMirrorD2D = check_and_cast<LteMacEnbD2D*>(mac_)->getHarqBuffersMirrorD2D(carrierFrequency);
        EV << "\t the acid that should be considered is " << (unsigned int)acid << endl;

        LteHarqProcessMirrorD2D* currentProcess = harqBuffersMirrorD2D->at(pair)->getProcess(acid);
        if (currentProcess->getUnitStatus(cw) != TXHARQ_PDU_BUFFERED)
        {
            // exit if the current active HARQ process is not ready for retransmission
            EV << NOW << " GnbSchedulerUl::schedulePerAcidRtxD2D - User is on ACID " << (unsigned int)acid << " HARQ process is IDLE. No RTX scheduled ." << endl;
            return 0;
        }

        Codeword allocatedCw = 0;
        //search for already allocated codeword
        //create "mirror" scList ID for other codeword than current
        std::pair<unsigned int, Codeword> scListMirrorId = std::pair<unsigned int, Codeword>(idToMacCid(senderId, D2D_SHORT_BSR), MAX_CODEWORDS - cw - 1);
        if (scheduleList_.find(carrierFrequency) != scheduleList_.end())
        {
            if (scheduleList_[carrierFrequency].find(scListMirrorId) != scheduleList_[carrierFrequency].end())
            {
                allocatedCw = MAX_CODEWORDS - cw - 1;
            }
        }
        // get current process buffered PDU byte length
        unsigned int bytes = currentProcess->getPduLength(cw);
        // bytes to serve
        unsigned int toServe = bytes;
        // blocks to allocate for each band
        std::vector<unsigned int> assignedBlocks;
        // bytes which blocks from the preceding vector are supposed to satisfy
        std::vector<unsigned int> assignedBytes;

        // end loop signal [same as bytes>0, but more secure]
        bool finish = false;
        // for each band
        unsigned int size = bandLim->size();
        for (unsigned int i = 0; (i < size) && (!finish); ++i)
        {
            // save the band and the relative limit
            Band b = bandLim->at(i).band_;
            int limit = bandLim->at(i).limit_.at(cw);

            // TODO add support to multi CW
            //unsigned int bandAvailableBytes = // if a codeword has been already scheduled for retransmission, limit available blocks to what's been  allocated on that codeword
            //((allocatedCw == MAX_CODEWORDS) ? availableBytes(nodeId,antenna, b, cw) : mac_->getAmc()->blocks2bytes(nodeId, b, cw, allocator_->getBlocks(antenna,b,nodeId) , direction_));    // available space
            unsigned int bandAvailableBytes = availableBytes(senderId, antenna, b, cw, dir, carrierFrequency);

            // use the provided limit as cap for available bytes, if it is not set to unlimited
            if (limit >= 0)
                bandAvailableBytes = limit < (int) bandAvailableBytes ? limit : bandAvailableBytes;

            EV << NOW << " GnbSchedulerUl::schedulePerAcidRtxD2D - BAND " << b << endl;
            EV << NOW << " GnbSchedulerUl::schedulePerAcidRtxD2D - total bytes:" << bytes << " still to serve: " << toServe << " bytes" << endl;
            EV << NOW << " GnbSchedulerUl::schedulePerAcidRtxD2D - Available: " << bandAvailableBytes << " bytes" << endl;

            unsigned int servedBytes = 0;
            // there's no room on current band for serving the entire request
            if (bandAvailableBytes < toServe)
            {
                // record the amount of served bytes
                servedBytes = bandAvailableBytes;
                // the request can be fully satisfied
            }
            else
            {
                // record the amount of served bytes
                servedBytes = toServe;
                // signal end loop - all data have been serviced
                finish = true;
                EV << NOW << " GnbSchedulerUl::schedulePerAcidRtxD2D - ALL DATA HAVE BEEN SERVICED"<< endl;
            }
            unsigned int servedBlocks = (servedBytes == 0) ? 0 : rbPerBand_;
            // update the bytes counter
            toServe -= servedBytes;
            // update the structures
            assignedBlocks.push_back(servedBlocks);
            assignedBytes.push_back(servedBytes);
        }

        if (toServe > 0)
        {
            // process couldn't be served - no sufficient space on available bands
            EV << NOW << " GnbSchedulerUl::schedulePerAcidRtxD2D - Unavailable space for serving node " << senderId << " ,HARQ Process " << (unsigned int)acid << " on codeword " << cw << endl;
            return 0;
        }
        else
        {
            // record the allocation
            unsigned int size = assignedBlocks.size();
            unsigned int cwAllocatedBlocks = 0;

            // create scList id for current cid/codeword
            std::pair<unsigned int,Codeword> scListId = std::pair<unsigned int,Codeword>(idToMacCid(senderId, D2D_SHORT_BSR), cw);

            for(unsigned int i = 0; i < size; ++i)
            {
                // For each LB for which blocks have been allocated
                Band b = bandLim->at(i).band_;

                cwAllocatedBlocks += assignedBlocks.at(i);
                EV << "\t Cw->" << allocatedCw << "/" << MAX_CODEWORDS << endl;
                //! handle multi-codeword allocation
                if (allocatedCw!=MAX_CODEWORDS)
                {
                    EV << NOW << " GnbSchedulerUl::schedulePerAcidRtxD2D - adding " << assignedBlocks.at(i) << " to band " << i << endl;
                    allocator_->addBlocks(antenna,b,senderId,assignedBlocks.at(i),assignedBytes.at(i));
                }
                //! TODO check if ok bandLim->at.limit_.at(cw) = assignedBytes.at(i);
            }

            // signal a retransmission
            // schedule list contains number of granted blocks
            scheduleList_[carrierFrequency][scListId] = cwAllocatedBlocks;
            // mark codeword as used
            if (allocatedCws_.find(senderId)!=allocatedCws_.end())
            {
                allocatedCws_.at(senderId)++;
            }
            else
            {
                allocatedCws_[senderId]=1;
            }

            EV << NOW << " GnbSchedulerUl::schedulePerAcidRtxD2D - HARQ Process " << (unsigned int)acid << " : " << bytes << " bytes served! " << endl;

            currentProcess->markSelected(cw);

            return bytes;
        }

    }
    catch(std::exception& e)
    {
        throw cRuntimeError("Exception in GnbSchedulerUl::schedulePerAcidRtxD2D(): %s", e.what());
    }
    return 0;
}

unsigned int GnbSchedulerUl::scheduleBgRtx(MacNodeId bgUeId, double carrierFrequency, Codeword cw, std::vector<BandLimit>* bandLim, Remote antenna, bool limitBl)
{
    try
    {
        BackgroundTrafficManager* bgTrafficManager = mac_->getBackgroundTrafficManager(carrierFrequency);
        unsigned int bytesPerBlock = bgTrafficManager->getBackloggedUeBytesPerBlock(bgUeId, direction_);

        // get the RTX buffer size
        unsigned int queueLength = bgTrafficManager->getBackloggedUeBuffer(bgUeId, direction_, true); // in bytes
        if (queueLength == 0)
            return 0;

        RbMap allocatedRbMap;

        BandLimitVector tempBandLim;
        tempBandLim.clear();
        if (bandLim == nullptr)
        {
            // Create a vector of band limit using all bands
            // FIXME: bandlim is never deleted

            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++)
            {
                BandLimit elem;
                // copy the band
                elem.band_ = Band(i);
                EV << "Putting band " << i << endl;
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
                {
                    elem.limit_[j]=-2;
                }
                tempBandLim.push_back(elem);
            }
            bandLim = &tempBandLim;
        }

        EV << NOW << " GnbSchedulerUl::scheduleBgRtx - Node[" << mac_->getMacNodeId() << ", User[" << bgUeId << "]"<< endl;

        Codeword allocatedCw = 0;

        // bytes to serve
        unsigned int toServe = queueLength;
        // blocks to allocate for each band
        std::vector<unsigned int> assignedBlocks;
        // bytes which blocks from the preceding vector are supposed to satisfy
        std::vector<unsigned int> assignedBytes;

        // end loop signal [same as bytes>0, but more secure]
        bool finish = false;
        // for each band
        unsigned int size = bandLim->size();
        for (unsigned int i = 0; (i < size) && (!finish); ++i)
        {
            // save the band and the relative limit
            Band b = bandLim->at(i).band_;
            int limit = bandLim->at(i).limit_.at(cw);

            unsigned int bandAvailableBytes = availableBytesBackgroundUe(bgUeId, antenna, b, direction_, carrierFrequency, (limitBl) ? limit : -1); // available space (in bytes)

            // use the provided limit as cap for available bytes, if it is not set to unlimited
            if (limit >= 0)
                bandAvailableBytes = limit < (int) bandAvailableBytes ? limit : bandAvailableBytes;

            EV << NOW << " GnbSchedulerUl::scheduleBgRtx BAND " << b << endl;
            EV << NOW << " GnbSchedulerUl::scheduleBgRtx total bytes:" << queueLength << " still to serve: " << toServe << " bytes" << endl;
            EV << NOW << " GnbSchedulerUl::scheduleBgRtx Available: " << bandAvailableBytes << " bytes" << endl;

            unsigned int servedBytes = 0;
            // there's no room on current band for serving the entire request
            if (bandAvailableBytes < toServe)
            {
                // record the amount of served bytes
                servedBytes = bandAvailableBytes;
                // the request can be fully satisfied
            }
            else
            {
                // record the amount of served bytes
                servedBytes = toServe;
                // signal end loop - all data have been serviced
                finish = true;
            }

            unsigned int servedBlocks = ceil((double)servedBytes/(bytesPerBlock * rbPerBand_)) * rbPerBand_;

            // update the bytes counter
            toServe -= servedBytes;
            // update the structures
            assignedBlocks.push_back(servedBlocks);
            assignedBytes.push_back(servedBytes);
        }

        if (toServe > 0)
        {
            // process couldn't be served - no sufficient space on available bands
            EV << NOW << " GnbSchedulerUl::scheduleBgRtx Unavailable space for serving node " << bgUeId << endl;
            return 0;
        }
        else
        {
            std::map<Band, unsigned int> allocatedRbMapEntry;

            // record the allocation
            unsigned int size = assignedBlocks.size();
            // unsigned int cwAllocatedBlocks =0;
            unsigned int allocatedBytes = 0;
            for(unsigned int i = 0; i < size; ++i)
            {
                allocatedRbMapEntry[i] = 0;

                // For each LB for which blocks have been allocated
                Band b = bandLim->at(i).band_;

                allocatedBytes += assignedBytes.at(i);
                // cwAllocatedBlocks +=assignedBlocks.at(i);
                allocatedRbMapEntry[i] += assignedBlocks.at(i);

                EV << "\t Cw->" << allocatedCw << "/" << MAX_CODEWORDS << endl;
                //! handle multi-codeword allocation
                if (allocatedCw!=MAX_CODEWORDS)
                {
                    EV << NOW << " GnbSchedulerUl::scheduleBgRtx - adding " << assignedBlocks.at(i) << " to band " << i << endl;
                    allocator_->addBlocks(antenna,b,bgUeId,assignedBlocks.at(i),assignedBytes.at(i));
                }
            }

            // signal a retransmission

            // mark codeword as used
            if (allocatedCws_.find(bgUeId)!=allocatedCws_.end())
                allocatedCws_.at(bgUeId)++;
            else
                allocatedCws_[bgUeId]=1;

            EV << NOW << " GnbSchedulerUl::scheduleBgRtx: " << allocatedBytes << " bytes served! " << endl;

            // update rb map
            allocatedRbMap[antenna] = allocatedRbMapEntry;

            // if uplink interference is enabled, mark the occupation in the ul transmission map (for ul interference computation purposes)
            LteChannelModel* channelModel = mac_->getPhy()->getChannelModel(carrierFrequency);
            if (channelModel->isUplinkInterferenceEnabled())
                binder_->storeUlTransmissionMap(carrierFrequency, antenna, allocatedRbMap, bgUeId, mac_->getMacCellId(), bgTrafficManager->getTrafficGenerator(bgUeId), UL);

            return allocatedBytes;
        }
    }
    catch(std::exception& e)
    {
        throw cRuntimeError("Exception in GnbSchedulerUl::scheduleBgRtx(): %s", e.what());
    }
    return 0;
}

unsigned int GnbSchedulerUl::availableBytesBackgroundUe(const MacNodeId id, Remote antenna, Band b, Direction dir, double carrierFrequency, int limit)
{
    EV << "GnbSchedulerUl::availableBytes MacNodeId " << id << " Antenna " << dasToA(antenna) << " band " << b << endl;
    // Retrieving this user available resource blocks
    int blocks = allocator_->availableBlocks(id,antenna,b);
    if (blocks == 0)
    {
        EV << "GnbSchedulerUl::availableBytes - No blocks available on band " << b << endl;
        return 0;
    }

    //Consistency Check
    if (limit>blocks && limit!=-1)
       throw cRuntimeError("GnbSchedulerUl::availableBytes signaled limit inconsistency with available space band b %d, limit %d, available blocks %d",b,limit,blocks);

    if (limit!=-1)
        blocks=(blocks>limit)?limit:blocks;

    unsigned int bytesPerBlock = mac_->getBackgroundTrafficManager(carrierFrequency)->getBackloggedUeBytesPerBlock(id, dir);
    unsigned int bytes = bytesPerBlock * blocks;
    EV << "GnbSchedulerUl::availableBytes MacNodeId " << id << " blocks [" << blocks << "], bytes [" << bytes << "]" << endl;

    return bytes;
}

/*  COMPLETE:        scheduleGrant(cid,bytes,terminate,active,eligible,band_limit,antenna);
 *  ANTENNA UNAWARE: scheduleGrant(cid,bytes,terminate,active,eligible,band_limit);
 *  BAND UNAWARE:    scheduleGrant(cid,bytes,terminate,active,eligible);
 */
unsigned int GnbSchedulerUl::scheduleGrant(MacCid cid, unsigned int bytes, bool& terminate, bool& active, bool& eligible, double carrierFrequency, BandLimitVector* bandLim, Remote antenna, bool limitBl)
{
    // Get the node ID and logical connection ID
    MacNodeId nodeId = MacCidToNodeId(cid);
    LogicalCid flowId = MacCidToLcid(cid);

    Direction dir = direction_;
    if (dir == UL)
    {
        // check if this connection is a D2D connection
        if (flowId == D2D_SHORT_BSR)
            dir = D2D;           // if yes, change direction
        if (flowId == D2D_MULTI_SHORT_BSR)
            dir = D2D_MULTI;     // if yes, change direction
        // else dir == UL
    }
    // else dir == DL

    // Get user transmission parameters
    const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, dir,carrierFrequency);
    const std::set<Band>& allowedBands = txParams.readBands();

    //get the number of codewords
    unsigned int numCodewords = txParams.getLayers().size();

    // TEST: check the number of codewords
    numCodewords = 1;

    EV << "GnbSchedulerUl::scheduleGrant - deciding allowed Bands" << endl;
    std::string bands_msg = "BAND_LIMIT_SPECIFIED";
    std::vector<BandLimit> tempBandLim;
    if (bandLim == nullptr )
    {
        bands_msg = "NO_BAND_SPECIFIED";

        txParams.print("grant()");

        emptyBandLim_.clear();
        // Create a vector of band limit using all bands
        if( emptyBandLim_.empty() )
        {
            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++)
            {
                BandLimit elem;
                // copy the band
                elem.band_ = Band(i);
                EV << "Putting band " << i << endl;
                // mark as unlimited
                for (unsigned int j = 0; j < numCodewords; j++)
                {
                    EV << "- Codeword " << j << endl;
                    if( allowedBands.find(elem.band_)!= allowedBands.end() )
                    {
                        EV << "\t" << i << " " << "yes" << endl;
                        elem.limit_[j]=-1;
                    }
                    else
                    {
                        EV << "\t" << i << " " << "no" << endl;
                        elem.limit_[j]=-2;
                    }
                }
                emptyBandLim_.push_back(elem);
            }
        }
        tempBandLim = emptyBandLim_;
        bandLim = &tempBandLim;
    }
    else
    {
        unsigned int numBands = mac_->getCellInfo()->getNumBands();
        // for each band of the band vector provided
        for (unsigned int i = 0; i < numBands; i++)
        {
            BandLimit& elem = bandLim->at(i);
            for (unsigned int j = 0; j < numCodewords; j++)
            {
                if (elem.limit_[j] == -2)
                    continue;

                if (allowedBands.find(elem.band_)!= allowedBands.end() )
                {
                    EV << "\t" << i << " " << "yes" << endl;
                    elem.limit_[j]=-1;
                }
                else
                {
                    EV << "\t" << i << " " << "no" << endl;
                    elem.limit_[j]=-2;
                }
            }
        }
    }
    EV << "GnbSchedulerUl::scheduleGrant(" << cid << "," << bytes << "," << terminate << "," << active << "," << eligible << "," << bands_msg << "," << dasToA(antenna) << ")" << endl;

    unsigned int totalAllocatedBytes = 0;  // total allocated data (in bytes)
    unsigned int totalAllocatedBlocks = 0; // total allocated data (in blocks)

    // === Perform normal operation for grant === //

    EV << "GnbSchedulerUl::scheduleGrant --------------------::[ START GRANT ]::--------------------" << endl;
    EV << "GnbSchedulerUl::scheduleGrant - Cell: " << mac_->getMacCellId() << endl;
    EV << "GnbSchedulerUl::scheduleGrant - CID: " << cid << "(UE: " << nodeId << ", Flow: " << flowId << ") current Antenna [" << dasToA(antenna) << "]" << endl;

    //! Multiuser MIMO support
    if (mac_->muMimo() && (txParams.readTxMode() == MULTI_USER))
    {
        // request AMC for MU_MIMO pairing
        MacNodeId peer = mac_->getAmc()->computeMuMimoPairing(nodeId, dir);
        if (peer != nodeId)
        {
            // this user has a valid pairing
            //1) register pairing  - if pairing is already registered false is returned
            if (allocator_->configureMuMimoPeering(nodeId, peer))
                EV << "GnbSchedulerUl::scheduleGrant - MU-MIMO pairing established: main user [" << nodeId << "], paired user [" << peer << "]" << endl;
            else
                EV << "GnbSchedulerUl::scheduleGrant - MU-MIMO pairing already exists between users [" << nodeId << "] and [" << peer << "]" << endl;
        }
        else
        {
            EV << "GnbSchedulerUl::scheduleGrant - no MU-MIMO pairing available for user [" << nodeId << "]" << endl;
        }
    }

    // registering DAS spaces to the allocator
    Plane plane = allocator_->getOFDMPlane(nodeId);
    allocator_->setRemoteAntenna(plane, antenna);

    // search for already allocated codeword
    unsigned int cwAlredyAllocated = 0;
    if (allocatedCws_.find(nodeId) != allocatedCws_.end())
        cwAlredyAllocated = allocatedCws_.at(nodeId);

    // Check OFDM space
    // OFDM space is not zero if this if we are trying to allocate the second cw in SPMUX or
    // if we are tryang to allocate a peer user in mu_mimo plane
    if (allocator_->computeTotalRbs() == 0 && (((txParams.readTxMode() != OL_SPATIAL_MULTIPLEXING &&
        txParams.readTxMode() != CL_SPATIAL_MULTIPLEXING) || cwAlredyAllocated == 0) &&
        (txParams.readTxMode() != MULTI_USER || plane != MU_MIMO_PLANE)))
    {
        terminate = true; // ODFM space ended, issuing terminate flag
        EV << "GnbSchedulerUl::scheduleGrant - Space ended, no schedulation." << endl;
        return 0;
    }

    // TODO This is just a BAD patch
    // check how a codeword may be reused (as in the if above) in case of non-empty OFDM space
    // otherwise check why an UE is stopped being scheduled while its buffer is not empty
    if (cwAlredyAllocated > 0)
    {
        terminate = true;
        return 0;
    }

    // ===== DEBUG OUTPUT ===== //
    bool debug = false; // TODO: make this configurable
    if (debug)
    {
        if (limitBl)
            EV << "GnbSchedulerUl::scheduleGrant - blocks: " << bytes << endl;
        else
            EV << "GnbSchedulerUl::scheduleGrant - Bytes: " << bytes << endl;
        EV << "GnbSchedulerUl::scheduleGrant - Bands: {";
        unsigned int size = (*bandLim).size();
        if (size > 0)
        {
            EV << (*bandLim).at(0).band_;
            for(unsigned int i = 1; i < size; i++)
                EV << ", " << (*bandLim).at(i).band_;
        }
        EV << "}\n";
    }
    // ===== END DEBUG OUTPUT ===== //

    EV << "GnbSchedulerUl::scheduleGrant - TxMode: " << txModeToA(txParams.readTxMode()) << endl;
    EV << "GnbSchedulerUl::scheduleGrant - Available codewords: " << numCodewords << endl;

    // Retrieve the first free codeword checking the eligibility - check eligibility could modify current cw index.
    Codeword cw = 0; // current codeword, modified by reference by the checkeligibility function
    if (!checkEligibility(nodeId, cw, carrierFrequency) || cw >= numCodewords)
    {
        eligible = false;

        EV << "GnbSchedulerUl::scheduleGrant - @@@@@ CODEWORD " << cw << " @@@@@" << endl;
        EV << "GnbSchedulerUl::scheduleGrant - Total allocation: " << totalAllocatedBytes << "bytes" << endl;
        EV << "GnbSchedulerUl::scheduleGrant - NOT ELIGIBLE!!!" << endl;
        EV << "GnbSchedulerUl::scheduleGrant --------------------::[  END GRANT  ]::--------------------" << endl;
        return totalAllocatedBytes; // return the total number of served bytes
    }

    // Get virtual buffer reference
    LteMacBuffer* conn = ((dir == DL) ? vbuf_->at(cid) : bsrbuf_->at(cid));

    // get the buffer size
    unsigned int queueLength = conn->getQueueOccupancy(); // in bytes
    if (queueLength == 0)
    {
        active = false;
        EV << "LteSchedulerEnb::scheduleGrant - scheduled connection is no more active . Exiting grant " << endl;
        EV << "GnbSchedulerUl::scheduleGrant --------------------::[  END GRANT  ]::--------------------" << endl;
        return totalAllocatedBytes;
    }

    bool stop = false;
    unsigned int toServe = 0;
    for (; cw < numCodewords; ++cw)
    {
        EV << "GnbSchedulerUl::scheduleGrant - @@@@@ CODEWORD " << cw << " @@@@@" << endl;

        queueLength += MAC_HEADER + RLC_HEADER_UM;  // TODO RLC may be either UM or AM
        toServe = queueLength;
        EV << "GnbSchedulerUl::scheduleGrant - bytes to be allocated: " << toServe << endl;

        unsigned int cwAllocatedBytes = 0;  // per codeword allocated bytes
        unsigned int cwAllocatedBlocks = 0; // used by uplink only, for signaling cw blocks usage to schedule list
        unsigned int vQueueItemCounter = 0; // per codeword MAC SDUs counter

        unsigned int allocatedCws = 0;
        unsigned int size = (*bandLim).size();
        for (unsigned int i = 0; i < size; ++i) // for each band
        {
            // save the band and the relative limit
            Band b = (*bandLim).at(i).band_;
            int limit = (*bandLim).at(i).limit_.at(cw);
            EV << "GnbSchedulerUl::scheduleGrant --- BAND " << b << " LIMIT " << limit << "---" << endl;

            // if the limit flag is set to skip, jump off
            if (limit == -2)
            {
                EV << "GnbSchedulerUl::scheduleGrant - skipping logical band according to limit value" << endl;
                continue;
            }

            // search for already allocated codeword
            if (allocatedCws_.find(nodeId) != allocatedCws_.end())
                allocatedCws = allocatedCws_.at(nodeId);

            unsigned int bandAvailableBytes = 0;
            unsigned int bandAvailableBlocks = 0;
            // if there is a previous blocks allocation on the first codeword, blocks allocation is already available
            if (allocatedCws != 0)
            {
                // get band allocated blocks
                int b1 = allocator_->getBlocks(antenna, b, nodeId);
                // limit eventually allocated blocks on other codeword to limit for current cw
                bandAvailableBlocks = (limitBl ? (b1 > limit ? limit : b1) : b1);
                bandAvailableBytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, bandAvailableBlocks, dir,carrierFrequency);
            }
            else // if limit is expressed in blocks, limit value must be passed to availableBytes function
            {
                bandAvailableBlocks = allocator_->availableBlocks(nodeId, antenna, b);
                bandAvailableBytes = (bandAvailableBlocks == 0) ? 0 : availableBytes(nodeId, antenna, b, cw, dir, carrierFrequency, (limitBl) ? limit : -1); // available space (in bytes)
            }

            // if no allocation can be performed, notify to skip the band on next processing (if any)
            if (bandAvailableBytes == 0)
            {
                EV << "GnbSchedulerUl::scheduleGrant - Band " << b << "will be skipped since it has no space left." << endl;
                (*bandLim).at(i).limit_.at(cw) = -2;
                continue;
            }

            //if bandLimit is expressed in bytes
            if (!limitBl)
            {
                // use the provided limit as cap for available bytes, if it is not set to unlimited
                if (limit >= 0 && limit < (int) bandAvailableBytes)
                {
                    bandAvailableBytes = limit;
                    EV << "GnbSchedulerUl::scheduleGrant - Band space limited to " << bandAvailableBytes << " bytes according to limit cap" << endl;
                }
            }
            else
            {
                // if bandLimit is expressed in blocks
                if(limit >= 0 && limit < (int) bandAvailableBlocks)
                {
                    bandAvailableBlocks=limit;
                    EV << "GnbSchedulerUl::scheduleGrant - Band space limited to " << bandAvailableBlocks << " blocks according to limit cap" << endl;
                }
            }

            EV << "GnbSchedulerUl::scheduleGrant - Available Bytes: " << bandAvailableBytes << " available blocks " << bandAvailableBlocks << endl;

            unsigned int uBytes = (bandAvailableBytes > queueLength) ? queueLength : bandAvailableBytes;
            unsigned int uBlocks = rbPerBand_;

            // allocate resources on this band
            if(allocatedCws == 0)
            {
                // mark here allocation
                allocator_->addBlocks(antenna,b,nodeId,uBlocks,uBytes);
                // add allocated blocks for this codeword
                cwAllocatedBlocks += uBlocks;
                totalAllocatedBlocks += uBlocks;
                cwAllocatedBytes += uBytes;
            }

            // update limit
            if(uBlocks>0 && (*bandLim).at(i).limit_.at(cw) > 0)
            {
                (*bandLim).at(i).limit_.at(cw) -= uBlocks;
                if ((*bandLim).at(i).limit_.at(cw) < 0)
                    throw cRuntimeError("Limit decreasing error during booked resources allocation on band %d : new limit %d, due to blocks %d ",
                        b,(*bandLim).at(i).limit_.at(cw),uBlocks);
            }

            // update the counter of bytes to be served
            toServe = (uBytes > toServe) ? 0 : toServe - uBytes;
            if (toServe == 0)
            {
                // all bytes booked, go to allocation
                stop = true;
                active = false;
                break;
            }
            // continue allocating (if there are available bands)
        }// Closes loop on bands

        if (cwAllocatedBytes > 0)
            vQueueItemCounter++;  // increase counter of served SDU

        // === update virtual buffer === //

        unsigned int consumedBytes = (cwAllocatedBytes == 0) ? 0 : cwAllocatedBytes - (MAC_HEADER + RLC_HEADER_UM);  // TODO RLC may be either UM or AM

        // number of bytes to be consumed from the virtual buffer
        while (!conn->isEmpty() && consumedBytes > 0)
        {

            unsigned int vPktSize = conn->front().first;
            if (vPktSize <= consumedBytes)
            {
                // serve the entire vPkt, remove pkt info
                conn->popFront();
                consumedBytes -= vPktSize;
                EV << "GnbSchedulerUl::scheduleGrant - the first SDU/BSR is served entirely, remove it from the virtual buffer, remaining bytes to serve[" << consumedBytes << "]" << endl;
            }
            else
            {
                // serve partial vPkt, update pkt info
                PacketInfo newPktInfo = conn->popFront();
                newPktInfo.first = newPktInfo.first - consumedBytes;
                conn->pushFront(newPktInfo);
                consumedBytes = 0;
                EV << "GnbSchedulerUl::scheduleGrant - the first SDU/BSR is partially served, update its size [" << newPktInfo.first << "]" << endl;
            }
        }

        EV << "GnbSchedulerUl::scheduleGrant - Codeword allocation: " << cwAllocatedBytes << "bytes" << endl;
        if (cwAllocatedBytes > 0)
        {
            // mark codeword as used
            if (allocatedCws_.find(nodeId) != allocatedCws_.end())
                allocatedCws_.at(nodeId)++;
            else
                allocatedCws_[nodeId] = 1;

            totalAllocatedBytes += cwAllocatedBytes;

            // create entry in the schedule list for this carrier
            if (scheduleList_.find(carrierFrequency) == scheduleList_.end())
            {
                LteMacScheduleList newScheduleList;
                scheduleList_[carrierFrequency] = newScheduleList;
            }
            // create entry in the schedule list for this pair <cid,cw>
            std::pair<unsigned int, Codeword> scListId(cid, cw);
            if (scheduleList_[carrierFrequency].find(scListId) == scheduleList_[carrierFrequency].end())
                scheduleList_[carrierFrequency][scListId] = 0;

            // if direction is DL , then schedule list contains number of to-be-trasmitted SDUs ,
            // otherwise it contains number of granted blocks
            scheduleList_[carrierFrequency][scListId] += ((dir == DL) ? vQueueItemCounter : cwAllocatedBlocks);

            EV << "GnbSchedulerUl::scheduleGrant - CODEWORD IS NOW BUSY: GO TO NEXT CODEWORD." << endl;
            if (allocatedCws_.at(nodeId) == MAX_CODEWORDS)
            {
                eligible = false;
                stop = true;
            }
        }
        else
        {
            EV << "GnbSchedulerUl::scheduleGrant - CODEWORD IS FREE: NO ALLOCATION IS POSSIBLE IN NEXT CODEWORD." << endl;
            eligible = false;
            stop = true;
        }
        if (stop)
            break;
    } // Closes loop on Codewords

    EV << "GnbSchedulerUl::scheduleGrant - Total allocation: " << totalAllocatedBytes << " bytes, " << totalAllocatedBlocks << " blocks" << endl;
    EV << "GnbSchedulerUl::scheduleGrant --------------------::[  END GRANT  ]::--------------------" << endl;

    return totalAllocatedBytes;
}

unsigned int GnbSchedulerUl::readAvailableRbs(const MacNodeId id,
    const Remote antenna, const Band b)
{
    return allocator_->availableBlocks(id, antenna, b);
}

unsigned int GnbSchedulerUl::availableBytes(const MacNodeId id,
    Remote antenna, Band b, Codeword cw, Direction dir, double carrierFrequency, int limit)
{
    EV << "GnbSchedulerUl::availableBytes MacNodeId " << id << " Antenna " << dasToA(antenna) << " band " << b << " cw " << cw << endl;
    // Retrieving this user available resource blocks
    int blocks = allocator_->availableBlocks(id,antenna,b);
    //Consistency Check
    if (limit>blocks && limit!=-1)
       throw cRuntimeError("GnbSchedulerUl::availableBytes signaled limit inconsistency with available space band b %d, limit %d, available blocks %d",b,limit,blocks);

    if (limit!=-1)
        blocks=(blocks>limit)?limit:blocks;

    unsigned int bytes = mac_->getAmc()->computeBytesOnNRbs(id, b, cw, blocks, dir,carrierFrequency);
    EV << "GnbSchedulerUl::availableBytes MacNodeId " << id << " blocks [" << blocks << "], bytes [" << bytes << "]" << endl;

    return bytes;
}

