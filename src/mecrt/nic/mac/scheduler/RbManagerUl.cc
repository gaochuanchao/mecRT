//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    RbManagerUl.cc / RbManagerUl.h
//
//  Description:
//    This file implements the resource block manager for the uplink of NR in the MEC.
//    Currently only frequency division resource allocation is supported.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/nic/mac/scheduler/RbManagerUl.h"


bool RbManagerUl::scheduleGrantedApp(AppId appId)
{
    MacNodeId ueId = MacCidToNodeId(appId);
    int numBand = appGrantInfos_[appId].numGrantedBands;

    if (numBand == 0)
    {
        EV << "RbManagerUl::scheduleGrantedApp - app " << appId << " has 0 granted bands, fail to schedule." << endl;
        return false;
    }

    if (flexibleBands_.size() < numBand)
    {
        EV << "RbManagerUl::scheduleGrantedApp - not enough flexible bands for newly granted app " << appId << endl;
        return false;
    }

    int dataRateMs = vehDataRate_[ueId] * ttiPerMs_;  // byte rate per band per ms
    if (dataRateMs == 0)
    {
        EV << "RbManagerUl::scheduleGrantedApp - app " << appId << " has 0 data rate, fail to schedule." << endl;
        return false;
    }

    int minBandDemand = getMinimumRequiredBands(appId); // calculate the minimum required bands for the app
    if (minBandDemand <= numBand)
    {
        // allocate the granted bands from the flexible bands to the app
        auto it = flexibleBands_.begin();
        for (int i = 0; i < numBand; i++)
        {
            appGrantInfos_[appId].grantedBandSet.insert(*it);
            appGrantedRbMap_[appId][*it] = rbPerBand_; // allocate rb
            it++;
        }
        // remove the allocated bands from the flexible bands
        for (auto band : appGrantInfos_[appId].grantedBandSet)
        {
            flexibleBands_.erase(band);
        }

        appToBeInitialized_.erase(appId); // remove the app from the to-be-initialized list
        scheduledApp_.insert(appId);
        return true;
    }
    else
    {
        EV << "RbManagerUl::scheduleGrantedApp - channel quality is bad for newly granted app " << appId << endl;
        return false;
    }
}


bool RbManagerUl::scheduleActiveApp(AppId appId)
{
    // check if the app is already scheduled
    if (scheduledApp_.find(appId) == scheduledApp_.end())
    {
        throw cRuntimeError("RbManagerUl::scheduleActiveApp - app %d is not in the scheduled app list.", appId);
    }
    
    releaseTempBands(appId); // release the temporary granted bands for the app

    // check if the granted bands is enough for the app
    if (isGrantEnough(appId))
    {
        EV << "RbManagerUl::scheduleActiveApp - app " << appId << " remains scheduled." << endl;
        return true; // the app is resumed
    }
    else
    {
        EV << "RbManagerUl::scheduleActiveApp - app " << appId << " is paused due to bad channel quality." << endl;
        pausedApp_.insert(appId); // mark the app as paused
        scheduledApp_.erase(appId); // remove the app from the scheduled apps
        return false; // the app is paused
    }
}


bool RbManagerUl::schedulePausedApp(AppId appId)
{
    // check if the app is already paused
    if (pausedApp_.find(appId) == pausedApp_.end())
    {
        throw cRuntimeError("RbManagerUl::schedulePausedApp - app %d is not in the paused app list.", appId);
    }
    
    // check if the granted bands is enough for the app
    if (isGrantEnough(appId))
    {
        EV << "RbManagerUl::schedulePausedApp - app " << appId << " is resumed." << endl;
        pausedApp_.erase(appId); // remove the app from the paused apps
        scheduledApp_.insert(appId); // add the app back to the scheduled apps
        return true; // the app is resumed
    }
    
    // try to allocate flexible bands for the app
    // check how many bands are needed for the app
    MacNodeId ueId = MacCidToNodeId(appId);
    if (vehDataRate_[ueId] == 0)
    {
        EV << "RbManagerUl::isGrantEnough - app " << appId << " has 0 data rate, fail to schedule." << endl;
        return false;
    }

    int minBandDemand = getMinimumRequiredBands(appId); // calculate the minimum required bands for the app
    // minBandDemand must be greater than granted bands, otherwise, isGrantEnough() would have returned true
    int extraBandDemand = minBandDemand - appGrantInfos_[appId].grantedBandSet.size(); // extra bands needed for the app
    if (extraBandDemand > flexibleBands_.size())
    {
        EV << "RbManagerUl::schedulePausedApp - not enough flexible bands for app " << appId << endl;
        return false; // not enough flexible bands for the app, continue pausing service
    }

    auto it = flexibleBands_.begin();
    for (int i = 0; i < extraBandDemand; i++)
    {
        appTempRbMap_[appId][*it] = rbPerBand_; // allocate rb
        appGrantInfos_[appId].tempBands.insert(*it); // add the band to the temporary granted bands
        it++;
    }
    // remove the allocated bands from the flexible bands
    for (auto band : appGrantInfos_[appId].tempBands)
    {
        flexibleBands_.erase(band);
    }

    EV << "RbManagerUl::schedulePausedApp - app " << appId << " is resumed with extra bands allocated." << endl;
    pausedApp_.erase(appId); // remove the app from the paused apps
    scheduledApp_.insert(appId); // add the app back to the scheduled apps
    return true; // the app is resumed with extra bands allocated
}


