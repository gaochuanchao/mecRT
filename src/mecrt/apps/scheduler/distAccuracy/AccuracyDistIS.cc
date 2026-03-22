//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    AccuracyDistIS.cc / AccuracyDistIS.h
//
//  Description:
//    This file provides the implementation of the AccuracyDistIS scheduling scheme, which is a 
//    distributed scheduling scheme for accuracy optimization in the Mobile Edge Computing System.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2026-03-22
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/apps/scheduler/distAccuracy/AccuracyDistIS.h"

AccuracyDistIS::AccuracyDistIS(Scheduler *scheduler)
    : SchemeBase(scheduler)
{
    // Additional initialization specific to the distributed scheduling scheme can be added here
    rsuId_ = scheduler_->rsuId_;  // Get the RSU ID of the scheduler node from the scheduler

    EV << NOW << " AccuracyDistIS::AccuracyDistIS - Initialized" << endl;
}


void AccuracyDistIS::initializeData()
{
    // Initialize the scheduling data
    EV << NOW << " AccuracyDistIS::initializeData - Initializing scheduling data" << endl;

    // initialize the apps vector, using vector index number to represent the real application ID
    appIds_.clear();
    appId2Index_.clear();  // clear the mapping from application ID to index
    appIds_.reserve(unscheduledApps_.size());  // reserve memory for the apps vector
    for (AppId appId : unscheduledApps_)    // enumerate the unscheduled apps
    {
        appIds_.push_back(appId);
        appId2Index_[appId] = appIds_.size() - 1;  // map the application ID to the index in the apps vector
    }

    maxRB_ = rsuStatus_[rsuId_].bands - rsuOnholdRbs_[rsuId_];  // get the resource block capacity of current RSU/gNB
    maxCU_ = rsuStatus_[rsuId_].cmpUnits - rsuOnholdCus_[rsuId_];  // get the computing unit capacity of current RSU/gNB

    // clear the service instances vectors
    instAppIndex_.clear();  // application IDs for the service instances
    instRBs_.clear();  // resource blocks for the service instances
    instCUs_.clear();  // computing units for the service instances
    instUtility_.clear();  // energy savings for the service instances
    instMaxOffTime_.clear();  // maximum allowable offloading time for the service instances
    instServiceType_.clear();  // Clear the selected service type vector
    instExeDelay_.clear();  // Clear the execution delay vector

    instAppBeginIndex_ = vector<int>(appIds_.size(), -1);  // the begin index of an application's service instances in instAppIndex_
    instAppEndIndex_ = vector<int>(appIds_.size(), -1);  // the end index of an application's service instances in instAppIndex_
    
    appMaxOffTime_.clear();  // maximum allowable offloading time for the applications
    appUtility_.clear();  // utility (i.e., energy savings) for the applications
    appExeDelay_.clear();  // Clear the application execution delay vector
    appServiceType_.clear();  // Clear the application service type vector
}


