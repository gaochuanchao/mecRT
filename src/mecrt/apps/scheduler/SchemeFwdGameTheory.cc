//
//                  VecSim
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// the non-cooperative game theory scheduling scheme in resource scheduling
// consider task forwarding in the backhaul network
//

#include "mecrt/apps/scheduler/SchemeFwdGameTheory.h"

SchemeFwdGameTheory::SchemeFwdGameTheory(Scheduler *scheduler)
    : SchemeFwdBase(scheduler)
{
    EV << NOW << " SchemeFwdGameTheory::SchemeFwdGameTheory - Initialized" << endl;
}


vector<srvInstance> SchemeFwdGameTheory::scheduleRequests()
{
    EV << NOW << " SchemeFwdGameTheory::scheduleRequests - Scheduling requests using game theory" << endl;

    if (appIds_.empty())
    {
        EV << NOW << " SchemeFwdGameTheory::scheduleRequests - no applications to schedule, returning empty vector" << endl;
        return {};  // return an empty vector if no applications are available
    }

    int totalCount = instAppIndex_.size();  // total number of service instances
    vector<int> sortedInst(totalCount);
    for (int instIdx = 0; instIdx < totalCount; instIdx++)   // enumerate the service instances
    {
        sortedInst[instIdx] = instIdx;  // fill sortedInst with indices from 0 to size-1
    }

    // sort the instances by utility in descending order
    sort(sortedInst.begin(), sortedInst.end(),
         [this](int a, int b) { return instUtility_[a] > instUtility_[b]; }  // sort in descending order
        );

    // greedyly add the app instances to the solution set
    vector<srvInstance> solution;  // vector to store the solution set
    set<int> selectedApps = set<int>();  // set to store the selected application indices
    for (int instIdx : sortedInst)   // enumerate the sorted instances
    {
        int appIndex = instAppIndex_[instIdx];  // get the application index
        if (selectedApps.find(appIndex) != selectedApps.end())  // if the application is already selected, skip
            continue;

        int rsuOffIndex = instOffRsuIndex_[instIdx];  // get the index of the RSU
        int rsuProIndex = instProRsuIndex_[instIdx];  // get the index of the RSU
        int resBlocks = instRBs_[instIdx];  // get the resource blocks
        int cmpUnits = instCUs_[instIdx];  // get the computing units

        // check if the RSU has enough resources
        if (rsuRBs_[rsuOffIndex] < resBlocks || rsuCUs_[rsuProIndex] < cmpUnits)
            continue;

        // add the instance to the solution set
        solution.emplace_back(appIds_[appIndex], rsuIds_[rsuOffIndex], rsuIds_[rsuProIndex], resBlocks, cmpUnits);
        selectedApps.insert(appIndex);  // mark the application as selected
        appMaxOffTime_[appIds_[appIndex]] = instMaxOffTime_[instIdx];  // store the maximum offloading time for the application
        appUtility_[appIds_[appIndex]] = instUtility_[instIdx];  // store the utility for the application

        // update the RSU status
        rsuRBs_[rsuOffIndex] -= resBlocks;
        rsuCUs_[rsuProIndex] -= cmpUnits;
    }

    EV << NOW << " SchemeFwdGameTheory::scheduleRequests - game theory schedule scheme ends, selected " << solution.size() 
       << " instances from " << instAppIndex_.size() << " total instances" << endl;

    return solution;  // return the solution set
}