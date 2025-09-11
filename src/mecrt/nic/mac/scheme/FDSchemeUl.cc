//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    FDSchemeUl.cc / FDSchemeUl.h
//
//  Description:
//    This file implements the uplink scheduling scheme for the gNB in the MEC.
//    This scheme prioritizes the UE with the best CQI in each time slot.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/nic/mac/scheme/FDSchemeUl.h"
#include "mecrt/nic/mac/scheduler/GnbSchedulerUl.h"

#include "stack/backgroundTrafficGenerator/BackgroundTrafficManager.h"

using namespace omnetpp;

void FDSchemeUl::prepareSchedule()
{
    EV << NOW << " FDSchemeUl::prepareSchedule - uplink scheduling for node " << mac_->getMacNodeId() << " (macNodeId)" << endl;

    if (binder_ == nullptr)
        binder_ = getBinder();

    activeConnectionTempSet_ = *activeConnectionSet_;

    // Build the score list by cycling through the active connections.
    ScoreList score;
    MacCid cid =0;
    unsigned int blocks =0;
    unsigned int byPs = 0;

    /***
     * Temporary enumeration to assign available bands list to UEs
     */
    unsigned int totalBands = mac_->getCellInfo()->getNumBands();
    //unsigned int totalBands = 3;
    std::set<Band> allBands;
    for (int i=0; i < totalBands; ++i)
        allBands.insert(i);

    for ( ActiveSet::iterator it1 = carrierActiveConnectionSet_.begin ();it1 != carrierActiveConnectionSet_.end (); )
    {
        // Current connection.
        cid = *it1;
        ++it1;
        MacNodeId nodeId = MacCidToNodeId(cid);     // destination UE macNodeId

        /***
         * TODO: the available bands should be set with the scheduling algorithms
         */
        check_and_cast<GnbMac*>(mac_)->setAllowedBandsUeUl(nodeId, allBands);
    }


    for ( ActiveSet::iterator it1 = carrierActiveConnectionSet_.begin ();it1 != carrierActiveConnectionSet_.end (); )
    {
        // Current connection.
        cid = *it1;

        ++it1;

        MacNodeId nodeId = MacCidToNodeId(cid);
        OmnetId id = binder_->getOmnetId(nodeId);
        if(nodeId == 0 || id == 0){
                // node has left the simulation - erase corresponding CIDs
                activeConnectionSet_->erase(cid);
                activeConnectionTempSet_.erase(cid);
                carrierActiveConnectionSet_.erase(cid);
                continue;
        }

        // if we are allocating the UL subframe, this connection may be either UL or D2D
        Direction dir;
        dir = (MacCidToLcid(cid) == D2D_SHORT_BSR) ? D2D : (MacCidToLcid(cid) == D2D_MULTI_SHORT_BSR) ? D2D_MULTI : direction_;

        // compute available blocks for the current user
        const UserTxParams& infoTemp = mac_->getAmc()->computeTxParams(nodeId,dir,carrierFrequency_);

        /**
         * We can adjust the usable band for the ue here (define a map).
         * In this case, we should use the MIN_CQI pilot mode (set in GnbMac.ned, initialized in GnbMac::initialization)
         * when design the scheduling algorithm to ensure all bands are accessible by UEs.
         * ***** otherwise, it is a new scheduling problem *******
         */
        UserTxParams * ueTxParams = infoTemp.dup();

        std::set<Band> bandsForUeUl;
        const std::set<Band>& allowedBands = infoTemp.readBands();  // set of bands satisfies CQI requirement
        std::set<Band>& allowedB = check_and_cast<GnbMac*>(mac_)->getAllowedBandsUeUl(nodeId);    // bands allocated

        for (std::set<Band>::iterator itBand = allowedB.begin(); itBand != allowedB.end(); ++itBand) {
            auto itB = allowedBands.find(*itBand);
            if (itB != allowedBands.end())
            {
                bandsForUeUl.insert(*itB);
                EV << "FDSchemeUl::prepareSchedule - adding usable band " << *itB   << " for UE " << nodeId << endl;
            }
        }

        ueTxParams->writeBands(bandsForUeUl);
        const UserTxParams& info = mac_->getAmc()->setTxParams(nodeId, direction_, (*ueTxParams), carrierFrequency_);

        /***
         * the number of layers for each codeword is only greater than 1 (which is 2) when
         * the TxMode = OL_SPATIAL_MULTIPLEXING or CL_SPATIAL_MULTIPLEXING
         * and the Rank >= 2
         */
        unsigned int codeword=info.getLayers().size();
        //no more free cw
        if (eNbScheduler_->allocatedCws(nodeId)==codeword)
            continue;

        bool cqiNull=false;
        for (unsigned int i=0;i<codeword;i++)
        {
            if (info.readCqiVector()[i]==0)
            cqiNull=true;
        }
        if (cqiNull)
            continue;

        const std::set<Band>& bands = info.readBands();
        std::set<Band>::const_iterator it = bands.begin(),et=bands.end();
        std::set<Remote>::iterator antennaIt = info.readAntennaSet().begin(), antennaEt=info.readAntennaSet().end();

        // compute score based on total available bytes
        unsigned int availableBlocks=0;
        unsigned int availableBytes =0;
        // for each antenna
        for (;antennaIt!=antennaEt;++antennaIt)
        {
            // for each logical band
            for (;it!=et;++it)
            {
                unsigned int bandBlocks = eNbScheduler_->readAvailableRbs(nodeId,*antennaIt,*it);
                availableBlocks += bandBlocks;
                availableBytes += mac_->getAmc()->computeBytesOnNRbs(nodeId,*it, bandBlocks, dir,carrierFrequency_);
            }
        }

        blocks = availableBlocks;
        // current user bytes per slot
        byPs = (blocks>0) ? (availableBytes/blocks ) : 0;

        // Create a new score descriptor for the connection, where the score is equal to the ratio between bytes per slot and long term rate
        ScoreDesc desc(cid,byPs);
        // insert the cid score
        score.push (desc);

        EV << NOW << " FDSchemeUl::prepareSchedule computed for cid " << cid << " score of " << desc.score_ << endl;
    }

    if (direction_ == UL || direction_ == DL)  // D2D background traffic not supported (yet?)
    {
        // query the BgTrafficManager to get the list of backlogged bg UEs to be added to the scorelist. This work
        // is done by this module itself, so that backgroundTrafficManager is transparent to the scheduling policy in use

        BackgroundTrafficManager* bgTrafficManager = mac_->getBackgroundTrafficManager(carrierFrequency_);
        std::list<int>::const_iterator it = bgTrafficManager->getBackloggedUesBegin(direction_),
                                         et = bgTrafficManager->getBackloggedUesEnd(direction_);

        int bgUeIndex;
        int bytesPerBlock;
        MacNodeId bgUeId;
        MacCid bgCid;
        for (; it != et; ++it)
        {
            bgUeIndex = *it;
            bgUeId = BGUE_MIN_ID + bgUeIndex;

            // the cid for a background UE is a 32bit integer composed as:
            // - the most significant 16 bits are set to the background UE id (BGUE_MIN_ID+index)
            // - the least significant 16 bits are set to 0 (lcid=0)
            bgCid = bgUeId << 16;

            bytesPerBlock = bgTrafficManager->getBackloggedUeBytesPerBlock(bgUeId, direction_);

            ScoreDesc bgDesc(bgCid, bytesPerBlock);
            score.push(bgDesc);
        }
    }

    // Schedule the connections in score order.
    while ( ! score.empty () )
    {
        // Pop the top connection from the list.
        ScoreDesc current = score.top ();

        bool terminate = false;
        bool active = true;
        bool eligible = true;
        unsigned int granted;

        if ( MacCidToNodeId(current.x_) >= BGUE_MIN_ID)
        {
            EV << NOW << " FDSchemeUl::prepareSchedule - scheduling background UE " << MacCidToNodeId(current.x_) << " with score of " << current.score_ << endl;

            // Grant data to that background connection.
            granted = requestGrantBackground (current.x_, 4294967295U, terminate, active, eligible);

            EV << NOW << " FDSchemeUl::prepareSchedule - granted " << granted << " bytes to background UE " << MacCidToNodeId(current.x_) << endl;
        }
        else
        {
            EV << NOW << " FDSchemeUl::prepareSchedule - scheduling connection " << current.x_ << " with score of " << current.score_ << endl;

            // Grant data to that connection.
            granted = requestGrant (current.x_, 4294967295U, terminate, active, eligible);

            EV << NOW << " FDSchemeUl::prepareSchedule - granted " << granted << " bytes to connection " << current.x_ << endl;
        }

        // Exit immediately if the terminate flag is set.
        if ( terminate ) break;

        // Pop the descriptor from the score list if the active or eligible flag are clear.
        if ( ! active || ! eligible )
        {
            score.pop ();
            EV << NOW << " FDSchemeUl::prepareSchedule - connection " << current.x_ << " was found ineligible" << endl;
        }

        // Set the connection as inactive if indicated by the grant ().
        if ( ! active )
        {
            EV << NOW << " FDSchemeUl::prepareSchedule - scheduling connection " << current.x_ << " set to inactive " << endl;

            if ( MacCidToNodeId(current.x_) <= BGUE_MIN_ID)
            {
                carrierActiveConnectionSet_.erase(current.x_);
                activeConnectionTempSet_.erase (current.x_);
            }
        }
    }
}

