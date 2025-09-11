//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    GnbSchedulerDl.cc / GnbSchedulerDl.h
//
//  Description:
//    This file implements the downlink scheduler for the gNB in the MEC.
//    The scheduler divides the bandwidth in the time dimension.
//    It simulates the MAC stack of the NIC module of gNB
//    LteSchedulerEnb --> LteSchedulerEnbDl
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/nic/mac/scheduler/GnbSchedulerDl.h"
#include "mecrt/nic/mac/scheme/FDSchemeDl.h"

#include "common/LteCommon.h"

#include "stack/mac/allocator/LteAllocationModuleFrequencyReuse.h"
#include "stack/mac/scheduler/LteScheduler.h"
#include "stack/mac/scheduling_modules/LteDrr.h"
#include "stack/mac/scheduling_modules/LteMaxCi.h"
#include "stack/mac/scheduling_modules/LtePf.h"
#include "stack/mac/scheduling_modules/LteMaxCiMultiband.h"
#include "stack/mac/scheduling_modules/LteMaxCiOptMB.h"
#include "stack/mac/scheduling_modules/LteMaxCiComp.h"
#include "stack/mac/scheduling_modules/LteAllocatorBestFit.h"

using namespace omnetpp;


void GnbSchedulerDl::initialize(Direction dir, LteMacEnb* mac)
{
    EV << "GnbSchedulerDl::initialize - initialize the downlink scheduler." << endl;

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
    std::string discipline_name = mac_->par("schedulingDisciplineDl").stdstringValue();
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


SchedDiscipline GnbSchedulerDl::getSchedDiscipline(std::string name)
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
LteScheduler* GnbSchedulerDl::getScheduler(SchedDiscipline discipline, std::string discipline_name)
{
    if ((discipline == UNKNOWN_DISCIPLINE) && (discipline_name == "FDSchemeDl"))
    {
        EV << "Creating gNB downlink scheduler FDSchemeDl" << endl;
        FDSchemeDl* newSchedule = new FDSchemeDl();
        /* add the mac pointer is because the new scheme is not a friend class of LteSchedulerEnb,
         * and we want to avoid changing the original file
         */
        newSchedule->setGnbSchedulerDl(this);

        return newSchedule;
    }
    else
        return getScheduler(discipline);
}


LteScheduler* GnbSchedulerDl::getScheduler(SchedDiscipline discipline)
{
    EV << "Creating LteScheduler " << schedDisciplineToA(discipline) << endl;

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


std::map<double, LteMacScheduleList>* GnbSchedulerDl::schedule()
{
    EV << "GnbSchedulerDl::schedule performed by Node: " << mac_->getMacNodeId() << " (gNB macNodeId)" << endl;


    // clearing structures for new scheduling
    std::map<double, LteMacScheduleList>::iterator lit = scheduleList_.begin();
    for (; lit != scheduleList_.end(); ++lit)
        lit->second.clear();
    allocatedCws_.clear();

    /***
     * clean the allocator
     * the value of resourceBlocks_ is set in GnbMac::initialize() after initializing GnbSchedulerDl.
     */
    allocator_->reset(resourceBlocks_, mac_->getCellInfo()->getNumBands());

    // schedule one carrier at a time
    LteScheduler* scheduler = NULL;
    std::vector<LteScheduler*>::iterator it = scheduler_.begin();
    for ( ; it != scheduler_.end(); ++it)
    {
        scheduler = *it;
        EV << "GnbSchedulerDl::schedule carrier [" << scheduler->getCarrierFrequency() << "]" << endl;

        unsigned int counter = scheduler->decreaseSchedulerPeriodCounter();
        if (counter > 0)
        {
            EV << " GnbSchedulerDl::schedule - not my turn (counter=" << counter << ")"<< endl;
            continue;
        }

        // scheduling of rac requests, retransmissions and transmissions
        EV << "________________________start RAC+RTX _______________________________" << endl;
        /***
         * there is no RAC scheduling for downlink, thus, it always return false.
         * LteScheduler::scheduleRacRequests eventually calls GnbSchedulerDl::racschedule.
         * LteScheduler::scheduleRetransmissions eventually calls GnbSchedulerDl::rtxschedule.
         */
        if ( !(scheduler->scheduleRacRequests()) && !(scheduler->scheduleRetransmissions()) )
        {
            EV << "___________________________end RAC+RTX ________________________________" << endl;
            EV << "___________________________start SCHED ________________________________" << endl;
            scheduler->updateSchedulingInfo();  // only overridden in some of the scheduling modules
            scheduler->schedule();
            EV << "____________________________ end SCHED ________________________________" << endl;
        }
    }

    // record assigned resource blocks statistics
    resourceBlockStatistics();

    return &scheduleList_;
}


bool GnbSchedulerDl::rtxschedule(double carrierFrequency, BandLimitVector* bandLim)
{
    EV << NOW << " GnbSchedulerDl::rtxschedule --------------------::[ START RTX-SCHEDULE ]::--------------------" << endl;
    EV << NOW << " GnbSchedulerDl::rtxschedule Cell:  " << mac_->getMacCellId() << " Direction: " << (direction_ == DL ? "DL" : "UL") << endl;

    // retrieving reference to HARQ entities
    HarqTxBuffers* harqQueues = mac_->getHarqTxBuffers(carrierFrequency);
    if (harqQueues != NULL)
    {
        HarqTxBuffers::iterator it = harqQueues->begin();
        HarqTxBuffers::iterator et = harqQueues->end();

        std::vector<BandLimit> usableBands;

        // examination of HARQ process in rtx status, adding them to scheduling list
        for(; it != et; ++it)
        {
            // For each UE
            MacNodeId nodeId = it->first;

            OmnetId id = binder_->getOmnetId(nodeId);
            if(id == 0){
                // UE has left the simulation, erase HARQ-queue
                it = harqQueues->erase(it);
                if(it == et)
                    break;
                else
                    continue;
            }
            LteHarqBufferTx* currHarq = it->second;
            std::vector<LteHarqProcessTx *> * processes = currHarq->getHarqProcesses();

            // Get user transmission parameters
            const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, direction_, carrierFrequency);// get the user info
            // TODO SK Get the number of codewords - FIX with correct mapping
            unsigned int codewords = txParams.getLayers().size();// get the number of available codewords

            EV << NOW << " GnbSchedulerDl::rtxschedule  UE: " << nodeId << endl;
            // get the number of HARQ processes
            unsigned int process=0;
            unsigned int maxProcesses = currHarq->getNumProcesses();
            for(process = 0; process < maxProcesses; ++process )
            {
                // for each HARQ process
                LteHarqProcessTx * currProc = (*processes)[process];

                if (allocatedCws_[nodeId] == codewords)
                    break;
                for (Codeword cw = 0; cw < codewords; ++cw)
                {

                    if (allocatedCws_[nodeId]==codewords)
                        break;
                    EV << NOW << " GnbSchedulerDl::rtxschedule process " << process << endl;

                    // skip processes which are not in rtx status
                    if (currProc->getUnitStatus(cw) != TXHARQ_PDU_BUFFERED)
                    {
                        EV << NOW << " GnbSchedulerDl::rtxschedule detected Acid: " << process << " in status " << currProc->getUnitStatus(cw) << endl;
                        continue;
                    }

                    EV << NOW << " GnbSchedulerDl::rtxschedule detected RTX Acid: " << process << endl;

                    // Get the bandLimit for the current user
                    bool ret = getBandLimit(&usableBands, nodeId);
                    if (ret)
                        bandLim = &usableBands;  // TODO fix this, must be combined with the bandlimit of the carrier
                    else
                        bandLim = nullptr;

                    // perform the retransmission
                    unsigned int bytes = schedulePerAcidRtx(nodeId,carrierFrequency,cw,process,bandLim);

                    // if a value different from zero is returned, there was a service
                    if(bytes > 0)
                    {
                        EV << NOW << " GnbSchedulerDl::rtxschedule CODEWORD IS NOW BUSY!!!" << endl;

                        check_and_cast<LteMacEnb*>(mac_)->signalProcessForRtx(nodeId, carrierFrequency, DL, false);

                        // do not process this HARQ process anymore
                        // go to next codeword
                        break;
                    }
                }
            }
        }
    }

    unsigned int availableBlocks = allocator_->computeTotalRbs();
    EV << " GnbSchedulerDl::rtxschedule OFDM Space: " << availableBlocks << endl;
    EV << "    GnbSchedulerDl::rtxschedule --------------------::[  END RTX-SCHEDULE  ]::-------------------- " << endl;

    return (availableBlocks == 0);
}


unsigned int GnbSchedulerDl::schedulePerAcidRtx(MacNodeId nodeId, double carrierFrequency, Codeword cw, unsigned char acid,
    std::vector<BandLimit>* bandLim, Remote antenna, bool limitBl)
{
    // Get user transmission parameters
    const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, direction_,carrierFrequency);    // get the user info
    const std::set<Band>& allowedBands = txParams.readBands();
    // TODO SK Get the number of codewords - FIX with correct mapping
    unsigned int codewords = txParams.getLayers().size();                // get the number of available codewords

    std::vector<BandLimit> tempBandLim;

    Codeword remappedCw = (codewords == 1) ? 0 : cw;

    if (bandLim == nullptr)
    {
        // Create a vector of band limit using all bands
        bandLim = &tempBandLim;
        bandLim->clear();

        unsigned int numBands = mac_->getCellInfo()->getNumBands();
        // for each band of the band vector provided
        for (unsigned int i = 0; i < numBands; i++)
        {
            BandLimit elem;
            // copy the band
            elem.band_ = Band(i);
            EV << "Putting band " << i << endl;
            for (unsigned int j = 0; j < codewords; j++)
            {
                if( allowedBands.find(elem.band_)!= allowedBands.end() )
                {
//                    EV << "\t" << i << " " << "yes" << endl;
                    elem.limit_[j]=-1;
                }
                else
                {
//                    EV << "\t" << i << " " << "no" << endl;
                    elem.limit_[j]=-2;
                }
            }
            bandLim->push_back(elem);
        }
    }
    else
    {
        unsigned int numBands = mac_->getCellInfo()->getNumBands();
        // for each band of the band vector provided
        for (unsigned int i = 0; i < numBands; i++)
        {
            BandLimit& elem = bandLim->at(i);
            for (unsigned int j = 0; j < codewords; j++)
            {
                if (elem.limit_[j] == -2)
                    continue;

                if (allowedBands.find(elem.band_)!= allowedBands.end() )
                {
//                    EV << "\t" << i << " " << "yes" << endl;
                    elem.limit_[j]=-1;
                }
                else
                {
//                    EV << "\t" << i << " " << "no" << endl;
                    elem.limit_[j]=-2;
                }
            }
        }
    }

    EV << NOW << " GnbSchedulerDl::schedulePerAcidRtx - Node [" << mac_->getMacNodeId() << "], User[" << nodeId << "],  Codeword [" << cw << "]  of [" << codewords << "] , ACID [" << (int)acid << "] " << endl;
    //! \test REALISTIC!!!  Multi User MIMO support
    if (mac_->muMimo() && (txParams.readTxMode() == MULTI_USER))
    {
        // request amc for MU_MIMO pairing
        MacNodeId peer = mac_->getAmc()->computeMuMimoPairing(nodeId);
        if (peer != nodeId)
        {
            // this user has a valid pairing
            //1) register pairing  - if pairing is already registered false is returned
            if (allocator_->configureMuMimoPeering(nodeId, peer))
            {
                EV << "GnbSchedulerDl::schedulePerAcidRtx - MU-MIMO pairing established: main user [" << nodeId << "], paired user [" << peer << "]" << endl;
            }
            else
            {
                EV << "GnbSchedulerDl::schedulePerAcidRtx - MU-MIMO pairing already exists between users [" << nodeId << "] and [" << peer << "]" << endl;
            }
        }
        else
        {
            EV << "GnbSchedulerDl::schedulePerAcidRtx - no MU-MIMO pairing available for user [" << nodeId << "]" << endl;
        }
    }
                //!\test experimental DAS support
                // registering DAS spaces to the allocator
    Plane plane = allocator_->getOFDMPlane(nodeId);
    allocator_->setRemoteAntenna(plane, antenna);

    // blocks to allocate for each band
    std::vector<unsigned int> assignedBlocks;
    // bytes which blocks from the preceding vector are supposed to satisfy
    std::vector<unsigned int> assignedBytes;

    HarqTxBuffers* harqTxBuff = mac_->getHarqTxBuffers(carrierFrequency);
    if (harqTxBuff == NULL)
        throw cRuntimeError("GnbSchedulerDl::schedulePerAcidRtx - HARQ Buffer not found for carrier %f", carrierFrequency);
    LteHarqBufferTx* currHarq = harqTxBuff->at(nodeId);

    // bytes to serve
    unsigned int bytes = currHarq->pduLength(acid, cw);

    // check selected process status.
    std::vector<UnitStatus> pStatus = currHarq->getProcess(acid)->getProcessStatus();

    Codeword allocatedCw = 0;
    // search for already allocated codeword
    // std::vector<UnitStatus>::iterator vit = pStatus.begin(), vet = pStatus.end();
    // for (;vit!=vet;++vit)
    // {
    //     // skip current codeword
    //     if (vit->first==cw) continue;
    //
    //     if (vit->second == TXHARQ_PDU_SELECTED)
    //     {
    //         allocatedCw=vit->first;
    //         break;
    //     }
    // }
    if (allocatedCws_.find(nodeId) != allocatedCws_.end())
    {
        allocatedCw = allocatedCws_.at(nodeId);
    }
    // for each band
    unsigned int size = bandLim->size();

    for (unsigned int i = 0; i < size; ++i)
    {
        // save the band and the relative limit
        Band b = bandLim->at(i).band_;
        int limit = bandLim->at(i).limit_.at(remappedCw);

        EV << "GnbSchedulerDl::schedulePerAcidRtx --- BAND " << b << " LIMIT " << limit << "---" << endl;
        // if the limit flag is set to skip, jump off
        if (limit == -2)
        {
            EV << "GnbSchedulerDl::schedulePerAcidRtx - skipping logical band according to limit value" << endl;
            continue;
        }

        unsigned int available = 0;
        // if a codeword has been already scheduled for retransmission, limit available blocks to what's been  allocated on that codeword
        if ((allocatedCw != 0))
        {
            // get band allocated blocks
            int b1 = allocator_->getBlocks(antenna, b, nodeId);

            // limit eventually allocated blocks on other codeword to limit for current cw
            //b1 = (limitBl ? (b1>limit?limit:b1) : b1);
            available = (b1 == 0) ? 0 :  mac_->getAmc()->computeBytesOnNRbs(nodeId, b, remappedCw, b1, direction_,carrierFrequency);
        }
        else
            available = availableBytes(nodeId, antenna, b, remappedCw, direction_, carrierFrequency, (limitBl) ? limit : -1);    // available space

        // use the provided limit as cap for available bytes, if it is not set to unlimited
        if (limit >= 0 && !limitBl)
            available = limit < (int) available ? limit : available;

        EV << NOW << " GnbSchedulerDl::schedulePerAcidRtx ----- BAND " << b << "-----" << endl;
        EV << NOW << " GnbSchedulerDl::schedulePerAcidRtx - To serve: " << bytes << " bytes" << endl;
        EV << NOW << " GnbSchedulerDl::schedulePerAcidRtx - Available: " << available << " bytes" << endl;

        unsigned int allocation = 0;
        if (available < bytes)
        {
            allocation = available;
            bytes -= available;
        }
        else
        {
            allocation = bytes;
            bytes = 0;
        }

        if (allocatedCw == 0)
        {
            unsigned int blocks = rbPerBand_;

            EV << NOW << " GnbSchedulerDl::schedulePerAcidRtx - Assigned blocks: " << blocks << endl;

            // assign only on the first codeword
            assignedBlocks.push_back(blocks);
            assignedBytes.push_back(allocation);
        }

        if (bytes == 0)
            break;
    }

    if (bytes > 0)
    {
        // process couldn't be served
        EV << NOW << " GnbSchedulerDl::schedulePerAcidRtx - Cannot serve HARQ Process" << acid << endl;
        return 0;
    }

        // record the allocation if performed
    size = assignedBlocks.size();
    // For each LB with assigned blocks
    for (unsigned int i = 0; i < size; ++i)
    {
        if (allocatedCw == 0)
        {
            // allocate the blocks
            allocator_->addBlocks(antenna, bandLim->at(i).band_, nodeId, assignedBlocks.at(i), assignedBytes.at(i));
        }
        // store the amount
        bandLim->at(i).limit_.at(remappedCw) = assignedBytes.at(i);
    }

    UnitList signal;
    signal.first = acid;
    signal.second.push_back(cw);

    EV << NOW << " GnbSchedulerDl::schedulePerAcidRtx - HARQ Process " << (int)acid << "  codeword  " << cw << " marking for retransmission " << endl;

    // if allocated codewords is not MAX_CODEWORDS, then there's another allocated codeword , update the codewords variable :

    if (allocatedCw != 0)
    {
        // TODO fixme this only works if MAX_CODEWORDS ==2
        --codewords;
        if (codewords <= 0)
            throw cRuntimeError("GnbSchedulerDl::schedulePerAcidRtx(): erroneus codeword count %d", codewords);
    }

    // signal a retransmission
    currHarq->markSelected(signal, codewords);

    // mark codeword as used
    if (allocatedCws_.find(nodeId) != allocatedCws_.end())
    {
        allocatedCws_.at(nodeId)++;
        }
    else
    {
        allocatedCws_[nodeId] = 1;
    }

    bytes = currHarq->pduLength(acid, cw);

    EV << NOW << " GnbSchedulerDl::schedulePerAcidRtx - HARQ Process " << (int)acid << "  codeword  " << cw << ", " << bytes << " bytes served!" << endl;

    return bytes;
}

