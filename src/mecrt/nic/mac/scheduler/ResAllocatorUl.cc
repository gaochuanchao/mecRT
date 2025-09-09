//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//

#include "mecrt/nic/mac/scheduler/ResAllocatorUl.h"

BandAdjustStatus ResAllocatorUl::checkScheduledApp(AppId appId)
{
    // check if the latest data rate is enough to offload the app data within the maximum offloading time
    // update the data rate first
    // appDataRate_[appId] = getBandRatePerTTI(appId, appBands_[appId]);    // byte rate per band per TTI
    MacNodeId ueId = MacCidToNodeId(appId);

    if (scheduledApp_.find(appId) == scheduledApp_.end())
    {
        throw cRuntimeError("ResAllocatorUl::checkScheduledApp - app %d is not in the scheduled app list.", appId);
    }

    if (vehDataRate_[ueId] == 0)
    {
        EV << "ResAllocatorUl::checkScheduledApp - app " << appId << " has 0 data rate, pause the service." << endl;
        releaseAllocatedBands(appId);
        scheduledApp_.erase(appId);
        pausedApp_.insert(appId);
        return STOP_SERVICE;
    }

    int dataRatePerMs = vehDataRate_[ueId] * ttiPerMs_;  // byte rate per band per milliseconds
    omnetpp::simtime_t maxOffloadTime = appGrants_[appId].maxOffloadTime;
    double dataSize = appGrants_[appId].inputSize + dataAddOn_; // data size to be offloaded

    // int prevBands = appBands_[appId].size();
    int grantedBands = appGrants_[appId].grantedBands;
    releaseAllocatedBands(appId);
    int totalAvailBand = availBands_.size() + reservedBand_.size();
    
    if (totalAvailBand >= grantedBands)
    {
        // check if the origial band allocation is enough to offload the data within the maximum offloading time
        omnetpp::simtime_t uploadTime = dataSize / (dataRatePerMs * grantedBands * 1000.0);
        if (uploadTime <= maxOffloadTime)
        {
            EV << "ResAllocatorUl::checkScheduledApp - app " << appId << " has adjusted the band allocation." << endl;
            allocateBands(appId, grantedBands);
            return ADJUSTED;
        }
        else
        {
            double bytePerBand = dataRatePerMs * 1000.0 * maxOffloadTime.dbl();
            int minBandDemand = ceil(dataSize / bytePerBand);
            if (minBandDemand <= totalAvailBand)
            {
                EV << "ResAllocatorUl::checkScheduledApp - app " << appId << " has adjusted the band allocation." << endl;
                allocateBands(appId, minBandDemand);
                return ADJUSTED;
            }
            else
            {
                EV << "ResAllocatorUl::checkScheduledApp - not enough available resource to adjust band allocation for app " << appId << ", pause the service." << endl;
                pausedApp_.insert(appId);
                scheduledApp_.erase(appId);
                return STOP_SERVICE;
            }
        }
    }
    else
    {
        omnetpp::simtime_t uploadTime = dataSize / (dataRatePerMs * totalAvailBand * 1000.0);
        if (uploadTime <= maxOffloadTime)
        {
            EV << "ResAllocatorUl::checkScheduledApp - app " << appId << " has adjusted the band allocation." << endl;
            allocateBands(appId, totalAvailBand);
            return ADJUSTED;
        }
        else
        {
            EV << "ResAllocatorUl::checkScheduledApp - not enough available resource to adjust band allocation for app " << appId << ", pause the service." << endl;
            pausedApp_.insert(appId);
            scheduledApp_.erase(appId);
            return STOP_SERVICE;
        }
    }
}

