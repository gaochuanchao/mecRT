//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    SchemeIterative.cc / SchemeIterative.h
//
//  Description:
//    This file implements the Iterative based scheduling scheme in the Mobile Edge Computing System.
//    The Iterative scheduling scheme is an approach for resource scheduling that decomposes the task mapping
//    and resource allocation problem into two subproblems: task mapping and resource allocation. Then, the two
//    subproblems are solved iteratively until convergence.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/apps/scheduler/SchemeIterative.h"

SchemeIterative::SchemeIterative(Scheduler *scheduler)
    : SchemeBase(scheduler)
{
    maxIter_ = 30;  // set the maximum number of iterations for the iterative scheme
    EV << NOW << " SchemeIterative::SchemeIterative - Initialized" << endl;
}


void SchemeIterative::initializeData()
{
    EV << NOW << " SchemeIterative::initializeData - initialize scheduling data" << endl;

    // call the base class method to initialize the data
    SchemeBase::initializeData();

    int numApp = appIds_.size();  // number of applications
    // initialize the mapping vectors
    appMapping_.clear();
    appMapping_.resize(numApp, -1);  // initialize the application mapping vector with -1 (no mapping)
    appInst_.clear();
    appInst_.resize(numApp, -1);  // initialize the selected instance index for each application to -1
    appCu_.clear();
    appCu_.resize(numApp, 0);  // initialize the computing units allocated to each application to 0
    appRb_.clear();
    appRb_.resize(numApp, 0);  // initialize the resource blocks allocated to each application to 0

    // initialize the available mapping vector
    availMapping_.clear();
    availMapping_.resize(numApp, vector<int>());

    // initialize the per-RSU per-application instance vector
    instPerRSUPerApp_.clear();
    instPerRSUPerApp_.resize(numApp, map<int, vector<int>>());  // {appIdx: {rsuIdx: {instIdx1, instIdx2, ...}}}
}


void SchemeIterative::generateScheduleInstances()
{
    EV << NOW << " SchemeIterative::generateScheduleInstances - generate schedule instances" << endl;

    initializeData();  // transform the scheduling data

    int instCount = 0;  // trace the number of instances generated
    for (int appIndex = 0; appIndex < appIds_.size(); appIndex++)    // enumerate the unscheduled apps
    {
        AppId appId = appIds_[appIndex];  // get the application ID
        
        double period = appInfo_[appId].period.dbl();
        if (period <= 0)
        {
            EV << NOW << " SchemeIterative::generateScheduleInstances - invalid period for application " << appId << ", skip" << endl;
            continue;
        }

        MacNodeId vehId = appInfo_[appId].vehId;
        set<int> availMappingSet;  // store the available mapping for this application
        if (vehAccessRsu_.find(vehId) != vehAccessRsu_.end())     // if there exists RSU in access
        {
            for(MacNodeId rsuId : vehAccessRsu_[vehId])   // enumerate the RSUs in access
            {
                if (rsuStatus_.find(rsuId) == rsuStatus_.end())
                    continue;  // if not found, skip
                
                int rsuIndex = rsuId2Index_[rsuId];  // get the index of the RSU in the rsuIds vector
                for (int cmpUnits = rsuCUs_[rsuIndex]; cmpUnits > 0; cmpUnits -= cuStep_)   // enumerate the computation units, counting down
                {
                    double exeDelay = computeExeDelay(appId, rsuId, cmpUnits);

                    if (exeDelay + offloadOverhead_ >= period)   // if the execution delay is larger than the period, skip
                        break;

                    for (int resBlocks = rsuRBs_[rsuIndex]; resBlocks > 0; resBlocks -= rbStep_)   // enumerate the resource blocks, counting down
                    {
                        double offloadDelay = computeOffloadDelay(vehId, rsuId, resBlocks, appInfo_[appId].inputSize);
                        double totalDelay = offloadDelay + exeDelay + offloadOverhead_;
                        if (totalDelay > period)   // if the offload delay is larger than the period, skip
                            break;

                        double utility = computeUtility(appId, offloadDelay, exeDelay, period);
                        if (utility <= 0)   // if the saved energy is less than 0, skip
                            continue;
                        
                        // AppInstance instance = {appIndex, rsuIndex, resBlocks, cmpUnits};
                        instAppIndex_.push_back(appIndex);
                        instRsuIndex_.push_back(rsuIndex);
                        instRBs_.push_back(resBlocks);
                        instCUs_.push_back(cmpUnits);
                        instUtility_.push_back(utility);  // energy savings for the instance
                        instMaxOffTime_.push_back(period - exeDelay - offloadOverhead_);  // maximum offloading time for the instance

                        instPerRSUPerApp_[appIndex][rsuIndex].push_back(instCount);  // store the instance index in the per-RSU per-application vector
                        instCount++;  // increment the instance count
                        availMappingSet.insert(rsuIndex);  // add the RSU index to the available mapping set
                    }
                }
            }
        }

        // store the available mapping for this application
        vector<int> availMappingVec(availMappingSet.begin(), availMappingSet.end());  // convert the set to vector
        availMapping_[appIndex] = availMappingVec;  // store the available mapping for this application
    }    
}


