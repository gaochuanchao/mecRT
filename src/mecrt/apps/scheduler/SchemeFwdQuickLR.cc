//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    SchemeFwdQuickLR.cc / SchemeFwdQuickLR.h
//
//  Description:
//    This file implements the equivalently linear time approximation scheduling scheme 
//    in the Mobile Edge Computing System with backhaul network support.
//    In this scheme, we classify the service instances into four types:
//        0: service instances light in terms of RB and CU (half or less of the available resources)
//        1: service instances light in terms of RB but heavy in terms of CU
//        2: service instances heavy in terms of RB but light in terms of CU
//        3: service instances heavy in terms of RB and CU (more than half of the available resources)
//    either type 1 or type 2 will be considered separately
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/apps/scheduler/SchemeFwdQuickLR.h"

SchemeFwdQuickLR::SchemeFwdQuickLR(Scheduler *scheduler)
    : SchemeFwdBase(scheduler)
{
    separateInstType_ = 1;  // default is to separate type 1 instances, light in terms of RB but heavy in terms of CU
    EV << NOW << " SchemeFwdQuickLR::SchemeFwdQuickLR - Initialized" << endl;
}

vector<srvInstance> SchemeFwdQuickLR::scheduleRequests()
{
    EV << NOW << " SchemeFwdQuickLR::scheduleRequests - QuickLR schedule scheme starts" << endl;

    if (appIds_.empty()) {
        EV << NOW << " SchemeFwdQuickLR::scheduleRequests - No applications to schedule" << endl;
        return {};
    }

    vector<int> instIndicesOne, instIndicesTwo, solutionIndices;  // vectors to store the indices of the instances
    double utilityOne, utilityTwo;
    candidateGenerateExcludeType(separateInstType_, instIndicesOne, utilityOne);  // generate candidates excluding the specified type
    candidateGenerateForType(separateInstType_, instIndicesTwo, utilityTwo);  // generate candidates for the specified type

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
    }

    EV << NOW << " SchemeFwdQuickLR::scheduleRequests - QuickLR schedule scheme ends, selected " << solution.size() 
        << " service instances from " << instAppIndex_.size() << " total instances" << endl;

    return solution;
}