BandAdjustStatus ResAllocatorUl::checkPausedApp(AppId appId)
{
    MacNodeId ueId = MacCidToNodeId(appId);

    if (pausedApp_.find(appId) == pausedApp_.end())
    {
        throw cRuntimeError("ResAllocatorUl::checkPausedApp - app %d is not in the paused app list.", appId);
    }

    if (vehDataRate_[ueId] == 0)
    {
        EV << "ResAllocatorUl::checkPausedApp - app " << appId << " has 0 data rate, continue pausing service." << endl;
        return STOP_SERVICE;
    }

    int dataRatePerMs = vehDataRate_[ueId] * ttiPerMs_;  // byte rate per band per milliseconds
    omnetpp::simtime_t maxOffloadTime = appGrants_[appId].maxOffloadTime;
    double bytePerBand = dataRatePerMs * 1000.0 * maxOffloadTime.dbl();
    double dataSize = appGrants_[appId].inputSize + dataAddOn_; // data size to be offloaded
    int minBandDemand = ceil(dataSize / bytePerBand);
    int totalBand = availBands_.size() + reservedBand_.size();

    if (minBandDemand <= totalBand)
    {
        EV << "ResAllocatorUl::checkPausedApp - app " << appId << " can be scheduled after adjusting the band allocation." << endl;
        int grantedBand = appGrants_[appId].grantedBands;
        if ((minBandDemand < grantedBand) && (grantedBand <= totalBand))
            allocateBands(appId, grantedBand);
        else
            allocateBands(appId, minBandDemand);

        pausedApp_.erase(appId);
        scheduledApp_.insert(appId);
        return ADJUSTED;
    }
    else
    {
        EV << "ResAllocatorUl::checkPausedApp - not enough available resource to allocate app " << appId << ", continue pausing service." << endl;
        return STOP_SERVICE;
    }
}


bool ResAllocatorUl::schedulePendingApp(AppId appId)
{
    // AppGrant& grant = appGrants_[appId];
    MacNodeId ueId = MacCidToNodeId(appId);

    if (appGrants_[appId].grantedBands == 0)
    {
        EV << "ResAllocatorUl::schedulePendingApp - app " << appId << " has 0 granted bands, stop service." << endl;
        return false;
    }

    if (availBands_.size() < appGrants_[appId].grantedBands)
    {
        EV << "ResAllocatorUl::schedulePendingApp - not enough bands for newly granted app " << appId << endl;
        return false;
    }

    int dataRateMs = vehDataRate_[ueId] * ttiPerMs_;  // byte rate per band per ms
    if (dataRateMs == 0)
    {
        EV << "ResAllocatorUl::schedulePendingApp - app " << appId << " has 0 data rate, stop service." << endl;
        return false;
    }

    omnetpp::simtime_t maxOffloadTime = appGrants_[appId].maxOffloadTime;
    double bytePerSecond = dataRateMs * appGrants_[appId].grantedBands * 1000.0;
    double dataSize = appGrants_[appId].inputSize + dataAddOn_; // data size to be offloaded
    omnetpp::simtime_t uploadTime = dataSize / bytePerSecond;

    if (uploadTime <= maxOffloadTime)
    {
        allocateBands(appId, appGrants_[appId].grantedBands);
        scheduledApp_.insert(appId);
        return true;
    }
    else
    {
        EV << "ResAllocatorUl::schedulePendingApp - channel quality is bad for newly granted app " << appId << endl;
        return false;
    }
}

void ResAllocatorUl::allocateBands(AppId appId, int numBand)
{
    // MacNodeId ueId = MacCidToNodeId(appId);
    appBands_[appId].clear();

    if (numBand <= availBands_.size())
    {
        for (int i = 0; i < numBand; i++)
        {
            auto bPtr = availBands_.begin();
            Band band = *bPtr;
            appBands_[appId].insert(band);
            availBands_.erase(bPtr);
        }

        appAvailBands_[appId] = numBand;
    }
    else
    {
        // first add all bands in availBands_ to appBands_
        appBands_[appId] = availBands_;
        appAvailBands_[appId] = availBands_.size();

        int remainBand = numBand - availBands_.size();
        availBands_.clear();
        // then add the remaining bands in reservedBand_ to appBands_
        if (remainBand > reservedBand_.size())
        {
            throw cRuntimeError("ResAllocatorUl::allocateBands - not enough bands for app %d.", appId);
        }

        for (int i = 0; i < remainBand; i++)
        {
            auto bPtr = reservedBand_.begin();
            Band band = *bPtr;
            appBands_[appId].insert(band);
            reservedBand_.erase(bPtr);
        }
    }

    updateAppRbMap(appId);
}