vector<srvInstance> SchemeIterative::scheduleRequests()
{
    EV << NOW << " SchemeIterative::scheduleRequests - Iterative schedule scheme starts" << endl;

    if (appIds_.empty()) {
        EV << NOW << " SchemeIterative::scheduleRequests - no applications to schedule" << endl;
        return {};  // return empty vector if no applications to schedule
    }

    vector<srvInstance> solution;  // vector to store the scheduled service instances
    // temporary vector to store the scheduled service instances index
    vector<int> tempSolution;  
    double totalUtility = 0.0;  // variable to store the total utility of the scheduled service instances

    // initialize the application mapping and resource allocations
    for (int appIndex = 0; appIndex < appIds_.size(); appIndex++)   // enumerate the applications
    {
        if (availMapping_[appIndex].empty())
            continue;  // skip if no available mapping

        int index = rand() % availMapping_[appIndex].size();  // randomly select an index from the available mapping
        int rsuIndex = availMapping_[appIndex][index];  // get the RSU index from the available mapping

        // check if any service instances are available for this application and RSU
        vector<int> & instances = instPerRSUPerApp_[appIndex][rsuIndex];  // get the instances for this application and RSU
        if (instances.empty())
            continue;  // skip if no instances available for this application and RSU

        // initialize the application mapping
        appMapping_[appIndex] = rsuIndex;  // store the mapping to RSU for this application
        // randomly select an instance from the available instances for this application and RSU
        int instIndex = instances[rand() % instances.size()];  // randomly select an instance index
        appCu_[appIndex] = instCUs_[instIndex];  // get the computing units allocated to this application
        appRb_[appIndex] = instRBs_[instIndex];  // get the resource blocks allocated to this application

        appInst_.clear();  // clear the application instance vector
        appInst_.resize(appIds_.size(), -1);  // resize the application instance vector with -1 (no instance selected)
    }

    // start iterating to find the best mapping and resource allocation
    double newTotalUtility;  // variable to store the new total utility
    for (int iter = 0; iter < maxIter_; iter++) {
        // determine resource allocation with the selected mapping
        decideResourceAllocation();

        // compute the new total utility
        // if the total utility is larger than the previous one, update the solution
        // otherwise, stop the iteration
        newTotalUtility = 0.0;  // reset the new total utility for this iteration
        for (int appIndex = 0; appIndex < appIds_.size(); appIndex++){
            int instIdx = appInst_[appIndex];  // get the instance index for this application
            if (instIdx >= 0) {     // if the instance index is valid
                newTotalUtility += instUtility_[instIdx];  // accumulate the utility values for each application
            }
        }

        if (newTotalUtility > totalUtility) {
            totalUtility = newTotalUtility;  // update the total utility
            tempSolution.clear();  // clear the temporary solution

            // store the scheduled service instances in the solution vector
            for (int appIndex = 0; appIndex < appIds_.size(); appIndex++) {
                int instIdx = appInst_[appIndex];  // get the instance index for this application
                if (instIdx >= 0) {     // if the instance index is valid
                    tempSolution.push_back(instIdx);  // store the temporary solution
                }
            }
        }
        else {
            EV << NOW << " SchemeIterative::scheduleRequests - no improvement in utility, stopping iteration." 
                << " Current iterative count " << (iter+1) << endl;
            break;  // stop the iteration if no improvement in utility
        }

        // decide the mapping for the next iteration given the current resource allocation
        decideMapping();

        // compute the total utility of the current solution
        // if the total utility is larger than the previous one, update the solution
        // otherwise, stop the iteration
        newTotalUtility = 0.0;  // reset the new total utility for the next iteration
        for (int appIndex = 0; appIndex < appIds_.size(); appIndex++){
            int instIdx = appInst_[appIndex];  // get the instance index for this application
            if (instIdx >= 0) {     // if the instance index is valid
                newTotalUtility += instUtility_[instIdx];  // accumulate the utility values for each application
            }
        }

        if (newTotalUtility > totalUtility) {
            totalUtility = newTotalUtility;  // update the total utility
            tempSolution.clear();  // clear the temporary solution

            // store the scheduled service instances in the solution vector
            for (int appIndex = 0; appIndex < appIds_.size(); appIndex++) {
                int instIdx = appInst_[appIndex];  // get the instance index for this application
                if (instIdx >= 0) {     // if the instance index is valid
                    tempSolution.push_back(instIdx);  // store the temporary solution
                }
            }
        }
        else {
            EV << NOW << " SchemeIterative::scheduleRequests - no improvement in utility, stopping iteration." 
                << " Current iterative count " << (iter+1) << endl;
            break;  // stop the iteration if no improvement in utility
        }
    }

    vector<int> rsuCuTemp = rsuCUs_;  // temporary vector to store the computing units available for each RSU
    vector<int> rsuRbTemp = rsuRBs_;  // temporary vector to store the resource blocks available for each RSU

    set<int> selectedApps;  // set to store the selected applications
    // convert the temporary solution to the final solution
    for (int instIdx : tempSolution) {
        int appIndex = instAppIndex_[instIdx];  // get the application index
        int rsuIndex = instRsuIndex_[instIdx];  // get the RSU index
        int rb = instRBs_[instIdx];  // get the resource blocks allocated to this application
        int cu = instCUs_[instIdx];  // get the computing units

        // check if the application has already been selected
        if (selectedApps.find(appIndex) != selectedApps.end()) {
            continue;  // skip if the application has already been selected
        }

        // check if resource blocks and computing units are available
        if (rsuRbTemp[rsuIndex] < rb || rsuCuTemp[rsuIndex] < cu) {
            continue;  // skip if the resource blocks or computing units are not available
        }

        AppId appId = appIds_[appIndex];  // get the application ID
        // create the service instance
        srvInstance srvInst = make_tuple(appId, rsuIds_[rsuIndex], rsuIds_[rsuIndex], rb, cu);
        solution.push_back(srvInst);  // add the service instance to the solution vector

        // compute the maximum offloading time for this application
        appMaxOffTime_[appId] = instMaxOffTime_[instIdx];  // store the maximum offloading time for this application
        appUtility_[appId] = instUtility_[instIdx];  // store the utility for this application

        // update the temporary resource blocks and computing units
        rsuRbTemp[rsuIndex] -= rb;  // subtract the resource blocks
        rsuCuTemp[rsuIndex] -= cu;  // subtract the computing units
        selectedApps.insert(appIndex);  // add the application index to the selected applications set
    }

    EV << NOW << " SchemeIterative::scheduleRequests - Iterative schedule scheme ends, selected " << solution.size() 
       << " instances" << endl;
    
    return solution;  // return the scheduled service instances
}