unsigned int GnbSchedulerDl::scheduleBgRtx(MacNodeId bgUeId, double carrierFrequency, Codeword cw, std::vector<BandLimit>* bandLim, Remote antenna, bool limitBl)
{
    try
    {
        BackgroundTrafficManager* bgTrafficManager = mac_->getBackgroundTrafficManager(carrierFrequency);
        unsigned int bytesPerBlock = bgTrafficManager->getBackloggedUeBytesPerBlock(bgUeId, direction_);

        // get the RTX buffer size
        unsigned int queueLength = bgTrafficManager->getBackloggedUeBuffer(bgUeId, direction_, true); // in bytes
        if (queueLength == 0)
            return 0;

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
                    elem.limit_[j]=-2;
                }
                tempBandLim.push_back(elem);
            }
            bandLim = &tempBandLim;
        }

        EV << NOW << "GnbSchedulerDl::scheduleBgRtx - Node[" << mac_->getMacNodeId() << ", User[" << bgUeId << "]" << endl;

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

            EV << NOW << " GnbSchedulerDl::scheduleBgRtx BAND " << b << endl;
            EV << NOW << " GnbSchedulerDl::scheduleBgRtx total bytes:" << queueLength << " still to serve: " << toServe << " bytes" << endl;
            EV << NOW << " GnbSchedulerDl::scheduleBgRtx Available: " << bandAvailableBytes << " bytes" << endl;

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
            EV << NOW << " GnbSchedulerDl::scheduleBgRtx Unavailable space for serving node " << bgUeId << endl;
            return 0;
        }
        else
        {

            // record the allocation
            unsigned int size = assignedBlocks.size();
            // unsigned int cwAllocatedBlocks =0;
            unsigned int allocatedBytes = 0;
            for(unsigned int i = 0; i < size; ++i)
            {
                // For each LB for which blocks have been allocated
                Band b = bandLim->at(i).band_;

                allocatedBytes += assignedBytes.at(i);
                // cwAllocatedBlocks +=assignedBlocks.at(i);
                EV << "\t Cw->" << allocatedCw << "/" << MAX_CODEWORDS << endl;
                //! handle multi-codeword allocation
                if (allocatedCw!=MAX_CODEWORDS)
                {
                    EV << NOW << " GnbSchedulerDl::scheduleBgRtx - adding " << assignedBlocks.at(i) << " to band " << i << endl;
                    allocator_->addBlocks(antenna,b,bgUeId,assignedBlocks.at(i),assignedBytes.at(i));
                }
            }

            // signal a retransmission

            // mark codeword as used
            if (allocatedCws_.find(bgUeId)!=allocatedCws_.end())
                allocatedCws_.at(bgUeId)++;
            else
                allocatedCws_[bgUeId]=1;

            EV << NOW << " GnbSchedulerDl::scheduleBgRtx: " << allocatedBytes << " bytes served! " << endl;

            return allocatedBytes;
        }
    }
    catch(std::exception& e)
    {
        throw cRuntimeError("Exception in LteSchedulerEnbUl::rtxAcid(): %s", e.what());
    }
    return 0;
}