void ResAllocatorUl::terminateService(AppId appId)
{
    scheduledApp_.erase(appId);
    pausedApp_.erase(appId);
    // appGrants_.erase(appId);     // reset by the mac stack instead of here
    if (appBands_.find(appId) != appBands_.end())
        releaseAllocatedBands(appId);
}

void ResAllocatorUl::releaseAllocatedBands(AppId appId)
{
    for (auto band : appBands_[appId])
    {
        if (band < threshold_)
            availBands_.insert(band);
        else
            reservedBand_.insert(band);
    }
    appBands_.erase(appId);
    
    appAvailBands_.erase(appId);
    appRbMap_.erase(appId);
}


void ResAllocatorUl::updateAppRbMap(AppId appId)
{
    appRbMap_[appId].clear();

    for (auto band : appBands_[appId])
    {
        appRbMap_[appId][band] = rbPerBand_;
    }
}

void ResAllocatorUl::readAppRbOccupation(const AppId appId, std::map<Band, unsigned int>& rbMap)
{
    if (appRbMap_.find(appId) != appRbMap_.end())
    {
        for (auto it = appRbMap_[appId].begin(); it != appRbMap_[appId].end(); ++it)
        {
            rbMap[it->first] = it->second;
        }
    }
}

void ResAllocatorUl::initBandStatus()
{
    availBands_ = set<Band>();
    reservedBand_ = set<Band>();
    for (int i = 0; i < numBands_; i++)
    {
        if (i < threshold_)
            availBands_.insert(i);
        else
            reservedBand_.insert(i);
    }
}

/***
 * ========================================================================================================
 * Following part is for future improvement (select the best bands for the app)
 * ========================================================================================================
 */


// bool ResAllocatorUl::adjustBandAllocation(AppId appId, set<Band>& candidateBands)
// {
//     /**
//      * amc_->getTxParams(): get the current transmission parameters of the UE
//      * amc_->existTxParams(): check if the transmission parameters exist
//      * UserTxParams::restoreDefaultValues(): reset the transmission parameters to default values
//      */
//     MacNodeId ueId = MacCidToNodeId(appId);

//     // check if needs to reset the transmission parameters
//     if (amc_->existTxParams(ueId, dir_, frequency_))
//     {
//         UserTxParams txParams = amc_->getTxParams(ueId, dir_, frequency_);
//         txParams.restoreDefaultValues();
//         amc_->setTxParams(ueId, dir_, txParams, frequency_);
//     }

//     auto maxOffloadTime = appGrants_[appId].maxOffloadTime;
//     int dataRatePerMs = getBandRatePerTTI(appId, candidateBands) * pow(2, numerology_);    // byte rate per band per milliseconds

//     if (dataRatePerMs == 0)
//     {
//         EV << "ResAllocatorUl::adjustBandAllocation - app " << appId << " has 0 data rate, stop service." << endl;
//         return false;
//     }

//     int minBandDemand = ceil(appGrants_[appId].inputSize / (dataRatePerMs * 1000.0 * maxOffloadTime));  // the minimum number of bands needed for offloading the app data
//     if (minBandDemand > candidateBands.size())
//     {
//         EV << "ResAllocatorUl::adjustBandAllocation - not enough bands for data offloading, stop service." << endl;
//         return false;
//     }
//     else
//     {
//         allocateBands(appId, minBandDemand);
//         updateAppRbMap(appId);

//         // update the data rate
//         appDataRate_[appId] = getBandRatePerTTI(appId, appBands_[appId]);
//         return true;
//     }
// }