void SchemeIterative::decideResourceAllocation()
{
    /***
     * determine the resource allocation for a given mapping
     * we use a greedy approach to allocate resources
     * in each iteration, we try to allocate resources to the application with the highest utility
     */
    appInst_.clear();  // clear the application instance vector
    appInst_.resize(appIds_.size(), -1);  // resize the application instance vector with -1 (no instance selected)
    
    // collect all posssible instances for the given mapping
    vector<int> candidateInst;  // vector to store the indices of the instances
    for (int appIndex = 0; appIndex < appIds_.size(); appIndex++)   // enumerate the applications
    {
        int rsuIndex = appMapping_[appIndex];  // get the current mapping RSU index for this application
        if (rsuIndex < 0)   // if the application is not mapped to any RSU, skip
            continue;
        
        // check if any service instances are available for this application and RSU
        vector<int> & instances = instPerRSUPerApp_[appIndex][rsuIndex];  // get the instances for this application and RSU
        if (instances.empty())
            continue;  // skip if no instances available for this application and RSU

        // otherwise, append the instances to the end of candidateInst vector
        candidateInst.insert(candidateInst.end(), instances.begin(), instances.end());
    }

    // sort candidateInst by utility in descending order
    sort(candidateInst.begin(), candidateInst.end(),
         [&](int a, int b) { return instUtility_[a] > instUtility_[b]; });

    /***
     * iterate through the sorted instances and allocate resources
     * we will allocate resources to the application with the highest utility first
     * and continue until no more resources can be allocated or all applications have been considered
     */
    vector<int> rsuRbTemp = rsuRBs_;  // temporary vector to store the resource blocks allocated to each application
    vector<int> rsuCuTemp = rsuCUs_;  // temporary vector to store the computing units allocated to each application
    set<int> consideredApps;  // reset the considered applications set
    // iterate through the instances and allocate resources
    for (int i = 0; i < candidateInst.size(); i++) {
        int instIdx = candidateInst[i];  // get the instance index
        int appIndex = instAppIndex_[instIdx];  // get the application index
        int rsuIndex = instRsuIndex_[instIdx];  // get the RSU index
        int rb = instRBs_[instIdx];  // get the resource blocks
        int cu = instCUs_[instIdx];  // get the computing units

        // check if the application has been considered for resource allocation
        if (consideredApps.find(appIndex) != consideredApps.end()) {
            continue;  // skip if the application has been considered
        }

        // check if the resource blocks and computing units are available
        if (rsuRbTemp[rsuIndex] < rb || rsuCuTemp[rsuIndex] < cu) {
            continue;  // skip if the resource blocks or computing units are not available
        }

        // allocate resources to this application
        appInst_[appIndex] = instIdx;  // store the instance index for this application
        appCu_[appIndex] = cu;  // update the computing units allocated to this application
        appRb_[appIndex] = rb;  // update the resource blocks allocated to this application
        consideredApps.insert(appIndex);  // add the application index to the considered applications set
        
        // update the temporary resource blocks and computing units
        rsuRbTemp[rsuIndex] -= rb;  // subtract the resource blocks allocated to this application
        rsuCuTemp[rsuIndex] -= cu;  // subtract the computing units allocated to this application
    }
}