unsigned int GnbSchedulerDl::availableBytesBackgroundUe(const MacNodeId id, Remote antenna, Band b, Direction dir, double carrierFrequency, int limit)
{
    EV << "GnbSchedulerDl::availableBytes MacNodeId " << id << " Antenna " << dasToA(antenna) << " band " << b << endl;
    // Retrieving this user available resource blocks
    int blocks = allocator_->availableBlocks(id,antenna,b);
    if (blocks == 0)
    {
        EV << "GnbSchedulerDl::availableBytes - No blocks available on band " << b << endl;
        return 0;
    }

    //Consistency Check
    if (limit>blocks && limit!=-1)
       throw cRuntimeError("GnbSchedulerDl::availableBytes signaled limit inconsistency with available space band b %d, limit %d, available blocks %d",b,limit,blocks);

    if (limit!=-1)
        blocks=(blocks>limit)?limit:blocks;

    unsigned int bytesPerBlock = mac_->getBackgroundTrafficManager(carrierFrequency)->getBackloggedUeBytesPerBlock(id, dir);
    unsigned int bytes = bytesPerBlock * blocks;
    EV << "GnbSchedulerDl::availableBytes MacNodeId " << id << " blocks [" << blocks << "], bytes [" << bytes << "]" << endl;

    return bytes;
}