// void ResAllocatorUl::allocateBands(AppId appId, int numBand)
// {
//     MacNodeId ueId = MacCidToNodeId(appId);
    // select the needed bands with the highest channel quality
    // const LteSummaryFeedback& sfb = amc_->getFeedback(ueId, MACRO, TRANSMIT_DIVERSITY, dir_, frequency_);
    // vector<Cqi> summaryCqi = sfb.getCqi(0); // get a vector of  CQI over first CW
    // get the top minBandDemand bands with the highest CQI, check the bands in availBands first
    // vector<Band> topBands;
    // if (numBand <= availBands_.size())
    // {
    //     vector<pair<Band, Cqi>> pairs;
    //     for (auto band : availBands_)
    //     {
    //         pairs.push_back(make_pair(band, summaryCqi[band]));
    //     }
    //     // sort the bands by CQI in descending order
    //     sort(pairs.begin(), pairs.end(), [](const pair<Band, Cqi>& a, const pair<Band, Cqi>& b) { return a.second > b.second; });
    //     for (int i = 0; i < numBand; i++)
    //     {
    //         topBands.push_back(pairs[i].first);
    //         availBands.erase(pairs[i].first);
    //     }
    // }
    // else
    // {
    //     // if the available bands are not enough, select the top minBandDemand bands with the highest CQI
    //     topBands = vector<Band>(availBands.begin(), availBands.end());
    //     vector<pair<Band, Cqi>> pairs;
    //     for (auto band : reservedBands)
    //     {
    //         pairs.push_back(make_pair(band, summaryCqi[band]));
    //     }
    //     // sort the bands by CQI in descending order
    //     sort(pairs.begin(), pairs.end(), [](const pair<Band, Cqi>& a, const pair<Band, Cqi>& b) { return a.second > b.second; });
    //     for (int i = 0; i < numBand - topBands.size(); i++)
    //     {
    //         topBands.push_back(pairs[i].first);
    //         reservedBands.erase(pairs[i].first);
    //     }
    // }
    // appBands_[appId] = set<Band>(topBands.begin(), topBands.end());

    // if (amc_->existTxParams(ueId, dir_, frequency_))
    // {
    //     UserTxParams txParams = amc_->getTxParams(ueId, dir_, frequency_);
    //     txParams.restoreDefaultValues();
    //     amc_->setTxParams(ueId, dir_, txParams, frequency_);
    // }
// }

// compute the data rate per band per TTI
// int ResAllocatorUl::getBandRatePerTTI(AppId appId, set<Band>& candidateBands)
// {
//     MacNodeId ueId = MacCidToNodeId(appId);

//     // recompute the transmission parameters and data rate
//     UsableBands useableBands = UsableBands(candidateBands.begin(), candidateBands.end());
//     amc_->setPilotUsableBands(ueId, useableBands);
//     int bytePerBand = amc_->computeBytesOnNRbs(ueId, Band(0), rbPerBand_, dir_,frequency_);   // byte rate per TTI

//     return bytePerBand;
// }

// bool ResAllocatorUl::schedulePendingApp(AppId appId)
// {
//     AppGrant& grant = appGrants_[appId];
//     MacNodeId ueId = MacCidToNodeId(appId);

//     if (carrierStatus_[frequency_].availBands.size() < grant.grantedBands)
//     {
//         EV << "ResAllocatorUl::schedulePendingApp - not enough bands for newly granted app " << appId << endl;
//         return false;
//     }

//     int dataRate = carrierStatus_[frequency_].vehDataRate[ueId];  // byte rate per band per TTI
//     if (dataRate == 0)
//     {
//         EV << "ResAllocatorUl::schedulePendingApp - app " << appId << " has 0 data rate, stop service." << endl;
//         return false;
//     }

//     omnetpp::simtime_t maxOffloadTime = grant.maxOffloadTime;
//     omnetpp::simtime_t uploadTime = grant.inputSize / (dataRate * pow(2, numerology_) * grant.grantedBands * 1000.0);

//     if (uploadTime <= maxOffloadTime)
//     {
//         allocateBands(appId, grant.grantedBands);
//         scheduledApp_.insert(appId);

