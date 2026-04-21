//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    SchemeFastLR.cc / SchemeFastLR.h
//
//  Description:
//    This file implements the FastLR scheduling scheme in the Mobile Edge Computing System.
//    The FastLR scheme is a linear time approximation algorithm for the multi-resource scheduling problem,
//    which categorizes all candidate service instances into two groups based on their resource demands,
//    and prioritizes the light instances (with resource demand no more than half) over the heavy instances.
//     [Scheme Source: Chuanchao Gao and Arvind Easwaran. 2025. Local Ratio based Real-time Job Offloading 
//      and Resource Allocation in Mobile Edge Computing. In Proceedings of the 4th International Workshop 
//      on Real-time and IntelliGent Edge computing (RAGE '25). Association for Computing Machinery, New York, 
//      NY, USA, Article 6, 1–6. https://doi.org/10.1145/3722567.3727843]
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/apps/scheduler/accuracyNF/AccuracyIDAssign.h"
#include <numeric>

AccuracyIDAssign::AccuracyIDAssign(Scheduler *scheduler)
    : AccuracyGreedy(scheduler)
{
    EV << NOW << " AccuracyIDAssign::AccuracyIDAssign - Initialized" << endl;
}


void AccuracyIDAssign::initializeData()
{
    EV << NOW << " AccuracyIDAssign::initializeData - Initializing scheduling data" << endl;

    AccuracyGreedy::initializeData();
    
    instMaxUtilization_.clear();
    instUtilizationSum_.clear();
    // initialize the per-RSU per-application instance vector
    int numApp = appIds_.size();  // number of applications
    instPerApp_.clear();
    instPerApp_.resize(numApp, vector<int>());  // {appIdx: {instIdx1, instIdx2, ...}}
    int numRsu = rsuIds_.size();  // number of RSUs
    instPerRsu_.clear();
    instPerRsu_.resize(numRsu, vector<int>());  // {rsuIdx: {instIdx1, instIdx2, ...}}
}


void AccuracyIDAssign::generateScheduleInstances()
{
    initializeData();  // transform the scheduling data
    EV << NOW << " AccuracyIDAssign::generateScheduleInstances - Generating schedule instances" << endl;

    bool debugMode = false;
    int instIndex = 0;  // counter for the number of generated instances
    for (int appIndex = 0; appIndex < appIds_.size(); appIndex++)    // enumerate the unscheduled apps
    {
        AppId appId = appIds_[appIndex];  // get the application ID
        
        double period = appInfo_[appId].period.dbl();
        if (period <= 0)
        {
            EV << "\t invalid period for application " << appId << ", skip" << endl;
            continue;
        }

        MacNodeId vehId = appInfo_[appId].vehId;
        if (vehAccessRsu_.find(vehId) != vehAccessRsu_.end())     // if there exists RSU in access
        {   
            if (debugMode)
                EV << "\t the number of accessible RSUs for vehicle " << vehId << " is " << vehAccessRsu_[vehId].size() << endl;
            
            for(MacNodeId rsuId : vehAccessRsu_[vehId])   // enumerate the RSUs in access
            {
                if (rsuStatus_.find(rsuId) == rsuStatus_.end())
                    continue;  // if not found, skip

                int rsuIndex = rsuId2Index_[rsuId];  // get the index of the RSU in the rsuIds vector
                int maxRB = floor(rsuRBs_[rsuIndex] * fairFactor_);  // maximum resource blocks for the offload RSU
                if (maxRB <= 0)
                    continue;  // if there is no resource blocks available, skip

                int maxCU = floor(rsuCUs_[rsuIndex] * fairFactor_);  // maximum computing units for the processing RSU
                if (maxCU <= 0)
                    continue;  // if there is no computing units available, skip

                if (debugMode)
                    EV << "\t period: " << period << ", offload RSU " << rsuId 
                        << " (maxRB: " << maxRB << ", maxCU: " << maxCU << ")" << endl;

                for (int resBlocks = 1; resBlocks <= maxRB; resBlocks += rbStep_)
                {
                    double offloadDelay = computeOffloadDelay(vehId, rsuId, resBlocks, appInfo_[appId].inputSize);
                    if (debugMode)
                    {
                        EV << "\t\tenumerate resBlocks " << resBlocks << ", offloadDelay: " << offloadDelay << "s" << endl;
                    }
                        
                    if (offloadDelay + offloadOverhead_ >= period)
                        continue;  // if the forwarding delay is too long, break

                    // enumerate all possible service types for the application
                    set<string> serviceTypes = db_->getGnbServiceTypes();
                    for (const string& serviceType : serviceTypes)
                    {
                        for (int cmpUnits = 1; cmpUnits <= maxCU; cmpUnits += cuStep_)
                        {
                            double exeDelay = computeExeDelay(rsuId, cmpUnits, serviceType);
                            if (exeDelay < 0)
                                continue;  // if the execution delay cannot be computed due to invalid parameters, skip

                            if (debugMode)
                            {
                                EV << "\t\t\tservice type " << serviceType << ", minCU: " << cmpUnits << ", exeDelayThreshold: " << exeDelay << endl;
                                debugMode = false;  // only print once
                            }

                            if (offloadDelay + offloadOverhead_ + exeDelay > period)
                                continue;  // if the total execution and forwarding time is too long, skip

                            double utility = computeUtility(appId, serviceType) / period;   // utility per second
                            if (utility <= 0)   // if the saved energy is less than 0, skip
                                continue;

                            // AppInstance instance = {appIndex, offRsuIndex, resBlocks, cmpUnits, serviceType};
                            instAppIndex_.push_back(appIndex);
                            instOffRsuIndex_.push_back(rsuIndex);
                            instRBs_.push_back(resBlocks);
                            instCUs_.push_back(cmpUnits);
                            instUtility_.push_back(utility);  // energy savings for the instance
                            instMaxOffTime_.push_back(period - exeDelay - offloadOverhead_);  // maximum offloading time for the instance
                            instServiceType_.push_back(serviceType);  // selected service type for the instance
                            instExeDelay_.push_back(exeDelay);  // execution delay for the instance

                            double rbUtil = double(resBlocks) / maxRB;
                            double cuUtil = double(cmpUnits) / maxCU;
                            instMaxUtilization_.push_back(max(rbUtil, cuUtil));  // the maximum resource utilization for the instance
                            instUtilizationSum_.push_back(rbUtil + cuUtil);  // the sum of resource utilization for the instance
                            instPerApp_[appIndex].push_back(instIndex);  // add the instance index to the per-application instance vector
                            instPerRsu_[rsuIndex].push_back(instIndex);  // add the instance index to the per-RSU instance vector
                            instIndex++;  // increment the instance index counter
                        }
                    }
                }
            }
        }
    }
}