void GnbSchedulerDl::backlog(MacCid cid)
{
    EV << "GnbSchedulerDl::backlog - backlogged data for Logical Cid " << cid << endl;
    if(cid == 1)
        return;

    EV << NOW << "GnbSchedulerDl::backlog CID notified " << cid << endl;
    activeConnectionSet_.insert(cid);

    std::vector<LteScheduler*>::iterator it = scheduler_.begin();
    for ( ; it != scheduler_.end(); ++it)
        (*it)->notifyActiveConnection(cid);
}


/*  COMPLETE:        scheduleGrant(cid,bytes,terminate,active,eligible,band_limit,antenna);
 *  ANTENNA UNAWARE: scheduleGrant(cid,bytes,terminate,active,eligible,band_limit);
 *  BAND UNAWARE:    scheduleGrant(cid,bytes,terminate,active,eligible);
 */
unsigned int GnbSchedulerDl::scheduleGrant(MacCid cid, unsigned int bytes, bool& terminate, bool& active, bool& eligible, double carrierFrequency, BandLimitVector* bandLim, Remote antenna, bool limitBl)
{
    // Get the node ID and logical connection ID
    MacNodeId nodeId = MacCidToNodeId(cid);
    LogicalCid flowId = MacCidToLcid(cid);

    Direction dir = direction_; // DL

    // Get user transmission parameters
    const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, dir,carrierFrequency);
    const std::set<Band>& allowedBands = txParams.readBands();

    /***
     * Get the number of codewords.
     * the number of layers for each codeword is only greater than 1 (which is 2) when
     * the TxMode = OL_SPATIAL_MULTIPLEXING or CL_SPATIAL_MULTIPLEXING
     * and the Rank >= 2
     */
    unsigned int numCodewords = txParams.getLayers().size();

    // TEST: check the number of codewords
    numCodewords = 1;

    EV << "GnbSchedulerDl::scheduleGrant - deciding allowed Bands" << endl;
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
    EV << "GnbSchedulerDl::scheduleGrant(" << cid << "," << bytes << "," << terminate << "," << active << "," << eligible << "," << bands_msg << "," << dasToA(antenna) << ")" << endl;

    unsigned int totalAllocatedBytes = 0;  // total allocated data (in bytes)
    unsigned int totalAllocatedBlocks = 0; // total allocated data (in blocks)

    // === Perform normal operation for grant === //

    EV << "GnbSchedulerDl::scheduleGrant --------------------::[ START GRANT ]::--------------------" << endl;
    EV << "GnbSchedulerDl::scheduleGrant Cell: " << mac_->getMacCellId() << endl;
    EV << "GnbSchedulerDl::scheduleGrant CID: " << cid << "(UE: " << nodeId << ", Flow: " << flowId << ") current Antenna [" << dasToA(antenna) << "]" << endl;

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
                EV << "GnbSchedulerDl::scheduleGrant MU-MIMO pairing established: main user [" << nodeId << "], paired user [" << peer << "]" << endl;
            else
                EV << "GnbSchedulerDl::scheduleGrant MU-MIMO pairing already exists between users [" << nodeId << "] and [" << peer << "]" << endl;
        }
        else
        {
            EV << "GnbSchedulerDl::scheduleGrant no MU-MIMO pairing available for user [" << nodeId << "]" << endl;
        }
    }

    // registering DAS spaces to the allocator
    Plane plane = allocator_->getOFDMPlane(nodeId);
    allocator_->setRemoteAntenna(plane, antenna);

    /***
     * search for already allocated codeword
     * allocatedCws_ (std::map<MacNodeId, unsigned int>) is reset for every call to GnbScheduler:: schedule()
     */
    unsigned int cwAlredyAllocated = 0;
    if (allocatedCws_.find(nodeId) != allocatedCws_.end())
        cwAlredyAllocated = allocatedCws_.at(nodeId);


    // Check OFDM space
    // OFDM space is not zero if this if we are trying to allocate the second cw in SPMUX or
    // if we are trying to allocate a peer user in mu_mimo plane
    if (allocator_->computeTotalRbs() == 0 && (((txParams.readTxMode() != OL_SPATIAL_MULTIPLEXING &&
        txParams.readTxMode() != CL_SPATIAL_MULTIPLEXING) || cwAlredyAllocated == 0) &&
        (txParams.readTxMode() != MULTI_USER || plane != MU_MIMO_PLANE)))
    {
        terminate = true; // ODFM space ended, issuing terminate flag
        EV << "GnbSchedulerDl::scheduleGrant Space ended, no schedulation." << endl;
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
            EV << "GnbSchedulerDl::scheduleGrant blocks: " << bytes << endl;
        else
            EV << "GnbSchedulerDl::scheduleGrant Bytes: " << bytes << endl;
        EV << "GnbSchedulerDl::scheduleGrant Bands: {";
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

    EV << "GnbSchedulerDl::scheduleGrant TxMode: " << txModeToA(txParams.readTxMode()) << endl;
    EV << "GnbSchedulerDl::scheduleGrant Available codewords: " << numCodewords << endl;

    // Retrieve the first free codeword checking the eligibility - check eligibility could modify current cw index.
    Codeword cw = 0; // current codeword, modified by reference by the checkeligibility function
    if (!checkEligibility(nodeId, cw, carrierFrequency) || cw >= numCodewords)
    {
        eligible = false;

        EV << "GnbSchedulerDl::scheduleGrant @@@@@ CODEWORD " << cw << " @@@@@" << endl;
        EV << "GnbSchedulerDl::scheduleGrant Total allocation: " << totalAllocatedBytes << "bytes" << endl;
        EV << "GnbSchedulerDl::scheduleGrant NOT ELIGIBLE!!!" << endl;
        EV << "GnbSchedulerDl::scheduleGrant --------------------::[  END GRANT  ]::--------------------" << endl;
        return totalAllocatedBytes; // return the total number of served bytes
    }

    // Get virtual buffer reference
    LteMacBuffer* conn = ((dir == DL) ? vbuf_->at(cid) : bsrbuf_->at(cid));

    // get the buffer size
    unsigned int queueLength = conn->getQueueOccupancy(); // in bytes
    if (queueLength == 0)
    {
        active = false;
        EV << "GnbSchedulerDl::scheduleGrant - scheduled connection is no more active . Exiting grant " << endl;
        EV << "GnbSchedulerDl::scheduleGrant --------------------::[  END GRANT  ]::--------------------" << endl;
        return totalAllocatedBytes;
    }

    bool stop = false;
    unsigned int toServe = 0;
    for (; cw < numCodewords; ++cw)
    {
        EV << "GnbSchedulerDl::scheduleGrant @@@@@ CODEWORD " << cw << " @@@@@" << endl;

        queueLength += MAC_HEADER + RLC_HEADER_UM;  // TODO RLC may be either UM or AM
        toServe = queueLength;
        EV << "GnbSchedulerDl::scheduleGrant bytes to be allocated: " << toServe << endl;

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
            EV << "GnbSchedulerDl::scheduleGrant --- BAND " << b << " LIMIT " << limit << "---" << endl;

            // if the limit flag is set to skip, jump off
            if (limit == -2)
            {
                EV << "GnbSchedulerDl::scheduleGrant skipping logical band according to limit value" << endl;
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
                EV << "GnbSchedulerDl::scheduleGrant Band " << b << "will be skipped since it has no space left." << endl;
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
                    EV << "GnbSchedulerDl::scheduleGrant Band space limited to " << bandAvailableBytes << " bytes according to limit cap" << endl;
                }
            }
            else
            {
                // if bandLimit is expressed in blocks
                if(limit >= 0 && limit < (int) bandAvailableBlocks)
                {
                    bandAvailableBlocks=limit;
                    EV << "GnbSchedulerDl::scheduleGrant Band space limited to " << bandAvailableBlocks << " blocks according to limit cap" << endl;
                }
            }

            EV << "GnbSchedulerDl::scheduleGrant Available Bytes: " << bandAvailableBytes << " available blocks " << bandAvailableBlocks << endl;

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
                EV << "GnbSchedulerDl::scheduleGrant - the first SDU/BSR is served entirely, remove it from the virtual buffer, remaining bytes to serve[" << consumedBytes << "]" << endl;
            }
            else
            {
                // serve partial vPkt, update pkt info
                PacketInfo newPktInfo = conn->popFront();
                newPktInfo.first = newPktInfo.first - consumedBytes;
                conn->pushFront(newPktInfo);
                consumedBytes = 0;
                EV << "GnbSchedulerDl::scheduleGrant - the first SDU/BSR is partially served, update its size [" << newPktInfo.first << "]" << endl;
            }
        }

        EV << "GnbSchedulerDl::scheduleGrant Codeword allocation: " << cwAllocatedBytes << "bytes" << endl;
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

            EV << "GnbSchedulerDl::scheduleGrant CODEWORD IS NOW BUSY: GO TO NEXT CODEWORD." << endl;
            if (allocatedCws_.at(nodeId) == MAX_CODEWORDS)
            {
                eligible = false;
                stop = true;
            }
        }
        else
        {
            EV << "GnbSchedulerDl::scheduleGrant CODEWORD IS FREE: NO ALLOCATION IS POSSIBLE IN NEXT CODEWORD." << endl;
            eligible = false;
            stop = true;
        }
        if (stop)
            break;
    } // Closes loop on Codewords

    EV << "GnbSchedulerDl::scheduleGrant Total allocation: " << totalAllocatedBytes << " bytes, " << totalAllocatedBlocks << " blocks" << endl;
    EV << "GnbSchedulerDl::scheduleGrant --------------------::[  END GRANT  ]::--------------------" << endl;

    return totalAllocatedBytes;
}