bool RbManagerUl::isGrantEnough(AppId appId)
{
    /***
     * check if the granted bands is enough by checking if the minimum band demand is less than or equal to the granted bands
     */

    MacNodeId ueId = MacCidToNodeId(appId);
    int numBand = appGrantInfos_[appId].grantedBandSet.size(); // total granted bands for the app

    if (numBand == 0)
    {
        EV << "RbManagerUl::isGrantEnough - app " << appId << " has 0 granted bands, fail to schedule." << endl;
        return false;
    }

    if (vehDataRate_[ueId] == 0)
    {
        EV << "RbManagerUl::isGrantEnough - app " << appId << " has 0 data rate, fail to schedule." << endl;
        return false;
    }

    int minBandDemand = getMinimumRequiredBands(appId); // calculate the minimum required bands for the app
    if (minBandDemand <= numBand)
    {
        EV << "RbManagerUl::isGrantEnough - granted bands is enough for app " << appId << endl;
        return true; // the granted bands is enough for the app
    }
    else
    {
        EV << "RbManagerUl::isGrantEnough - channel quality is bad for app " << appId << endl;
        return false; // the granted bands is not enough for the app
    }
}


int RbManagerUl::getMinimumRequiredBands(AppId appId)
{
    // calculate the minimum required bands for the app
    MacNodeId ueId = MacCidToNodeId(appId);

    int dataRatePerMs = vehDataRate_[ueId] * ttiPerMs_;  // byte rate per band per milliseconds
    omnetpp::simtime_t maxOffloadTime = appGrantInfos_[appId].maxOffloadTime;
    double bytePerBand = dataRatePerMs * 1000.0 * maxOffloadTime.dbl();
    double dataSize = appGrantInfos_[appId].inputSize + dataAddOn_; // data size to be offloaded
    return ceil(dataSize / bytePerBand); // return the minimum required bands
}


void RbManagerUl::readAppRbOccupation(const AppId appId, std::map<Band, unsigned int>& rbMap)
{
    // read the app resource block occupation status
    // first check appGrantedRbMap_ for the granted bands
    if (appGrantedRbMap_.find(appId) != appGrantedRbMap_.end())
    {
        for (const auto &it : appGrantedRbMap_[appId])
        {
            rbMap[it.first] = it.second; // add the granted bands to the rbMap
        }
    }

    // then check appTempRbMap_ for the temporary granted bands
    if (appTempRbMap_.find(appId) != appTempRbMap_.end())
    {
        for (const auto &it : appTempRbMap_[appId])
        {
            rbMap[it.first] = it.second; // add the temporary granted bands to the rbMap
        }
    }
}


void RbManagerUl::resetRbStatus()
{
    // reset the resource allocation status
    scheduledApp_.clear();
    pausedApp_.clear();
    appToBeInitialized_.clear();
    vehDataRate_.clear();
    appGrantedRbMap_.clear();
    appTempRbMap_.clear();
    flexibleBands_.clear();
    appGrantInfos_.clear();

    initBandStatus(); // re-initialize the band status
}


void RbManagerUl::terminateAppService(AppId appId)
{
    // terminate the service for the app
    scheduledApp_.erase(appId); // remove the app from the scheduled apps
    pausedApp_.erase(appId); // remove the app from the paused apps
    appToBeInitialized_.erase(appId); // remove the app from the apps to be initialized

    // restore the flexible bands
    if (appGrantedRbMap_.find(appId) != appGrantedRbMap_.end())
    {
        for (const auto &it : appGrantedRbMap_[appId])
        {
            flexibleBands_.insert(it.first); // add the granted bands back to the flexible bands
        }
    }
    appGrantedRbMap_.erase(appId);
    appGrantInfos_[appId].grantedBandSet.clear(); // clear the granted bands set

    // release the granted bands for the app
    releaseTempBands(appId);

    // appGrantInfos_.erase(appId);     // reset by the mac stack instead of here
}


void RbManagerUl::releaseTempBands(AppId appId)
{
    // release the temporary granted bands for the app
    if (appTempRbMap_.find(appId) != appTempRbMap_.end())
    {
        for (const auto &band : appTempRbMap_[appId])
        {
            flexibleBands_.insert(band.first); // add the band back to the flexible bands
        }
    }
    appTempRbMap_.erase(appId); // remove the temporary granted bands
    appGrantInfos_[appId].tempBands.clear(); // clear the temporary granted bands set
}


void RbManagerUl::initBandStatus()
{
    // initialize the band status for available bands and reserved bands
    for (int i = 0; i < numBands_; i++)
    {
        Band band = static_cast<Band>(i);
        flexibleBands_.insert(band); // all bands are initially flexible
    }
}

