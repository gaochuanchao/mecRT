//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    AccuracyFastSANF.cc / AccuracyFastSANF.h
//
//  Description:
//    This file implements the variance of FastSA without considering data forwarding in the backhaul network
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//


#include "mecrt/apps/scheduler/accuracy/AccuracyFastSANF.h"

AccuracyFastSANF::AccuracyFastSANF(Scheduler *scheduler)
    : AccuracyGreedy(scheduler)
{
    EV << NOW << " AccuracyFastSANF::AccuracyFastSANF - Initialized" << endl;
}


void AccuracyFastSANF::generateScheduleInstances()
{
    EV << NOW << " AccuracyFastSANF::generateScheduleInstances - Generating schedule instances" << endl;

    initializeData();  // transform the scheduling data

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
            
            for(MacNodeId offRsuId : vehAccessRsu_[vehId])   // enumerate the RSUs in access
            {
                if (rsuStatus_.find(offRsuId) == rsuStatus_.end())
                    continue;  // if not found, skip

                int offRsuIndex = rsuId2Index_[offRsuId];  // get the index of the RSU in the rsuIds vector
                if (rsuRBs_[offRsuIndex] <= 0 || rsuCUs_[offRsuIndex] <= 0)
                    continue;  // if there is no resource blocks available, skip

                int maxRB = floor(rsuRBs_[offRsuIndex] * fairFactor_);  // maximum resource blocks for the offload RSU
                int maxCU = floor(rsuCUs_[offRsuIndex] * fairFactor_);  // maximum computing units for the processing RSU
                double fwdDelay = 0;  // no data forwarding delay in this scheme
                if (debugMode)
                    EV << "\t period: " << period << ", offload RSU " << offRsuId << " to process RSU " << offRsuId
                        << " (maxRB: " << maxRB << ", maxCU: " << maxCU << ", fwdDelay: " << fwdDelay << "s)" << endl;
                // if maxRB/rbStep_ is smaller than maxCU/cuStep_, enumerate RB
                if (maxRB / rbStep_ < maxCU / cuStep_)
                {
                    for (int resBlocks = maxRB; resBlocks > 0; resBlocks -= rbStep_)
                    {
                        double offloadDelay = computeOffloadDelay(vehId, offRsuId, resBlocks, appInfo_[appId].inputSize);
                        if (debugMode)
                        {
                            EV << "\t\tenumerate resBlocks " << resBlocks << ", offloadDelay: " << offloadDelay << "s" << endl;
                        }
                            
                        if (fwdDelay + offloadDelay + offloadOverhead_ >= period)
                            break;  // if the forwarding delay is too long, break

                        double exeDelayThreshold = period - offloadDelay - fwdDelay - offloadOverhead_;
                        // enumerate all possible service types for the application
                        set<string> serviceTypes = db_->getGnbServiceTypes();
                        for (const string& serviceType : serviceTypes)
                        {
                            int minCU = computeMinRequiredCUs(offRsuId, exeDelayThreshold, serviceType);
                            if (debugMode)
                            {
                                EV << "\t\t\tservice type " << serviceType << ", minCU: " << minCU << ", exeDelayThreshold: " << exeDelayThreshold << endl;
                                debugMode = false;  // only print once
                            }

                            if (minCU > maxCU)
                                continue;  // if the minimum computing units required is larger than the maximum computing units available, skip

                            double exeDelay = computeExeDelay(offRsuId, minCU, serviceType);
                            double utility = computeUtility(appId, serviceType) / period;   // utility per second
                            if (utility <= 0)   // if the saved energy is less than 0, skip
                                continue;

                            // AppInstance instance = {appIndex, offRsuIndex, procRsuIndex, resBlocks, cmpUnits};
                            instAppIndex_.push_back(appIndex);
                            instOffRsuIndex_.push_back(offRsuIndex);
                            instProRsuIndex_.push_back(offRsuIndex);
                            instRBs_.push_back(resBlocks);
                            instCUs_.push_back(minCU);
                            instUtility_.push_back(utility);  // energy savings for the instance
                            instMaxOffTime_.push_back(period - fwdDelay - exeDelay - offloadOverhead_);  // maximum offloading time for the instance
                            instServiceType_.push_back(serviceType);  // selected service type for the instance
                            instExeDelay_.push_back(exeDelay);  // execution delay for the instance
                        }
                    }
                }
                else    // else enumerate CUs
                {
                    // enumerate all possible service types for the application
                    set<string> serviceTypes = db_->getGnbServiceTypes();
                    for (const string& serviceType : serviceTypes)
                    {
                        for (int cmpUnits = maxCU; cmpUnits > 0; cmpUnits -= cuStep_)
                        {
                            double exeDelay = computeExeDelay(offRsuId, cmpUnits, serviceType);
                            if (exeDelay + fwdDelay + offloadOverhead_ >= period)
                                break;  // if the total execution and forwarding time is too long, skip

                            // determine the smallest resource blocks required to meet the deadline
                            double offloadTimeThreshold = period - exeDelay - fwdDelay - offloadOverhead_;
                            int minRB = computeMinRequiredRBs(vehId, offRsuId, offloadTimeThreshold, appInfo_[appId].inputSize);
                            if (minRB > maxRB)
                                break;  // if the minimum resource blocks required is larger than the maximum resource blocks available, break

                            double utility = computeUtility(appId, serviceType) / period;   // utility per second
                            if (utility <= 0)   // if the saved energy is less than 0, skip
                                continue;

                            // AppInstance instance = {appIndex, offRsuIndex, procRsuIndex, resBlocks, cmpUnits, serviceType};
                            instAppIndex_.push_back(appIndex);
                            instOffRsuIndex_.push_back(offRsuIndex);
                            instProRsuIndex_.push_back(offRsuIndex);
                            instRBs_.push_back(minRB);
                            instCUs_.push_back(cmpUnits);
                            instUtility_.push_back(utility);  // energy savings for the instance
                            instMaxOffTime_.push_back(offloadTimeThreshold);  // maximum offloading time for the instance
                            instServiceType_.push_back(serviceType);  // selected service type for the instance
                            instExeDelay_.push_back(exeDelay);  // execution delay for the instance
                        }
                    }
                }
                
            }
        }
    }
}


