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

#include "mecrt/apps/scheduler/accuracyNF/AccuracyIterative.h"

AccuracyIterative::AccuracyIterative(Scheduler *scheduler)
    : AccuracyGreedy(scheduler)
{
    maxIter_ = 30;  // set the maximum number of iterations for the iterative scheme
    EV << NOW << " AccuracyIterative::AccuracyIterative - Initialized" << endl;
}


void AccuracyIterative::initializeData()
{
    EV << NOW << " AccuracyIterative::initializeData - initialize scheduling data" << endl;

    // call the base class method to initialize the data
    AccuracyGreedy::initializeData();

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


void AccuracyIterative::generateScheduleInstances()
{
    initializeData();  // transform the scheduling data
    EV << NOW << " AccuracyIterative::generateScheduleInstances - generate schedule instances" << endl;

    bool debugMode = false;
    int instCount = 0;  // trace the number of instances generated
    for (int appIndex = 0; appIndex < appIds_.size(); appIndex++)    // enumerate the unscheduled apps
    {
        AppId appId = appIds_[appIndex];  // get the application ID
        
        double period = appInfo_[appId].period.dbl();
        if (period <= 0)
        {
            EV << NOW << " AccuracyIterative::generateScheduleInstances - invalid period for application " << appId << ", skip" << endl;
            continue;
        }

        MacNodeId vehId = appInfo_[appId].vehId;
        set<int> availMappingSet;  // store the available mapping for this application
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
                            instUtility_.push_back(utility);  // utility for the instance
                            instMaxOffTime_.push_back(period - exeDelay - offloadOverhead_);  // maximum offloading time for the instance
                            instServiceType_.push_back(serviceType);  // selected service type for the instance
                            instExeDelay_.push_back(exeDelay);  // execution delay for the instance
                            
                            instPerRSUPerApp_[appIndex][rsuIndex].push_back(instCount);  // store the instance index in the per-RSU per-application vector
                            instCount++;  // increment the instance count
                            availMappingSet.insert(rsuIndex);  // add the RSU index to the available mapping set
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
                            instUtility_.push_back(utility);  // utility for the instance
                            instMaxOffTime_.push_back(offloadTimeThreshold);  // maximum offloading time for the instance
                            instServiceType_.push_back(serviceType);  // selected service type for the instance
                            instExeDelay_.push_back(exeDelay);  // execution delay for the instance
                            
                            instPerRSUPerApp_[appIndex][rsuIndex].push_back(instCount);  // store the instance index in the per-RSU per-application vector
                            instCount++;  // increment the instance count
                            availMappingSet.insert(rsuIndex);  // add the RSU index to the available mapping set
                        }
                    }
                }
            }
        }

        // store the available mapping for this application
        vector<int> availMappingVec(availMappingSet.begin(), availMappingSet.end());  // convert the set to vector
        availMapping_[appIndex] = availMappingVec;  // store the available mapping for this application
    }    
}