unsigned int GnbSchedulerDl::readAvailableRbs(const MacNodeId id,
    const Remote antenna, const Band b)
{
    return allocator_->availableBlocks(id, antenna, b);
}

unsigned int GnbSchedulerDl::availableBytes(const MacNodeId id,
    Remote antenna, Band b, Codeword cw, Direction dir, double carrierFrequency, int limit)
{
    EV << "GnbSchedulerDl::availableBytes MacNodeId " << id << " Antenna " << dasToA(antenna) << " band " << b << " cw " << cw << endl;
    // Retrieving this user available resource blocks
    int blocks = allocator_->availableBlocks(id,antenna,b);
    //Consistency Check
    if (limit>blocks && limit!=-1)
       throw cRuntimeError("GnbSchedulerDl::availableBytes signaled limit inconsistency with available space band b %d, limit %d, available blocks %d",b,limit,blocks);

    if (limit!=-1)
        blocks=(blocks>limit)?limit:blocks;

    unsigned int bytes = mac_->getAmc()->computeBytesOnNRbs(id, b, cw, blocks, dir,carrierFrequency);
    EV << "GnbSchedulerDl::availableBytes MacNodeId " << id << " blocks [" << blocks << "], bytes [" << bytes << "]" << endl;

    return bytes;
}