vector<srvInstance> AccuracyFastSANF::scheduleRequests()
{
    EV << NOW << " AccuracyFastSANF::scheduleRequests - FastSA schedule scheme starts" << endl;

    if (appIds_.empty()) {
        EV << NOW << " AccuracyFastSANF::scheduleRequests - No applications to schedule" << endl;
        return {};
    }

    vector<int> instIndicesOne, instIndicesTwo, solutionIndices;  // vectors to store the indices of the instances
    double utilityOne, utilityTwo;

    defineInstanceCategory();  // define the instance categories based on resource utilization
    candidateGenerateForType({"LL", "HL", "HH"}, instIndicesOne, utilityOne);  // generate candidates for the specified type
    candidateGenerateForType({"LH"}, instIndicesTwo, utilityTwo);  // generate candidates for the specified type

    // compare the two solutions and choose the one with higher utility
    if (utilityOne >= utilityTwo) 
        solutionIndices = instIndicesOne;  // choose the first solution if it has higher or equal utility
    else 
        solutionIndices = instIndicesTwo;  // choose the second solution if it has higher utility

    // construct the final solution based on the selected indices
    vector<srvInstance> solution;  // vectors to store the solutions
    for (int instIdx : solutionIndices) 
    {
        int appIndex = instAppIndex_[instIdx];  // get the application index
        int offRsuIndex = instOffRsuIndex_[instIdx];  // get the offload RSU index
        int proRsuIndex = instProRsuIndex_[instIdx];  // get the processing RSU index

        // add the instance to the solution set
        solution.emplace_back(appIds_[appIndex], rsuIds_[offRsuIndex], rsuIds_[proRsuIndex], instRBs_[instIdx], instCUs_[instIdx]);
        appMaxOffTime_[appIds_[appIndex]] = instMaxOffTime_[instIdx];  // store the maximum offloading time for the application
        appUtility_[appIds_[appIndex]] = instUtility_[instIdx];  // store the utility for the application
        appExeDelay_[appIds_[appIndex]] = instExeDelay_[instIdx];  // store the execution delay for the application
        appServiceType_[appIds_[appIndex]] = instServiceType_[instIdx];  // store the service type for the application
    }

    EV << NOW << " AccuracyFastSANF::scheduleRequests - FastSA schedule scheme ends, selected " << solution.size() 
        << " service instances from " << instAppIndex_.size() << " total instances" << endl;

    return solution;
}


void AccuracyFastSANF::defineInstanceCategory()
{
    int totalInstances = instAppIndex_.size();
    instCategory_.clear();
    rbUtilization_.clear();
    cuUtilization_.clear();
    
    instCategory_.reserve(totalInstances);
    rbUtilization_.reserve(totalInstances);
    cuUtilization_.reserve(totalInstances);

    for (int instIdx = 0; instIdx < totalInstances; instIdx++) {
        int offRsuIndex = instOffRsuIndex_[instIdx];  // get the offload RSU index
        int proRsuIndex = instProRsuIndex_[instIdx];  // get the processing RSU index
        double rb = instRBs_[instIdx];  // get the resource blocks
        double cu = instCUs_[instIdx];  // get the computing units

        rbUtilization_.push_back(rb / rsuRBs_[offRsuIndex]);
        cuUtilization_.push_back(cu / rsuCUs_[proRsuIndex]);

        bool isLightRB = (rb * 2 <= rsuRBs_[offRsuIndex]);
        bool isLightCU = (cu * 2 <= rsuCUs_[proRsuIndex]);

        if (isLightRB && isLightCU)
            instCategory_.push_back("LL");
        else if (isLightRB && !isLightCU)
            instCategory_.push_back("LH");
        else if (!isLightRB && isLightCU)
            instCategory_.push_back("HL");
        else
            instCategory_.push_back("HH");
    }
}