//         appDataRate_[appId] = getBandRatePerTTI(appId, appBands_[appId]);
//         updateAppRbMap(appId);
//         return true;
//     }
//     else
//     {
//         EV << "ResAllocatorUl::schedulePendingApp - channel quality is bad for newly granted app " << appId << endl;
//         return false;
//     }

    // if (uploadTime > maxOffloadTime)
    // {
    //     set<Band> topBands = findBestBands(appId, grant.grantedBands);
    //     // check if needs to reset the transmission parameters
    //     if (amc_->existTxParams(MacCidToNodeId(appId), dir_, frequency_))
    //     {
    //         UserTxParams txParams = amc_->getTxParams(ueId, dir_, frequency_);
    //         txParams.restoreDefaultValues();
    //         amc_->setTxParams(ueId, dir_, txParams, frequency_);
    //     }
    //     // determine the new data rate per band per milliseconds
    //     int newRatePerTTI = getBandRatePerTTI(appId, topBands);
    //     if (newRatePerTTI == 0)
    //     {
    //         // reset the transmission parameters
    //         UserTxParams txParams = amc_->getTxParams(ueId, dir_, frequency_);
    //         txParams.restoreDefaultValues();
    //         amc_->setTxParams(ueId, dir_, txParams, frequency_);
            
    //         EV << "ResAllocatorUl::schedulePendingApp - app " << appId << " has 0 data rate, stop service." << endl;
    //         return false;
    //     }

    //     int newDataRate = newRatePerTTI * pow(2, numerology_);
    //     uploadTime = grant.inputSize / (newDataRate * grant.grantedBands * 1000.0);

    //     if(uploadTime > maxOffloadTime)
    //     {
    //         // reset the transmission parameters
    //         UserTxParams txParams = amc_->getTxParams(ueId, dir_, frequency_);
    //         txParams.restoreDefaultValues();
    //         amc_->setTxParams(ueId, dir_, txParams, frequency_);
            
    //         EV << "ResAllocatorUl::schedulePendingApp - channel quality is bad for newly granted app " << appId << endl;
    //         return false;
    //     }
        
    //     appBands_[appId] = set<Band>();
    //     for (auto band : topBands)
    //     {
    //         appBands_[appId].insert(band);
    //         carrierStatus_[frequency_].availBands.erase(band);
    //     }

    //     appDataRate_[appId] = newRatePerTTI;
    //     updateAppRbMap(appId);
    //     return true;
    // }
    // else
    // {
    //     allocateBands(appId, grant.grantedBands);
    //     scheduledApp_.insert(appId);

    //     appDataRate_[appId] = getBandRatePerTTI(appId, appBands_[appId]);
    //     updateAppRbMap(appId);
    //     return true;
    // }
// }

// set<Band> ResAllocatorUl::findBestBands(AppId appId, int numBand)
// {
//     MacNodeId ueId = MacCidToNodeId(appId);
//     // select the needed bands with the highest channel quality
//     const LteSummaryFeedback& sfb = amc_->getFeedback(ueId, MACRO, TRANSMIT_DIVERSITY, dir_, frequency_);
//     vector<Cqi> summaryCqi = sfb.getCqi(0); // get a vector of  CQI over first CW
//     // get the top minBandDemand bands with the highest CQI, check the bands in availBands first
//     vector<Band> topBands;

//     set<Band>& availBands = carrierStatus_[frequency_].availBands;
//     vector<pair<Band, Cqi>> pairs;
//     for (auto band : availBands)
//     {
//         pairs.push_back(make_pair(band, summaryCqi[band]));
//     }
//     // sort the bands by CQI in descending order
//     sort(pairs.begin(), pairs.end(), [](const pair<Band, Cqi>& a, const pair<Band, Cqi>& b) { return a.second > b.second; });
//     for (int i = 0; i < numBand; i++)
//     {
//         topBands.push_back(pairs[i].first);
//     }

//     return set<Band>(topBands.begin(), topBands.end());
// }