void AccuracyDistIS::generateScheduleInstances()
{
    EV << NOW << " AccuracyDistIS::generateScheduleInstances - Generating schedule instances" << endl;

    initializeData();  // transform the scheduling data

    bool debugMode = false;

    int instIndex = 0;  // the index for the current service instances
    for (int appIndex = 0; appIndex < appIds_.size(); appIndex++)    // enumerate the unscheduled apps
    {
        AppId appId = appIds_[appIndex];  // get the application ID
        instAppBeginIndex_[appIndex] = instIndex;  // set the begin index of the application's service instances
        
        double period = appInfo_[appId].period.dbl();
        if (period <= 0)
        {
            EV << "\t invalid period for application " << appId << ", skip" << endl;
            continue;
        }

        MacNodeId vehId = appInfo_[appId].vehId;
        if (debugMode)
            EV << "\t period: " << period << ", RSU " << rsuId_ << 
            " (maxRB: " << maxRB_ << ", maxCU: " << maxCU_ << ")" << endl;

        // if maxRB/rbStep_ is smaller than maxCU/cuStep_, enumerate RB
        if (maxRB_ / rbStep_ < maxCU_ / cuStep_)
        {
            for (int resBlocks = 1; resBlocks <= maxRB_; resBlocks += rbStep_)
            {
                double offloadDelay = computeOffloadDelay(vehId, rsuId_, resBlocks, appInfo_[appId].inputSize);
                if (debugMode)
                {
                    EV << "\t\tenumerate resBlocks " << resBlocks << ", offloadDelay: " << offloadDelay << "s" << endl;
                }
                    
                if (offloadDelay + offloadOverhead_ >= period)
                    continue;  // if the forwarding delay is too long, break

                double exeDelayThreshold = period - offloadDelay - offloadOverhead_;
                // enumerate all possible service types for the application
                set<string> serviceTypes = db_->getGnbServiceTypes();
                for (const string& serviceType : serviceTypes)
                {
                    int minCU = computeMinRequiredCUs(rsuId_, exeDelayThreshold, serviceType);
                    if (debugMode)
                    {
                        EV << "\t\t\tservice type " << serviceType << ", minCU: " << minCU << ", exeDelayThreshold: " << exeDelayThreshold << endl;
                        debugMode = false;  // only print once
                    }

                    if (minCU > maxCU_)
                        continue;  // if the minimum computing units required is larger than the maximum computing units available, skip

                    double exeDelay = computeExeDelay(rsuId_, minCU, serviceType);
                    double utility = computeUtility(appId, serviceType) / period;   // utility per second
                    if (utility <= 0)   // if the saved energy is less than 0, skip
                        continue;

                    // AppInstance instance = {appIndex, offRsuIndex, procRsuIndex, resBlocks, cmpUnits};
                    instAppIndex_.push_back(appIndex);
                    instRBs_.push_back(resBlocks);
                    instCUs_.push_back(minCU);
                    instUtility_.push_back(utility);  // energy savings for the instance
                    instMaxOffTime_.push_back(period - exeDelay - offloadOverhead_);  // maximum offloading time for the instance
                    instServiceType_.push_back(serviceType);  // selected service type for the instance
                    instExeDelay_.push_back(exeDelay);  // execution delay for the instance

                    instIndex++;
                }
            }
        }
        else    // else enumerate CUs
        {
            // enumerate all possible service types for the application
            set<string> serviceTypes = db_->getGnbServiceTypes();
            for (const string& serviceType : serviceTypes)
            {
                for (int cmpUnits = 1; cmpUnits <= maxCU_; cmpUnits += cuStep_)
                {
                    double exeDelay = computeExeDelay(rsuId_, cmpUnits, serviceType);
                    if (exeDelay + offloadOverhead_ >= period)
                        continue;  // if the total execution and forwarding time is too long, skip

                    // determine the smallest resource blocks required to meet the deadline
                    double offloadTimeThreshold = period - exeDelay - offloadOverhead_;
                    int minRB = computeMinRequiredRBs(vehId, rsuId_, offloadTimeThreshold, appInfo_[appId].inputSize);
                    if (minRB > maxRB_)
                        continue;  // if the minimum resource blocks required is larger than the maximum resource blocks available, continue

                    double utility = computeUtility(appId, serviceType) / period;   // utility per second
                    if (utility <= 0)   // if the saved energy is less than 0, skip
                        continue;

                    // AppInstance instance = {appIndex, offRsuIndex, procRsuIndex, resBlocks, cmpUnits, serviceType};
                    instAppIndex_.push_back(appIndex);
                    instRBs_.push_back(minRB);
                    instCUs_.push_back(cmpUnits);
                    instUtility_.push_back(utility);  // energy savings for the instance
                    instMaxOffTime_.push_back(offloadTimeThreshold);  // maximum offloading time for the instance
                    instServiceType_.push_back(serviceType);  // selected service type for the instance
                    instExeDelay_.push_back(exeDelay);  // execution delay for the instance

                    instIndex++;
                }
            }
        }

        instAppEndIndex_[appIndex] = instIndex;  // set the end index of the application's service instances
    }
}