void AccuracyFastSANF::candidateGenerateForType(const vector<string> serviceTypes, vector<int>& instIndices, double& totalUtility)
{
    if (serviceTypes.empty()) {
        EV << NOW << " AccuracyFastSANF::candidateGenerateForType - Invalid service types" << endl;
        return;  // invalid service types
    }
    
    // define five assistant vectors
    vector<double> reductPerAppIndex(appIds_.size(), 0.0);  // vector to store the reduction of utility for each application
    vector<double> reductRbPerRsuIndex(rsuIds_.size(), 0.0);  // vector to store the reduction of utility for each RSU in terms of RB
    vector<double> reductCuPerRsuIndex(rsuIds_.size(), 0.0);  // vector to store the reduction of utility for each RSU in terms of CU
    // vector to store the reduction of utility for each application in each RSU in terms of RB
    vector<vector<double>> reductRbAppInRsu(appIds_.size(), vector<double>(rsuIds_.size(), 0.0));
    // vector to store the reduction of utility for each application in each RSU in terms of CU
    vector<vector<double>> reductCuAppInRsu(appIds_.size(), vector<double>(rsuIds_.size(), 0.0));
    /***
     * only consider service instances of the targeted types
     */
    vector<int> candidateInstIdx;  // vector to store the indices of the candidate instances
    for (const string& srvType : serviceTypes) {
        if (srvType != "LL" && srvType != "LH" && srvType != "HL" && srvType != "HH") {
            stringstream ss;
            ss << NOW << " AccuracyFastSANF::candidateGenerateForType - Invalid service type: " << srvType << endl;
            throw invalid_argument(ss.str());
        }

        for (int instIdx = 0; instIdx < instAppIndex_.size(); instIdx++) {
            int appIndex = instAppIndex_[instIdx];  // get the application index
            int offRsuIndex = instOffRsuIndex_[instIdx];  // get the offload RSU index
            int proRsuIndex = instProRsuIndex_[instIdx];  // get the processing RSU index

            if (instCategory_[instIdx] != srvType)
                continue;

            double rbUtil = rbUtilization_[instIdx];  // RB utilization
            double cuUtil = cuUtilization_[instIdx];  // CU utilization
            // check the updated utility
            double redApp = reductPerAppIndex[appIndex];  // reduction of utility for the application
            // reduction of utility for the RSU
            double redOffRsu = reductRbPerRsuIndex[offRsuIndex] - reductRbAppInRsu[appIndex][offRsuIndex];
            // reduction of utility for the RSU
            double redProRsu = reductCuPerRsuIndex[proRsuIndex] - reductCuAppInRsu[appIndex][proRsuIndex];

            double utility = instUtility_[instIdx] - redApp - 2 * redOffRsu * rbUtil - 2 * redProRsu * cuUtil;  // updated utility
            
            if (utility <= 0)
                continue;  // skip if the updated utility is less than or equal to 0

            // append the instance to the candidate vector, and update the reduction of utility
            candidateInstIdx.push_back(instIdx);
            reductPerAppIndex[appIndex] += utility;  // update the reduction of utility for the application
            reductRbPerRsuIndex[offRsuIndex] += utility;  // update the reduction of utility for the offloading RSU
            reductRbAppInRsu[appIndex][offRsuIndex] += utility;  // update the reduction of utility for the application in the offloading RSU
            reductCuPerRsuIndex[proRsuIndex] += utility;  // update the reduction of utility for the processing RSU
            reductCuAppInRsu[appIndex][proRsuIndex] += utility;  // update the reduction of utility for the application in the processing RSU
        }
    }
    

    // make a copy of vec resources
    vector<int> rsuRBsCopy = rsuRBs_;
    vector<int> rsuCUsCopy = rsuCUs_;
    /***
     * enumerate the candidate instances in reverse order to select the instances
     */
    totalUtility = 0;  // initialize the total utility
    instIndices.clear();  // clear the instance indices vector
    set<int> selectedApps;  // set to store the selected application indices
    for (int i = candidateInstIdx.size() - 1; i >= 0; i--) {
        int instIdx = candidateInstIdx[i];  // get the instance index
        int appIndex = instAppIndex_[instIdx];  // get the application index
        if (selectedApps.find(appIndex) != selectedApps.end())  // if the application is already selected, skip
            continue;

        int offRsuIndex = instOffRsuIndex_[instIdx];  // get the offload RSU index
        int proRsuIndex = instProRsuIndex_[instIdx];  // get the processing RSU index
        
        // check if the RSU has enough resources
        if (rsuRBsCopy[offRsuIndex] < instRBs_[instIdx] || rsuCUsCopy[proRsuIndex] < instCUs_[instIdx])
            continue;  // skip if the RSU does not have enough resources

        // add the instance to the solution set
        instIndices.push_back(instIdx);  // add the instance index to the indices vector
        selectedApps.insert(appIndex);  // mark the application as selected
        totalUtility += instUtility_[instIdx];  // accumulate the utility for the selected instance

        // update the RSU status
        rsuRBsCopy[offRsuIndex] -= instRBs_[instIdx];
        rsuCUsCopy[proRsuIndex] -= instCUs_[instIdx];
    }
}