void FDSchemeUl::commitSchedule()
{
    *activeConnectionSet_ = activeConnectionTempSet_;
}

bool FDSchemeUl::scheduleRacRequests()
{
    // reset the band limit vector used for rac
    // TODO do this only when it was actually used in previous slot
    for (unsigned int i = 0; i < bandLimit_->size(); i++)
    {
        // copy the element
        slotRacBandLimit_[i].band_ = bandLimit_->at(i).band_;
        slotRacBandLimit_[i].limit_ = bandLimit_->at(i).limit_;
    }

    return eNbScheduler_->racschedule(carrierFrequency_, &slotRacBandLimit_);
}


bool FDSchemeUl::scheduleRetransmissions()
{
    // step 1: schedule retransmissions, if any, for foreground UEs
    // step 2: schedule retransmissions, if any and if there is still space, for background UEs

    bool spaceEnded = false;
    bool skip = false;
    // optimization: do not call rtxschedule if no process is ready for rtx for this carrier
    if (direction_ == DL && mac_->getProcessForRtx(carrierFrequency_, DL) == 0)
        skip = true;
    if (direction_ == UL && mac_->getProcessForRtx(carrierFrequency_, UL) == 0 && mac_->getProcessForRtx(carrierFrequency_, D2D) == 0)
        skip = true;

    if (!skip)
    {
        // reset the band limit vector used for retransmissions
        // TODO do this only when it was actually used in previous slot
        for (unsigned int i = 0; i < bandLimit_->size(); i++)
        {
            // copy the element
            slotRtxBandLimit_[i].band_ = bandLimit_->at(i).band_;
            slotRtxBandLimit_[i].limit_ = bandLimit_->at(i).limit_;
        }
        spaceEnded = eNbScheduler_->rtxschedule(carrierFrequency_, &slotRtxBandLimit_);
    }

    if (!spaceEnded)
    {
        // check if there are backlogged retransmissions for background UEs
        BackgroundTrafficManager* bgTrafficManager = mac_->getBackgroundTrafficManager(carrierFrequency_);
        std::list<int>::const_iterator it = bgTrafficManager->getBackloggedUesBegin(direction_, true),
                                       et = bgTrafficManager->getBackloggedUesEnd(direction_, true);
        if (it != et)
        {
            // if the bandlimit was not reset for foreground UEs, do it here
            if (skip)
            {
                // reset the band limit vector used for retransmissions
                // TODO do this only when it was actually used in previous slot
                for (unsigned int i = 0; i < bandLimit_->size(); i++)
                {
                    // copy the element
                    slotRtxBandLimit_[i].band_ = bandLimit_->at(i).band_;
                    slotRtxBandLimit_[i].limit_ = bandLimit_->at(i).limit_;
                }
            }
            spaceEnded = eNbScheduler_->rtxscheduleBackground(carrierFrequency_, &slotRtxBandLimit_);
        }
    }
    return spaceEnded;
}

