//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    AccuracyFastIS.cc / AccuracyFastIS.h
//
//  Description:
//    This file implements the equivalently linear time approximation scheduling scheme 
//    in the Mobile Edge Computing System with backhaul network support.
//    In this scheme, we classify the service instances into four types:
//        LI: service instances light in terms of RB and CU (half or less of the available resources)
//        HI: service instances heavy in terms of RB or CU (more than half of the available resources)
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2026-03-20
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/apps/scheduler/accuracyNF/AccuracyFastIS.h"

AccuracyFastIS::AccuracyFastIS(Scheduler *scheduler)
    : AccuracyGreedy(scheduler)
{
    EV << NOW << " AccuracyFastIS::AccuracyFastIS - Initialized" << endl;
}


void AccuracyFastIS::initializeData()
{
    EV << NOW << " AccuracyFastIS::initializeData - Initializing scheduling data" << endl;

    AccuracyGreedy::initializeData();
    
    instCategory_.clear();
    instUtilizationSum_.clear();
}


void AccuracyFastIS::generateScheduleInstances()
{
    initializeData();  // transform the scheduling data
    EV << NOW << " AccuracyFastIS::generateScheduleInstances - Generating schedule instances" << endl;

    bool debugMode = false;

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
                // if maxRB/rbStep_ is smaller than maxCU/cuStep_, enumerate RB
                if (maxRB / rbStep_ < maxCU / cuStep_)
                {
                    for (int resBlocks = 1; resBlocks <= maxRB; resBlocks += rbStep_)
                    {
                        double offloadDelay = computeOffloadDelay(vehId, rsuId, resBlocks, appInfo_[appId].inputSize);
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
                            int minCU = computeMinRequiredCUs(rsuId, exeDelayThreshold, serviceType);
                            if (debugMode)
                            {
                                EV << "\t\t\tservice type " << serviceType << ", minCU: " << minCU << ", exeDelayThreshold: " << exeDelayThreshold << endl;
                                debugMode = false;  // only print once
                            }

                            if (minCU > maxCU)
                                continue;  // if the minimum computing units required is larger than the maximum computing units available, skip

                            double exeDelay = computeExeDelay(rsuId, minCU, serviceType);
                            double utility = computeUtility(appId, serviceType) / period;   // utility per second
                            if (utility <= 0)   // if the saved energy is less than 0, skip
                                continue;

                            // AppInstance instance = {appIndex, offRsuIndex, resBlocks, cmpUnits, serviceType};
                            instAppIndex_.push_back(appIndex);
                            instOffRsuIndex_.push_back(rsuIndex);
                            instRBs_.push_back(resBlocks);
                            instCUs_.push_back(minCU);
                            instUtility_.push_back(utility);  // energy savings for the instance
                            instMaxOffTime_.push_back(period - exeDelay - offloadOverhead_);  // maximum offloading time for the instance
                            instServiceType_.push_back(serviceType);  // selected service type for the instance
                            instExeDelay_.push_back(exeDelay);  // execution delay for the instance
                            double utilizationSum = double(resBlocks) / maxRB + double(minCU) / maxCU;  // sum of resource utilization for the instance
                            instUtilizationSum_.push_back(utilizationSum);  // store the sum of resource

                            // define category for the instance
                            bool isLightRB = (resBlocks * 2 <= maxRB);
                            bool isLightCU = (minCU * 2 <= maxCU);
                            if (isLightRB && isLightCU)
                                instCategory_.push_back("LI");
                            else
                                instCategory_.push_back("HI");
                        }
                    }
                }
                else    // else enumerate CUs
                {
                    // enumerate all possible service types for the application
                    set<string> serviceTypes = db_->getGnbServiceTypes();
                    for (const string& serviceType : serviceTypes)
                    {
                        for (int cmpUnits = 1; cmpUnits <= maxCU; cmpUnits += cuStep_)
                        {
                            double exeDelay = computeExeDelay(rsuId, cmpUnits, serviceType);
                            if (exeDelay + offloadOverhead_ >= period)
                                continue;  // if the total execution and forwarding time is too long, skip

                            // determine the smallest resource blocks required to meet the deadline
                            double offloadTimeThreshold = period - exeDelay - offloadOverhead_;
                            int minRB = computeMinRequiredRBs(vehId, rsuId, offloadTimeThreshold, appInfo_[appId].inputSize);
                            if (minRB > maxRB)
                                continue;  // if the minimum resource blocks required is larger than the maximum resource blocks available, continue

                            double utility = computeUtility(appId, serviceType) / period;   // utility per second
                            if (utility <= 0)   // if the saved energy is less than 0, skip
                                continue;

                            // AppInstance instance = {appIndex, offRsuIndex, resBlocks, cmpUnits, serviceType};
                            instAppIndex_.push_back(appIndex);
                            instOffRsuIndex_.push_back(rsuIndex);
                            instRBs_.push_back(minRB);
                            instCUs_.push_back(cmpUnits);
                            instUtility_.push_back(utility);  // energy savings for the instance
                            instMaxOffTime_.push_back(offloadTimeThreshold);  // maximum offloading time for the instance
                            instServiceType_.push_back(serviceType);  // selected service type for the instance
                            instExeDelay_.push_back(exeDelay);  // execution delay for the instance
                            double utilizationSum = double(minRB) / maxRB + double(cmpUnits) / maxCU;  // sum of resource utilization for the instance
                            instUtilizationSum_.push_back(utilizationSum);  // store the sum of resource utilization for the instance

                            // define category for the instance
                            bool isLightRB = (minRB * 2 <= maxRB);
                            bool isLightCU = (cmpUnits * 2 <= maxCU);
                            if (isLightRB && isLightCU)
                                instCategory_.push_back("LI");
                            else
                                instCategory_.push_back("HI");
                        }
                    }
                }
            }
        }
    }
}


