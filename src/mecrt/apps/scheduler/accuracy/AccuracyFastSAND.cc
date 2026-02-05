//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    AccuracyFastSAND.cc / AccuracyFastSAND.h
//
//  Description:
//    This file implements the equivalently linear time approximation scheduling scheme 
//    in the Mobile Edge Computing System with backhaul network support.
//    In this scheme, we classify the service instances into four types:
//        LL: service instances light in terms of RB and CU (half or less of the available resources)
//        LH: service instances light in terms of RB but heavy in terms of CU
//        HL: service instances heavy in terms of RB but light in terms of CU
//        HH: service instances heavy in terms of RB and CU (more than half of the available resources)
//    either type LH or type HL will be considered separately
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/apps/scheduler/accuracy/AccuracyFastSAND.h"

AccuracyFastSAND::AccuracyFastSAND(Scheduler *scheduler)
    : AccuracyGreedy(scheduler)
{
    EV << NOW << " AccuracyFastSAND::AccuracyFastSAND - Initialized" << endl;
}


vector<srvInstance> AccuracyFastSAND::scheduleRequests()
{
    EV << NOW << " AccuracyFastSAND::scheduleRequests - FastSAND schedule scheme starts" << endl;

    if (appIds_.empty()) {
        EV << NOW << " AccuracyFastSAND::scheduleRequests - No applications to schedule" << endl;
        return {};
    }

    vector<int> solutionIndices;  // vectors to store the indices of the instances

    defineInstanceCategory();  // define the instance categories based on resource utilization
    candidateGenerateForType({"LL", "LH", "HL", "HH"}, solutionIndices);  // generate candidates for the specified type

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

    EV << NOW << " AccuracyFastSAND::scheduleRequests - FastSAND schedule scheme ends, selected " << solution.size() 
        << " service instances from " << instAppIndex_.size() << " total instances" << endl;

    return solution;
}


void AccuracyFastSAND::defineInstanceCategory()
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


void AccuracyFastSAND::candidateGenerateForType(const vector<string> serviceTypes, vector<int>& instIndices)
{
    if (serviceTypes.empty()) {
        EV << NOW << " AccuracyFastSAND::candidateGenerateForType - Invalid service types" << endl;
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
            ss << NOW << " AccuracyFastSAND::candidateGenerateForType - Invalid service type: " << srvType << endl;
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

        // update the RSU status
        rsuRBsCopy[offRsuIndex] -= instRBs_[instIdx];
        rsuCUsCopy[proRsuIndex] -= instCUs_[instIdx];
    }
}