vector<srvInstance> AccuracyIterative::scheduleRequests()
{
    EV << NOW << " AccuracyIterative::scheduleRequests - Iterative schedule scheme starts" << endl;

    if (appIds_.empty()) {
        EV << NOW << " AccuracyIterative::scheduleRequests - no applications to schedule" << endl;
        return {};  // return empty vector if no applications to schedule
    }

    vector<srvInstance> solution;  // vector to store the scheduled service instances
    // temporary vector to store the scheduled service instances index
    vector<int> tempSolution;  
    double totalUtility = 0.0;  // variable to store the total utility of the scheduled service instances

    appInst_.clear();  // clear the application instance vector
    appInst_.resize(appIds_.size(), -1);  // resize the application instance vector with -1 (no instance selected)
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
    }

    // start iterating to find the best mapping and resource allocation
    // double newTotalUtility;  // variable to store the new total utility
    for (int iter = 0; iter < maxIter_; iter++) {
        // determine resource allocation with the selected mapping
        decideResourceAllocation();

        // decide the mapping for the next iteration given the current resource allocation
        decideMapping();

        // Step 3: re-run resource allocation under the updated mapping
        decideResourceAllocation();

        // evaluate the full-round solution
        double newTotalUtility = 0.0;
        vector<int> newTempSolution;
        for (int appIndex = 0; appIndex < appIds_.size(); appIndex++) {
            int instIdx = appInst_[appIndex];
            if (instIdx >= 0) {
                newTotalUtility += instUtility_[instIdx];
                newTempSolution.push_back(instIdx);
            }
        }

        bool sameSolution = (newTempSolution.size() == tempSolution.size());
        if (sameSolution) {
            vector<int> oldSol = tempSolution;
            vector<int> newSol = newTempSolution;
            sort(oldSol.begin(), oldSol.end());
            sort(newSol.begin(), newSol.end());
            sameSolution = (oldSol == newSol);
        }

        if (newTotalUtility > totalUtility) {
            totalUtility = newTotalUtility;
            tempSolution = newTempSolution;
        }
        else if (newTotalUtility == totalUtility && !sameSolution) {
            tempSolution = newTempSolution;  // accept plateau move
        }
        else {
            EV << NOW << " AccuracyIterative::scheduleRequests - converged after full round."
            << " Current iterative count " << (iter + 1) << endl;
            break;
        }
    }

    vector<int> rsuCuTemp = rsuCUs_;  // temporary vector to store the computing units available for each RSU
    vector<int> rsuRbTemp = rsuRBs_;  // temporary vector to store the resource blocks available for each RSU

    set<int> selectedApps;  // set to store the selected applications
    // convert the temporary solution to the final solution
    for (int instIdx : tempSolution) {
        int appIndex = instAppIndex_[instIdx];  // get the application index
        int rsuIndex = instOffRsuIndex_[instIdx];  // get the RSU index
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
        appMaxOffTime_[appId] = instMaxOffTime_[instIdx];  // store the maximum offloading time for the application
        appUtility_[appId] = instUtility_[instIdx];  // store the utility for the application
        appExeDelay_[appId] = instExeDelay_[instIdx];  // store the execution delay for the application
        appServiceType_[appId] = instServiceType_[instIdx];  // store the service type for the application

        // update the temporary resource blocks and computing units
        rsuRbTemp[rsuIndex] -= rb;  // subtract the resource blocks
        rsuCuTemp[rsuIndex] -= cu;  // subtract the computing units
        selectedApps.insert(appIndex);  // add the application index to the selected applications set
    }

    EV << NOW << " AccuracyIterative::scheduleRequests - Iterative schedule scheme ends, selected " << solution.size() 
       << " instances" << endl;
    
    return solution;  // return the scheduled service instances
}


void AccuracyIterative::decideResourceAllocation()
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
    // sort(candidateInst.begin(), candidateInst.end(),
    //      [&](int a, int b) { return instUtility_[a] > instUtility_[b]; });

    // sort candidateInst by utility first, then prefer lighter resource usage
    sort(candidateInst.begin(), candidateInst.end(),
        [&](int a, int b) {
            if (instUtility_[a] != instUtility_[b])
                return instUtility_[a] > instUtility_[b];

            int costA = instRBs_[a] + instCUs_[a];
            int costB = instRBs_[b] + instCUs_[b];
            if (costA != costB)
                return costA < costB;

            // additional tie-breaker: prefer smaller CU usage
            if (instCUs_[a] != instCUs_[b])
                return instCUs_[a] < instCUs_[b];

            return instRBs_[a] < instRBs_[b];
        });

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
        int rsuIndex = instOffRsuIndex_[instIdx];  // get the RSU index
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