void SchemeIterative::decideMapping()
{
    /***
     * determine the mapping under the given resource allocation
     * we use a greedy approach to determine the mapping
     * in each iteration, we choose the mapping for the application with the highest utility
     */
    appInst_.clear();  // clear the application instance vector
    appInst_.resize(appIds_.size(), -1);  // resize the application instance vector with -1 (no instance selected)

    // collect all posssible instances index for the given resource allocation
    vector<int> candidateInst;  // vector to store the indices of the instances
    for (int appIndex = 0; appIndex < appIds_.size(); appIndex++)   // enumerate the applications
    {
        int rb = appRb_[appIndex];  // get the resource blocks allocated to this application
        int cu = appCu_[appIndex];  // get the computing units allocated to this application
        // enumerate available mapping for this application
        for (int rsuIndex : availMapping_[appIndex])   // enumerate the RSUs in the available mapping
        {
            vector<int> & instances = instPerRSUPerApp_[appIndex][rsuIndex];  // get the instances for this application and RSU
            for (int instIdx : instances)   // enumerate the instances for this application and RSU
            {
                // check if the resource blocks and computing units match
                if (instRBs_[instIdx] == rb && instCUs_[instIdx] == cu) {  
                    candidateInst.push_back(instIdx);  // add the instance index to the candidate instances vector
                }
            }
        }
    }

    // sort candidateInst by utility in descending order
    sort(candidateInst.begin(), candidateInst.end(),
         [&](int a, int b) { return instUtility_[a] > instUtility_[b]; });

    /***
     * iterate through the sorted instances and determine the mapping
     * we will determine the mapping for the application with the highest utility first
     * and continue until no more applications can be mapped or all applications have been considered
     */
    vector<int> rsuRbTemp = rsuRBs_;  // temporary vector to store the resource blocks allocated to each application
    vector<int> rsuCuTemp = rsuCUs_;  // temporary vector to store the computing units allocated to each application
    set<int> consideredApps;  // set to store the applications that have been considered for resource allocation
    for (int i = 0; i < candidateInst.size(); i++) {
        auto instIdx = candidateInst[i];  // get the instance index
        int appIndex = instAppIndex_[instIdx];    // get the application index
        int rsuIndex = instRsuIndex_[instIdx];  // get the RSU index
        int rb = instRBs_[instIdx];  // get the resource blocks
        int cu = instCUs_[instIdx];  // get the computing units

        // check if the application has been considered for resource allocation
        if (consideredApps.find(appIndex) != consideredApps.end()) {
            continue;  // skip if the application has been considered
        }

        // check if the resource blocks and computing units are available
        if (rsuRbTemp[rsuIndex] < rb || rsuCuTemp[rsuIndex] < cu) {
            continue;  // skip if the resource blocks or computing units are not available
        }

        // determine the mapping for this application
        appMapping_[appIndex] = rsuIndex;  // store the mapping to RSU for this application
        appInst_[appIndex] = instIdx;  // store the instance index for this application

        consideredApps.insert(appIndex);  // add the application index to the considered applications set
        // update the temporary resource blocks and computing units
        rsuRbTemp[rsuIndex] -= rb;  // subtract the resource blocks allocated to this application
        rsuCuTemp[rsuIndex] -= cu;  // subtract the computing units allocated to this application
    }
}