double AccuracyDistIS::computeExeDelay(MacNodeId rsuId, double cmpUnits, string serviceType)
{
    /***
     * total computing cycle = T * C
     * where T is the execution time for the full computing resource allocation, and C is the capacity
     *      time = T * C / n
     * * where n is the number of computing units allocated to the application
     */

    //check if db_ is not null
    if (!db_) 
    {
        throw std::runtime_error("AccuracyDistIS::computeExeDelay - db_ is null, cannot compute execution delay");
    }
    
    // Get the execution time from the database
    double baseExeTime = db_->getGnbExeTime(serviceType, rsuStatus_[rsuId].deviceType);
    if (baseExeTime <= 0) 
    {
        stringstream ss;
        ss << NOW << " AccuracyDistIS::computeExeDelay - the demanded service " << serviceType 
           << " is not supported on RSU[nodeId=" << rsuId << "], return INFINITY";
        throw std::runtime_error(ss.str());
    }

    if (rsuStatus_[rsuId].cmpCapacity <= 0 || cmpUnits <= 0) {
        return INFINITY;
    }

    return baseExeTime * rsuStatus_[rsuId].cmpCapacity / cmpUnits;
}



int AccuracyDistIS::computeMinRequiredCUs(MacNodeId rsuId, double exeTimeThreshold, string serviceType)
{
    /***
     * total computing cycle = T * C
     * where T is the execution time for the full computing resource allocation, and C is the capacity
     *      time = T * C / n
     * * where n is the number of computing units allocated to the application
     */

    //check if db_ is not null
    if (!db_) 
    {
        throw std::runtime_error("AccuracyDistIS::computeExeDelay - db_ is null, cannot compute execution delay");
    }
    
    // Get the execution time from the database
    double baseExeTime = db_->getGnbExeTime(serviceType, rsuStatus_[rsuId].deviceType);
    if (baseExeTime <= 0) 
    {
        stringstream ss;
        ss << NOW << " AccuracyDistIS::computeExeDelay - the demanded service " << serviceType 
           << " is not supported on RSU[nodeId=" << rsuId << "], return INFINITY";
        throw std::runtime_error(ss.str());
    }

    if (rsuStatus_[rsuId].cmpCapacity <= 0 || exeTimeThreshold <= 0) {
        return INFINITY;
    }

    return ceil(baseExeTime * rsuStatus_[rsuId].cmpCapacity / exeTimeThreshold);
}


int AccuracyDistIS::computeMinRequiredRBs(MacNodeId vehId, MacNodeId rsuId, double offloadTimeThreshold, int dataSize)
{
    /**
     * offload time = dataSize / rate
     * rate = rb * virtualLinkRate_
     * => offload time = dataSize / (rb * virtualLinkRate_)
     * => rb = dataSize / (offload time * virtualLinkRate_)
     */

    /***
     * during data transmission, there are multiple headers added to the data. In total, 33 bytes are added
     *      - UDP header (8B)
     *      - IP header (20B)
     *      - PdcpPdu header (1B)
     *      - RlcSdu header (2B) : RLC_HEADER_UM
     *      - MacPdu header (2B) : MAC_HEADER
     */
    if (offloadTimeThreshold <= 0) {
        return INFINITY;
    }

    double actualSize = dataSize + 33;
    double bytesPerRb = offloadTimeThreshold / ttiPeriod_ * veh2RsuRate_[make_tuple(vehId, rsuId)];
    
    return ceil(actualSize / bytesPerRb);
}



double AccuracyDistIS::computeUtility(AppId appId, string serviceType)
{
    /**
     * The utility is defined as the accuracy improvement brought by offloading
     * Utility = accuracy_offload - accuracy_local
     */
    if (!db_) 
    {
        throw std::runtime_error("AccuracyDistIS::computeUtility - db_ is null, cannot compute utility");
    }

    double serviceAccuracy = db_->getGnbServiceAccuracy(serviceType);
    if (serviceAccuracy <= 0) 
    {
        stringstream ss;
        ss << NOW << " AccuracyDistIS::computeUtility - the demanded service " << serviceType 
           << " is not supported, cannot compute utility";
        throw std::runtime_error(ss.str());
    }

    return serviceAccuracy - appInfo_[appId].accuracy;
}