void FDSchemeUl::schedule()
{
    EV << "FDSchemeUl::schedule - start uplink scheduling" << endl;

    activeConnectionSet_ = eNbScheduler_->readActiveConnections();

    // obtain the list of cids that can be scheduled on this carrier
    buildCarrierActiveConnectionSet();

    // scheduling
    prepareSchedule();
    commitSchedule();
}

void FDSchemeUl::buildCarrierActiveConnectionSet()
{
    carrierActiveConnectionSet_.clear();

    // put in the activeConnectionSet only connections that are active
    // and whose UE is enabled to use this carrier

    if (binder_ == NULL)
        binder_ = getBinder();

    const UeSet& carrierUeSet = binder_->getCarrierUeSet(carrierFrequency_);
    ActiveSet::iterator it = activeConnectionSet_->begin();
    for (; it != activeConnectionSet_->end(); ++it)
    {
        if (carrierUeSet.find(MacCidToNodeId(*it)) != carrierUeSet.end())
            carrierActiveConnectionSet_.insert(*it);
    }
}


unsigned int FDSchemeUl::requestGrant(MacCid cid, unsigned int bytes, bool& terminate, bool& active, bool& eligible, BandLimitVector* bandLim)
{
    if (bandLim == NULL)
    {
        // reset the band limit vector used for requesting grants
        for (unsigned int i = 0; i < bandLimit_->size(); i++)
        {
            // copy the element
            slotReqGrantBandLimit_[i].band_ = bandLimit_->at(i).band_;
            slotReqGrantBandLimit_[i].limit_ = bandLimit_->at(i).limit_;
        }
        bandLim = &slotReqGrantBandLimit_;
    }

//    // === DEBUG === //
//    EV << "LteScheduler::requestGrant - Set Band Limit for this carrier: " << endl;
//    BandLimitVector::iterator it = bandLim->begin();
//    for (; it != bandLim->end(); ++it)
//    {
//        EV << " - Band[" << it->band_ << "] limit[";
//        for (int cw=0; cw < it->limit_.size(); cw++)
//        {
//            EV << it->limit_[cw] << ", ";
//        }
//        EV << "]" << endl;
//    }
//    // === END DEBUG === //

    return eNbScheduler_->scheduleGrant(cid, bytes, terminate, active, eligible, carrierFrequency_, bandLim);
}

