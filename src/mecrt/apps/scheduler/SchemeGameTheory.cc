//
//                  VecSim
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// the non-cooperative game theory scheduling scheme in resource scheduling
//

#include "mecrt/apps/scheduler/SchemeGameTheory.h"


SchemeGameTheory::SchemeGameTheory(Scheduler *scheduler)
    : SchemeBase(scheduler)
{
    EV << NOW << " SchemeGameTheory::SchemeGameTheory - Initialized" << endl;
}


vector<srvInstance> SchemeGameTheory::scheduleRequests()
{
    /***
     * In a non-cooperative game, each application instance is treated as a player,
     * in each round, the player will choose the RSU that maximizes its utility,
     * therefore, we can simply sort the service instances by their utility,
     * and then greedily add the instances to the solution set until the resources of the RSU are exhausted.
     */

    EV << NOW << " SchemeGameTheory::scheduleRequests - game theory schedule scheme starts" << endl;

    if (appIds_.empty()) {
        EV << NOW << " SchemeGameTheory::scheduleRequests - no applications to schedule" << endl;
        return {};  // return empty vector if no applications to schedule
    }
    
    int totalCount = instAppIndex_.size();  // total number of service instances
    vector<int> sortedInst(totalCount);
    for (int instIdx = 0; instIdx < totalCount; instIdx++) {
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

        int rsuIndex = instRsuIndex_[instIdx];  // get the index of the RSU
        int resBlocks = instRBs_[instIdx];  // get the resource blocks
        int cmpUnits = instCUs_[instIdx];  // get the computing units

        // check if the RSU has enough resources
        if (rsuRBs_[rsuIndex] < resBlocks || rsuCUs_[rsuIndex] < cmpUnits)
            continue;

        // add the instance to the solution set
        solution.emplace_back(appIds_[appIndex], rsuIds_[rsuIndex], rsuIds_[rsuIndex], resBlocks, cmpUnits);
        selectedApps.insert(appIndex);  // mark the application as selected
        appMaxOffTime_[appIds_[appIndex]] = instMaxOffTime_[instIdx];  // store the maximum offloading time for the application
        appUtility_[appIds_[appIndex]] = instUtility_[instIdx];  // store the utility for the application

        // update the RSU status
        rsuRBs_[rsuIndex] -= resBlocks;
        rsuCUs_[rsuIndex] -= cmpUnits;
    }
    EV << NOW << " SchemeGameTheory::scheduleRequests - game theory schedule scheme ends, selected " << solution.size() 
       << " instances from " << instAppIndex_.size() << " total instances" << endl;
    
    return solution;  // return the solution set
}