vector<srvInstance> AccuracyFastIS::scheduleRequests()
{
    EV << NOW << " AccuracyFastIS::scheduleRequests - FastIS schedule scheme starts" << endl;

    if (appIds_.empty()) {
        EV << NOW << " AccuracyFastIS::scheduleRequests - No applications to schedule" << endl;
        return {};
    }

    vector<int> solutionIndices;  // vectors to store the indices of the instances

    solutionGeneration(solutionIndices);  // generate the solution for the specified type

    // construct the final solution based on the selected indices
    vector<srvInstance> solution;  // vectors to store the solutions
    for (int instIdx : solutionIndices) 
    {
        int appIndex = instAppIndex_[instIdx];  // get the application index
        int rsuIndex = instOffRsuIndex_[instIdx];  // get the offload RSU index

        // add the instance to the solution set
        // typedef tuple<AppId, MacNodeId, MacNodeId, int, int> srvInstance;
        solution.emplace_back(appIds_[appIndex], rsuIds_[rsuIndex], rsuIds_[rsuIndex], instRBs_[instIdx], instCUs_[instIdx]);
        appMaxOffTime_[appIds_[appIndex]] = instMaxOffTime_[instIdx];  // store the maximum offloading time for the application
        appUtility_[appIds_[appIndex]] = instUtility_[instIdx];  // store the utility for the application
        appExeDelay_[appIds_[appIndex]] = instExeDelay_[instIdx];  // store the execution delay for the application
        appServiceType_[appIds_[appIndex]] = instServiceType_[instIdx];  // store the service type for the application
    }

    EV << NOW << " AccuracyFastIS::scheduleRequests - FastIS schedule scheme ends, selected " << solution.size() 
        << " service instances from " << instAppIndex_.size() << " total instances" << endl;

    return solution;
}


void AccuracyFastIS::solutionGeneration(vector<int>& instIndices)
{   
    // define five assistant vectors
    vector<double> reductPerAppIndex(appIds_.size(), 0.0);  // vector to store the reduction of utility for each application
    vector<double> reductPerRsuIndex(rsuIds_.size(), 0.0);  // vector to store the reduction of utility for each RSU
    // vector to store the reduction of utility for each application in each RSU in terms of RB
    vector<vector<double>> reductAppInRsu(appIds_.size(), vector<double>(rsuIds_.size(), 0.0));
    /***
     * only consider service instances of the targeted types
     */
    vector<int> candidateInstIdx;  // vector to store the indices of the candidate instances

    const vector<string> serviceType = {"LI", "HI"};
    for (const string& srvType : serviceType)
    {
        for (int instIdx = 0; instIdx < instAppIndex_.size(); instIdx++) {
            if (instCategory_[instIdx] != srvType)
                continue;

            int appIndex = instAppIndex_[instIdx];  // get the application index
            int rsuIndex = instOffRsuIndex_[instIdx];  // get the offload RSU index

            // check the updated utility
            double redApp = reductPerAppIndex[appIndex];  // reduction of utility for the application
            // reduction of utility for the RSU
            double redRsu = reductPerRsuIndex[rsuIndex] - reductAppInRsu[appIndex][rsuIndex];

            double utility = instUtility_[instIdx] - redApp - 2 * redRsu * instUtilizationSum_[instIdx];  // updated utility
            
            if (utility <= 0)
                continue;  // skip if the updated utility is less than or equal to 0

            // append the instance to the candidate vector, and update the reduction of utility
            candidateInstIdx.push_back(instIdx);
            reductPerAppIndex[appIndex] += utility;  // update the reduction of utility for the application
            reductPerRsuIndex[rsuIndex] += utility;  // update the reduction of utility for the offloading RSU
            reductAppInRsu[appIndex][rsuIndex] += utility;  // update the reduction of utility for the application in the offloading RSU
        }
    }

    // make a copy of vec resources
    vector<int> rsuRBsCopy = rsuRBs_;
    vector<int> rsuCUsCopy = rsuCUs_;
    /***
     * enumerate the candidate instances in reverse order to select the instances
     */
    instIndices.clear();  // clear the instance indices vector
    set<int> selectedApps;  // set to store the selected application indices
    for (int i = candidateInstIdx.size() - 1; i >= 0; i--) {
        int instIdx = candidateInstIdx[i];  // get the instance index
        int appIndex = instAppIndex_[instIdx];  // get the application index
        
        // if the application is already selected, skip
        if (selectedApps.find(appIndex) != selectedApps.end())  
            continue;

        int rsuIndex = instOffRsuIndex_[instIdx];  // get the offload RSU index
        
        // check if the RSU has enough resources
        if (rsuRBsCopy[rsuIndex] < instRBs_[instIdx] || rsuCUsCopy[rsuIndex] < instCUs_[instIdx])
            continue;  // skip if the RSU does not have enough resources

        // add the instance to the solution set
        instIndices.push_back(instIdx);  // add the instance index to the indices vector
        selectedApps.insert(appIndex);  // mark the application as selected

        // update the RSU status
        rsuRBsCopy[rsuIndex] -= instRBs_[instIdx];
        rsuCUsCopy[rsuIndex] -= instCUs_[instIdx];
    }
}