vector<srvInstance> AccuracyIDAssign::scheduleRequests()
{
    EV << NOW << " AccuracyIDAssign::scheduleRequests - ID Assign schedule scheme starts" << endl;

    if (appIds_.size() == 0) {
        EV << NOW << " AccuracyIDAssign::scheduleRequests - no applications to schedule" << endl;
        return {};
    }

    vector<double> instUtilityTemp = instUtility_;
    vector<int> candidateInstIdx;  // vector to store the indices of the candidate instances
    vector<srvInstance> solution;  // vector to store the solution set

    // create a vector of instance indices and sort it based on instMaxUtilization_ in ascending order
    vector<int> instIndices(instUtility_.size());
    iota(instIndices.begin(), instIndices.end(), 0);  // initialize the instance indices
    sort(instIndices.begin(), instIndices.end(), [this](int a, int b) {
        return instMaxUtilization_[a] < instMaxUtilization_[b];
    });  // sort the instance indices based on the maximum resource utilization in ascending order

    // select candidate instances based on the sorted instance indices
    for (int instIdx : instIndices)
    {
        if (instUtilityTemp[instIdx] <= 0)
            continue;  // skip if the utility is less than or equal to 0

        candidateInstIdx.push_back(instIdx);

        // update the utility of the remaining instances
        int appIndex = instAppIndex_[instIdx];  // get the application index
        int rsuIndex = instOffRsuIndex_[instIdx];  // get the RSU index
        double utility = instUtilityTemp[instIdx];  // get the utility of the instance
        for (int idx : instPerApp_[appIndex])  // update the utility of the instances for the same application
        {
            if (instUtilityTemp[idx] <= 0)
                continue;  // skip if the instance has no utility
            
            instUtilityTemp[idx] -= utility;  // reduce the utility by the selected instance's utility
        }
        for (int idx : instPerRsu_[rsuIndex])  // update the utility of the instances for the same RSU
        {
            if (instAppIndex_[idx] == appIndex || instUtilityTemp[idx] <= 0)
                continue;  // skip if the instance belongs to the same application or has no utility

            instUtilityTemp[idx] -= utility * instUtilizationSum_[idx];
        }
    }

    // enumerate the candidate instances in reverse order to select the instances
    set<int> selectedApps;  // set to store the selected application indices
    for (int i = candidateInstIdx.size() - 1; i >= 0; i--) {
        int instIdx = candidateInstIdx[i];  // get the instance index
        int appIndex = instAppIndex_[instIdx];  // get the application index
        int rsuIndex = instOffRsuIndex_[instIdx];  // get the RSU index
        if (selectedApps.find(appIndex) != selectedApps.end())  // if the application is already selected, skip
            continue;
        
        // check if the RSU has enough resources
        if (rsuRBs_[rsuIndex] < instRBs_[instIdx] || rsuCUs_[rsuIndex] < instCUs_[instIdx])
            continue;  // skip if the RSU does not have enough resources

        // add the instance to the solution set, (appId, offloading rsuId, processing rsuId, bands, cmpUnits)
        solution.emplace_back(appIds_[appIndex], rsuIds_[rsuIndex], rsuIds_[rsuIndex], instRBs_[instIdx], instCUs_[instIdx]);
        selectedApps.insert(appIndex);  // mark the application as selected

        appMaxOffTime_[appIds_[appIndex]] = instMaxOffTime_[instIdx];  // store the maximum offloading time for the application
        appUtility_[appIds_[appIndex]] = instUtility_[instIdx];  // store the utility for the application
        appExeDelay_[appIds_[appIndex]] = instExeDelay_[instIdx];  // store the execution delay for the application
        appServiceType_[appIds_[appIndex]] = instServiceType_[instIdx];  // store the service type for the application

        // update the RSU status
        rsuRBs_[rsuIndex] -= instRBs_[instIdx];
        rsuCUs_[rsuIndex] -= instCUs_[instIdx];
    }

    EV << NOW << " AccuracyIDAssign::scheduleRequests - IDAssign schedule scheme ends, selected " 
       << solution.size() << " instances from "<< instAppIndex_.size() << " total instances" << endl;

    return solution;  // return the solution set
}