void AccuracyIterative::decideMapping()
{
    /***
     * determine the mapping under the given resource allocation, but we allow a small tolerance instead of exact (RB, CU) matching.
     * we use a greedy approach to determine the mapping, and use the current allocation as a soft preference:
     *      when utilities are equal, prefer instances close to the current (RB, CU).
     * in each iteration, we choose the mapping for the application with the highest utility
     */
    vector<int> prevAppInst = appInst_;
    appInst_.clear();  // clear the application instance vector
    appInst_.resize(appIds_.size(), -1);  // resize the application instance vector with -1 (no instance selected)

    // tolerance for near-match
    int rbTol = rbStep_ * 2;  // the tolerance for resource blocks, set to 2 steps
    int cuTol = cuStep_ * 2;  // the tolerance for computing units, set to 2 steps

    // collect all possible instances for all currently feasible mappings
    vector<int> candidateInst;
    for (int appIndex = 0; appIndex < appIds_.size(); appIndex++)
    {
        int rb = appRb_[appIndex];
        int cu = appCu_[appIndex];

        bool selectedLastStep = (prevAppInst[appIndex] >= 0);
        
        for (int rsuIndex : availMapping_[appIndex])
        {
            vector<int> & instances = instPerRSUPerApp_[appIndex][rsuIndex];
            for (int instIdx : instances)
            {
                if (!selectedLastStep) {
                    // if not selected in previous allocation step,
                    // consider all feasible instances so the app can re-enter
                    candidateInst.push_back(instIdx);
                }
                else if (abs(instRBs_[instIdx] - rb) <= rbTol &&
                         abs(instCUs_[instIdx] - cu) <= cuTol) {
                    // if selected previously, prefer near-match instances
                    candidateInst.push_back(instIdx);
                }
            }
        }
    }

    // fallback: if near-match gives too few candidates, allow all instances
    if (candidateInst.empty()) {
        for (int appIndex = 0; appIndex < appIds_.size(); appIndex++) {
            for (int rsuIndex : availMapping_[appIndex]) {
                vector<int> & instances = instPerRSUPerApp_[appIndex][rsuIndex];
                candidateInst.insert(candidateInst.end(), instances.begin(), instances.end());
            }
        }
    }

    // sort candidateInst by utility in descending order
    // sort(candidateInst.begin(), candidateInst.end(),
    //      [&](int a, int b) { return instUtility_[a] > instUtility_[b]; });

    // sort by utility first; when tied, prefer instances closer to the current resource pair,
    // and then prefer lighter resource usage
    sort(candidateInst.begin(), candidateInst.end(),
         [&](int a, int b) {
             if (instUtility_[a] != instUtility_[b])
                 return instUtility_[a] > instUtility_[b];

             int appA = instAppIndex_[a];
             int appB = instAppIndex_[b];

             int distA = abs(instRBs_[a] - appRb_[appA]) + abs(instCUs_[a] - appCu_[appA]);
             int distB = abs(instRBs_[b] - appRb_[appB]) + abs(instCUs_[b] - appCu_[appB]);
             if (distA != distB)
                 return distA < distB;

             int costA = instRBs_[a] + instCUs_[a];
             int costB = instRBs_[b] + instCUs_[b];
             if (costA != costB)
                 return costA < costB;

             return instOffRsuIndex_[a] < instOffRsuIndex_[b];
         });

    /***
     * iterate through the sorted instances and determine the mapping
     * we will determine the mapping for the application with the highest utility first
     * and continue until no more applications can be mapped or all applications have been considered
     */
    vector<int> rsuRbTemp = rsuRBs_;  // temporary vector to store the resource blocks allocated to each application
    vector<int> rsuCuTemp = rsuCUs_;  // temporary vector to store the computing units allocated to each application
    set<int> consideredApps;  // set to store the applications that have been considered for resource allocation
    for (int instIdx: candidateInst) {
        int appIndex = instAppIndex_[instIdx];    // get the application index
        int rsuIndex = instOffRsuIndex_[instIdx];  // get the RSU index
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

        // update mapping and also allow the resource pair to change
        appMapping_[appIndex] = rsuIndex;
        appInst_[appIndex] = instIdx;
        appRb_[appIndex] = rb;
        appCu_[appIndex] = cu;

        consideredApps.insert(appIndex);  // add the application index to the considered applications set
        rsuRbTemp[rsuIndex] -= rb;  // subtract the resource blocks allocated to this application
        rsuCuTemp[rsuIndex] -= cu;  // subtract the computing units allocated to this application
    }
}

