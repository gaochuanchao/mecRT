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

#include "mecrt/apps/scheduler/SchemeFastLR.h"

SchemeFastLR::SchemeFastLR(Scheduler *scheduler)
    : SchemeBase(scheduler)
{
    EV << NOW << " SchemeFastLR::SchemeFastLR - Initialized" << endl;
}

vector<srvInstance> SchemeFastLR::scheduleRequests()
{
    EV << NOW << " SchemeFastLR::scheduleRequests - FastLR schedule scheme starts" << endl;

    if (appIds_.size() == 0) {
        EV << NOW << " SchemeFastLR::scheduleRequests - no applications to schedule" << endl;
        return {};
    }

    vector<srvInstance> solution;  // vector to store the solution set
    // define three assistant vectors
    vector<double> reductPerAppIndex(appIds_.size(), 0.0);  // vector to store the reduction of utility for each application 
    vector<double> reductPerRsuIndex(rsuIds_.size(), 0.0);  // vector to store the reduction of utility for each RSU
    // vector to store the reduction of utility for each application in each RSU
    vector<vector<double>> reductAppInRsu(appIds_.size(), vector<double>(rsuIds_.size(), 0.0));  

    vector<int> candidateInstIdx;  // vector to store the indices of the candidate instances

    for (int instType = 0; instType < 2; instType++) {
        // instType = 0 for light service instances, instType = 1 for heavy service instances
        // if instType is 0, then we will enumerate the light service instances first
        // if instType is 1, then we will enumerate the heavy service instances first

        for (int instIdx = 0; instIdx < instAppIndex_.size(); instIdx++) {
            int appIndex = instAppIndex_[instIdx];  // get the application index
            int rsuIndex = instRsuIndex_[instIdx];  // get the RSU index
            double rb = instRBs_[instIdx];  // get the resource blocks
            double cu = instCUs_[instIdx];  // get the computing units

            if (instType == 0) {  // if instType is 0, then we will enumerate the light service instances
                if (rb * 2 > rsuRBs_[rsuIndex] || cu * 2 > rsuCUs_[rsuIndex])   // this is a heavy service instance
                    continue;  // skip if the instance is not light
            }
            else if (instType == 1) {  // if instType is 1, then we will enumerate the heavy service instances
                if (rb * 2 <= rsuRBs_[rsuIndex] || cu * 2 <= rsuCUs_[rsuIndex])     // this is a light service instance
                    continue;  // skip if the instance is not heavy
            }
                
            double rbUtil = rb / rsuRBs_[rsuIndex];  // RB utilization
            double cuUtil = cu / rsuCUs_[rsuIndex];  // CU utilization

            // check the updated utility
            double redApp = reductPerAppIndex[appIndex];  // reduction of utility for the application
            double redRsu = reductPerRsuIndex[rsuIndex] - reductAppInRsu[appIndex][rsuIndex];  // reduction of utility for the RSU
            double utility = instUtility_[instIdx] - redApp - redRsu * 2 * (rbUtil + cuUtil);  // updated utility
            if (utility <= 0)
                continue;  // skip if the updated utility is less than or equal to 0

            // append the instance to the candidate vector, and update the reduction of utility
            candidateInstIdx.push_back(instIdx);
            reductPerAppIndex[appIndex] += utility;  // update the reduction of utility for the application
            reductPerRsuIndex[rsuIndex] += utility;  // update the reduction of utility for the RSU
            reductAppInRsu[appIndex][rsuIndex] += utility;  // update the reduction of utility for the application in the RSU
        }
    }

    // enumerate the candidate instances in reverse order to select the instances
    set<int> selectedApps;  // set to store the selected application indices
    for (int i = candidateInstIdx.size() - 1; i >= 0; i--) {
        int instIdx = candidateInstIdx[i];  // get the instance index
        int appIndex = instAppIndex_[instIdx];  // get the application index
        int rsuIndex = instRsuIndex_[instIdx];  // get the RSU index
        if (selectedApps.find(appIndex) != selectedApps.end())  // if the application is already selected, skip
            continue;
        
        // check if the RSU has enough resources
        if (rsuRBs_[rsuIndex] < instRBs_[instIdx] || rsuCUs_[rsuIndex] < instCUs_[instIdx])
            continue;  // skip if the RSU does not have enough resources

        // add the instance to the solution set
        solution.emplace_back(appIds_[appIndex], rsuIds_[rsuIndex], rsuIds_[rsuIndex], instRBs_[instIdx], instCUs_[instIdx]);
        selectedApps.insert(appIndex);  // mark the application as selected
        appMaxOffTime_[appIds_[appIndex]] = instMaxOffTime_[instIdx];  // store the maximum offloading time for the application
        appUtility_[appIds_[appIndex]] = instUtility_[instIdx];  // store the utility for the application

        // update the RSU status
        rsuRBs_[rsuIndex] -= instRBs_[instIdx];
        rsuCUs_[rsuIndex] -= instCUs_[instIdx];
    }

    EV << NOW << " SchemeFastLR::scheduleRequests - FastLR schedule scheme ends, selected " 
       << solution.size() << " instances from "<< instAppIndex_.size() << " total instances" << endl;

    return solution;  // return the solution set
}