void SchemeFwdQuickLR::candidateGenerateExcludeType(int instanceType, vector<int>& instIndices, double& totalUtility)
{
    // define five assistant vectors
    vector<double> reductPerAppIndex(appIds_.size(), 0.0);  // vector to store the reduction of utility for each application
    vector<double> reductRbPerRsuIndex(rsuIds_.size(), 0.0);  // vector to store the reduction of utility for each RSU in terms of RB
    vector<double> reductCuPerRsuIndex(rsuIds_.size(), 0.0);  // vector to store the reduction of utility for each RSU in terms of CU
    // vector to store the reduction of utility for each application in each RSU in terms of RB
    vector<vector<double>> reductRbAppInRsu(appIds_.size(), vector<double>(rsuIds_.size(), 0.0));
    // vector to store the reduction of utility for each application in each RSU in terms of CU
    vector<vector<double>> reductCuAppInRsu(appIds_.size(), vector<double>(rsuIds_.size(), 0.0));

    // make a copy of vec resources
    vector<int> rsuRBsCopy = rsuRBs_;
    vector<int> rsuCUsCopy = rsuCUs_;

    vector<int> candidateInstIdx;  // vector to store the indices of the candidate instances
    for (int instType = 0; instType < 4; instType++) {
        if (instType == instanceType)
            continue;  // skip the instance type that is excluded

        for (int instIdx = 0; instIdx < instAppIndex_.size(); instIdx++) {
            int appIndex = instAppIndex_[instIdx];  // get the application index
            int offRsuIndex = instOffRsuIndex_[instIdx];  // get the offload RSU index
            int proRsuIndex = instProRsuIndex_[instIdx];  // get the processing RSU index
            double rb = instRBs_[instIdx];  // get the resource blocks
            double cu = instCUs_[instIdx];  // get the computing units

            if (instType == 0) {  // only consider service instances light in terms of RB and CU
                if (rb * 2 > rsuRBsCopy[offRsuIndex] || cu * 2 > rsuCUsCopy[proRsuIndex])
                    continue;
            }
            else if (instType == 1) {  // only consider service instances light in terms of RB but heavy in terms of CU
                if (!(rb * 2 <= rsuRBsCopy[offRsuIndex] && cu * 2 > rsuCUsCopy[proRsuIndex]))
                    continue;
            }
            else if (instType == 2) {  // only consider service instances heavy in terms of RB but light in terms of CU
                if (!(rb * 2 > rsuRBsCopy[offRsuIndex] && cu * 2 <= rsuCUsCopy[proRsuIndex]))
                    continue;
            }
            else if (instType == 3) {  // only consider service instances heavy in terms of RB and CU
                if (rb * 2 <= rsuRBsCopy[offRsuIndex] || cu * 2 <= rsuCUsCopy[proRsuIndex])
                    continue;
            }
                
            double rbUtil = rb / rsuRBsCopy[offRsuIndex];  // RB utilization
            double cuUtil = cu / rsuCUsCopy[proRsuIndex];  // CU utilization

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
        instIndices.push_back(instIdx);  // store the instance index
        selectedApps.insert(appIndex);  // mark the application as selected
        totalUtility += instUtility_[instIdx];  // accumulate the utility

        // update the RSU status
        rsuRBsCopy[offRsuIndex] -= instRBs_[instIdx];
        rsuCUsCopy[proRsuIndex] -= instCUs_[instIdx];
    }
}


void SchemeFwdQuickLR::candidateGenerateForType(int instanceType, vector<int>& instIndices, double& totalUtility)
{
    if (instanceType < 1 || instanceType > 2) {
        EV << NOW << " SchemeFwdQuickLR::candidateGenerateForType - Invalid instance type: " << instanceType << endl;
        return;  // invalid instance type
    }
    
    // define five assistant vectors
    vector<double> reductPerAppIndex(appIds_.size(), 0.0);  // vector to store the reduction of utility for each application
    vector<double> reductRbPerRsuIndex(rsuIds_.size(), 0.0);  // vector to store the reduction of utility for each RSU in terms of RB
    vector<double> reductCuPerRsuIndex(rsuIds_.size(), 0.0);  // vector to store the reduction of utility for each RSU in terms of CU
    // vector to store the reduction of utility for each application in each RSU in terms of RB
    vector<vector<double>> reductRbAppInRsu(appIds_.size(), vector<double>(rsuIds_.size(), 0.0));
    // vector to store the reduction of utility for each application in each RSU in terms of CU
    vector<vector<double>> reductCuAppInRsu(appIds_.size(), vector<double>(rsuIds_.size(), 0.0));

    // make a copy of vec resources
    vector<int> rsuRBsCopy = rsuRBs_;
    vector<int> rsuCUsCopy = rsuCUs_;

    /***
     * only consider service instances (1) light in terms of RB but heavy in terms of CU
     *      or (2) heavy in terms of RB but light in terms of CU
     */
    vector<int> candidateInstIdx;  // vector to store the indices of the candidate instances
    for (int instIdx = 0; instIdx < instAppIndex_.size(); instIdx++) {
        int appIndex = instAppIndex_[instIdx];  // get the application index
        int offRsuIndex = instOffRsuIndex_[instIdx];  // get the offload RSU index
        int proRsuIndex = instProRsuIndex_[instIdx];  // get the processing RSU index
        double rb = instRBs_[instIdx];  // get the resource blocks
        double cu = instCUs_[instIdx];  // get the computing units

        if (instanceType == 1) {  // only consider service instances light in terms of RB but heavy in terms of CU
            if (!(rb * 2 <= rsuRBsCopy[offRsuIndex] && cu * 2 > rsuCUsCopy[proRsuIndex]))
                continue;
        }
        else if (instanceType == 2) {  // only consider service instances heavy in terms of RB but light in terms of CU
            if (!(rb * 2 > rsuRBsCopy[offRsuIndex] && cu * 2 <= rsuCUsCopy[proRsuIndex]))
                continue;
        }

        // check the updated utility
        double redApp = reductPerAppIndex[appIndex];  // reduction of utility for the application
        // reduction of utility for the RSU
        double redOffRsu = reductRbPerRsuIndex[offRsuIndex] - reductRbAppInRsu[appIndex][offRsuIndex];
        // reduction of utility for the RSU
        double redProRsu = reductCuPerRsuIndex[proRsuIndex] - reductCuAppInRsu[appIndex][proRsuIndex];

        double utility = 0;
        if (instanceType == 1)  // service instances light in terms of RB but heavy in terms of CU
        {
            double rbUtil = rb / rsuRBsCopy[offRsuIndex];  // RB utilization
            utility = instUtility_[instIdx] - redApp - 2 * redOffRsu * rbUtil - redProRsu;  // updated utility
        }
        else if (instanceType == 2)  // service instances heavy in terms of RB but light in terms of CU
        {
            double cuUtil = cu / rsuCUsCopy[proRsuIndex];  // CU utilization
            utility = instUtility_[instIdx] - redApp - redOffRsu - 2 * redProRsu * cuUtil;  // updated utility
        }
        
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



