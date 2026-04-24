//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    SchemeSARound.cc / SchemeSARound.h
//
//  Description:
//    This file implements a SARound scheduling scheme in the Mobile Edge Computing System.
//    The SARound scheduling scheme determines the service instance packing for each ES (RSU) one by one.
//    For each ES, it solves a linear programming (LP) relaxation of the original integer linear programming (ILP) problem.
//    Then, it applies a floor rounding technique to convert the fractional solution of the LP into an integer solution.
//      [scheme source: C. Gao and A. Easwaran, "Real-Time Service Subscription and Adaptive Offloading Control in
//       Vehicular Edge Computing", RTSS 2025]
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/apps/scheduler/accuracyNF/AccuracySARound.h"

AccuracySARound::AccuracySARound(Scheduler *scheduler)
    : AccuracyGreedy(scheduler),
      env_()    // initialize the Gurobi environment
{
    // we only need to initialize the Gurobi environment once
    env_.set(GRB_IntParam_OutputFlag, 0);  // suppress output
    env_.set(GRB_IntParam_LogToConsole, 0);
    env_.set(GRB_DoubleParam_TimeLimit, 2);  // set time limit for the optimization, 2s
    env_.set(GRB_IntParam_Threads, 0);  // use default thread setting (32) for optimization
    env_.set(GRB_IntParam_Presolve, -1);  // Let Gurobi decide (default)
    /***
     * Method values:
     *      -1=automatic, 0=primal simplex, 1=dual simplex, 2=barrier, 3=concurrent, 
     *      4=deterministic concurrent, and 5=deterministic concurrent simplex
     */
    env_.set(GRB_IntParam_Method, 1); 

    // perform a dummy optimization to check if the environment is set up correctly
    warmUpGurobiEnv();

    EV << NOW << " AccuracySARound::AccuracySARound - Initialized" << endl;
}


void AccuracySARound::warmUpGurobiEnv()
{
    GRBModel dummyModel(env_);
    GRBVar x = dummyModel.addVar(0.0, 1.0, 0.0, GRB_BINARY, "x");
    dummyModel.set(GRB_IntParam_OutputFlag, 0);
    dummyModel.setObjective(GRBLinExpr(x), GRB_MAXIMIZE);  // Set a dummy objective
    dummyModel.optimize();  // Warm-up run
    EV << NOW << " AccuracySARound::warmUpGurobiEnv - Gurobi environment warmed up" << endl;
}


void AccuracySARound::initializeData()
{
    EV << NOW << " AccuracySARound::initializeData - initialize scheduling data" << endl;

    // call the base class method to initialize the data
    AccuracyGreedy::initializeData();

    // initialize the per-RSU instance index vector
    instPerRsuIndex_.clear();
    instPerRsuIndex_.resize(rsuIds_.size(), vector<int>());

    reductPerAppIndex_.clear();  // clear the reduction vector for each application
    reductPerAppIndex_.resize(appIds_.size(), 0.0);  // initialize the reduction vector with zeros
}


void AccuracySARound::generateScheduleInstances()
{
    initializeData();  // transform the scheduling data
    EV << NOW << " AccuracySARound::generateScheduleInstances - generate schedule instances" << endl;

    bool debugMode = false;
    int instCount = 0;  // trace the number of instances generated
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
                        if (offloadDelay < 0)
                            continue;  // if the offloading delay cannot be computed due to invalid parameters, skip
                        
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
                            if (exeDelay <= 0)
                                continue;  // if the execution delay is invalid, skip

                            double utility = computeUtility(appId, serviceType) / period;   // utility per second
                            if (utility <= 0)   // if the saved energy is less than 0, skip
                                continue;

                            // AppInstance instance = {appIndex, offRsuIndex, resBlocks, cmpUnits, serviceType};
                            instAppIndex_.push_back(appIndex);
                            // instOffRsuIndex_.push_back(rsuIndex);
                            instRBs_.push_back(resBlocks);
                            instCUs_.push_back(minCU);
                            instUtility_.push_back(utility);  // utility for the instance
                            instMaxOffTime_.push_back(period - exeDelay - offloadOverhead_);  // maximum offloading time for the instance
                            instServiceType_.push_back(serviceType);  // selected service type for the instance
                            instExeDelay_.push_back(exeDelay);  // execution delay for the instance
                            
                            instPerRsuIndex_[rsuIndex].push_back(instCount);  // add the instance index to the RSU ID
                            instCount++;  // increment the instance count
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
                            if (exeDelay <= 0)
                                continue;  // if the execution delay is invalid, skip

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
                            // instOffRsuIndex_.push_back(rsuIndex);
                            instRBs_.push_back(minRB);
                            instCUs_.push_back(cmpUnits);
                            instUtility_.push_back(utility);  // utility for the instance
                            instMaxOffTime_.push_back(offloadTimeThreshold);  // maximum offloading time for the instance
                            instServiceType_.push_back(serviceType);  // selected service type for the instance
                            instExeDelay_.push_back(exeDelay);  // execution delay for the instance
                            
                            instPerRsuIndex_[rsuIndex].push_back(instCount);  // add the instance index to the RSU ID
                            instCount++;  // increment the instance count
                        }
                    }
                }
            }
        }
    }    
}


vector<srvInstance> AccuracySARound::scheduleRequests()
{
    EV << NOW << " AccuracySARound::scheduleRequests - SARound schedule scheme starts" << endl;

    if (appIds_.size() == 0) {
        EV << NOW << " AccuracySARound::scheduleRequests - no applications to schedule" << endl;
        return {};
    }

    vector<double> instUtilityTemp = instUtility_;  // create a temporary vector to store the updated utility of each service instance
    vector<srvInstance> solution;  // vector to store the solution set
    // vector to store the indices of the candidate instances for each RSU
    vector<vector<int>> candidateInsts(rsuIds_.size(), vector<int>());

    // enumerate RSU one by one
    for (int rsuIndex = 0; rsuIndex < rsuIds_.size(); rsuIndex++) 
    {
        // check resources of the RSU
        if (rsuRBs_[rsuIndex] <= 0 || rsuCUs_[rsuIndex] <= 0) {
            EV << NOW << " AccuracySARound::scheduleRequests - RSU " << rsuIds_[rsuIndex] << " has no resources, skip" << endl;
            continue;  // skip if the RSU has no resources
        }

        // check if there are service instances for this RSU
        if (instPerRsuIndex_[rsuIndex].empty()) {
            EV << NOW << " AccuracySARound::scheduleRequests - RSU " << rsuIds_[rsuIndex] << " has no service instances, skip" << endl;
            continue;  // skip if there are no service instances for this RSU
        }

        vector<int> candidates = floorRounding(rsuIndex, instUtilityTemp);  // get the service instance candidates for this RSU
        candidateInsts[rsuIndex] = candidates;  // store the candidates for this RSU

        // update the reduction vector for each application
        set<int> consideredApps;  // set to store the considered application indices
        for (int instIdx : candidates) {
            int appIndex = instAppIndex_[instIdx];  // get the application index
            if (consideredApps.find(appIndex) != consideredApps.end()) {   
                continue;  // based on the rounding approach, this will not happen, but we add this check just in case 
            }
            reductPerAppIndex_[appIndex] += instUtilityTemp[instIdx];  // update the reduction for the application
            consideredApps.insert(appIndex);  // add the application index to the considered set
        }
    }

    // check the service instances in candidates from end to start
    set<AppId> selectedApps;  // set to store the selected application indices
    for (int rsuIndex = rsuIds_.size() - 1; rsuIndex >= 0; rsuIndex--) {
        for (int instIdx : candidateInsts[rsuIndex]) 
        {
            int appIndex = instAppIndex_[instIdx];  // get the application index
            AppId appId = appIds_[appIndex];  // get the application ID
            if (selectedApps.find(appId) != selectedApps.end()) {
                continue;  // if the application is already selected, skip
            }

            solution.emplace_back(appId, rsuIds_[rsuIndex], rsuIds_[rsuIndex], instRBs_[instIdx], instCUs_[instIdx]);
            selectedApps.insert(appId);  // mark the application as selected
            appMaxOffTime_[appId] = instMaxOffTime_[instIdx];  // store the maximum offloading time for the application
            appUtility_[appId] = instUtility_[instIdx];  // store the utility for the application
            appExeDelay_[appId] = instExeDelay_[instIdx];  // store the execution delay for the application
            appServiceType_[appId] = instServiceType_[instIdx];  // store the service type for the application
        }
    }

    EV << NOW << " AccuracySARound::scheduleRequests - SARound schedule scheme ends, selected " 
       << solution.size() << " service instances from "<< instAppIndex_.size() << " total service instances" << endl;

    return solution;  // return the scheduled service instances
}


vector<int> AccuracySARound::floorRounding(int rsuIndex, vector<double> &instUtilityTemp)
{
    /**
     * This function is used to determine the service instance candidates for each RSU
     * for service instance index:
     *      - global index refers to the index in the instAppIndex_, instOffRsuIndex_, instRBs_, instCUs_ vectors
     *      - local index refers to the index in the localInstUtils, instGlobalIndices vectors
     */

    // candidates vector to store the index of the selected service instances
    vector<int> candidates;
    // store the global indices of the service instances with positive utility  
    vector<int> instGlobalIndices;
    // store the positive utility values of the service instances in local instance vector
    vector<double> localInstUtils;
    // vector to store the indices of the service instances with positive utility for each application
    // the index is the local index
    vector<vector<int>> instLocalIdxPerApp(appIds_.size(), vector<int>());  
    int maxUtilIdx = -1;  // global index of the service instance with maximum utility
    double maxUtil = 0.0;  // maximum utility of the service instances

    // enumerate the service instances for this RSU
    int totalCount = instPerRsuIndex_[rsuIndex].size();  // total number of service instances for this RSU
    for (int i = 0; i < totalCount; i++) {
        int instGlobalIdx = instPerRsuIndex_[rsuIndex][i];  // get the global index of the service instance
        int appIndex = instAppIndex_[instGlobalIdx];  // get the application index
        
        // update the utility of the service instance
        instUtilityTemp[instGlobalIdx] = instUtility_[instGlobalIdx] - reductPerAppIndex_[appIndex];  
        if (instUtilityTemp[instGlobalIdx] <= 0)
            continue;  // skip if the utility is less than or equal to zero

        if (instUtilityTemp[instGlobalIdx] > maxUtil) {  // check if the utility is greater than the maximum utility
            maxUtil = instUtilityTemp[instGlobalIdx];  // update the maximum utility
            maxUtilIdx = instGlobalIdx;  // update the index of the service instance with maximum utility
        }

        instGlobalIndices.push_back(instGlobalIdx);  // store the global index of the service instance with positive utility
        localInstUtils.push_back(instUtilityTemp[instGlobalIdx]);  // store the positive utility value of the service instance
        instLocalIdxPerApp[appIndex].push_back(instGlobalIndices.size() - 1);
    }
    
    if (instGlobalIndices.empty()) {     // if no service instance has positive utility
        EV << NOW << " AccuracySARound::floorRounding - No service instances with positive utility for RSU " 
           << rsuIds_[rsuIndex] << ", skip" << endl;
        return {};  // return empty candidates if no service instances with positive utility
    }
    else if (instGlobalIndices.size() == 1) {  // if only one service instance has positive utility
        EV << NOW << " AccuracySARound::floorRounding - Only one service instance with positive utility for RSU " 
           << rsuIds_[rsuIndex] << ", select it" << endl;
        return {instGlobalIndices[0]};  // return the single service instance with positive utility
    }


    /***
     * ========= solve the LP problem =========
     * using gurobi to solve the LP problem, obtain the optimal basic solution
     * to maximize the utility of the service instances
     * ========= solve the LP problem =========
     */

    // ========== create a linear programming model ============
    GRBModel model = GRBModel(env_);

    // ========== add all variables to the model ============
    int numVars = instGlobalIndices.size();  // number of variables (service instances with positive utility)
    vector<double> lb(numVars, 0.0);        // lower bounds
    vector<double> ub(numVars, 1.0);        // upper bounds
    vector<char> vtype(numVars, GRB_CONTINUOUS); // variable types
    GRBVar* vars = model.addVars(lb.data(), ub.data(), localInstUtils.data(), vtype.data(), nullptr, numVars);

    // ========== add constraints to the model ============    
    // add resource constraints for the RSU
    std::vector<double> rbCoeffs(numVars, 0.0);
    std::vector<double> cuCoeffs(numVars, 0.0);
    for (int i = 0; i < numVars; i++) {     // i is the local index of the service instance
        int instGlobalIdx = instGlobalIndices[i];  // get the global index of the service instance
        rbCoeffs[i] = instRBs_[instGlobalIdx];  // resource block coefficients
        cuCoeffs[i] = instCUs_[instGlobalIdx];  // computing unit coefficients
    }
    // create linear expressions for the resource block and computing unit constraints
    GRBLinExpr rbConstraint, cuConstraint;
    rbConstraint.addTerms(rbCoeffs.data(), vars, numVars);  // resource block constraint
    cuConstraint.addTerms(cuCoeffs.data(), vars, numVars);  // computing unit constraint
    model.addConstr(rbConstraint <= rsuRBs_[rsuIndex], "RB_Constraint");  // add resource block constraint to the model
    model.addConstr(cuConstraint <= rsuCUs_[rsuIndex], "CU_Constraint");  // add computing unit constraint to the model

    // add instance selection constraints for each application
    for (int appIndex = 0; appIndex < appIds_.size(); appIndex++) {
        // get the indices of the service instances for the application
        vector<int>& instsPerApp = instLocalIdxPerApp[appIndex];
        int numInsts = instsPerApp.size();  // number of service instances for the application
        if (numInsts == 0) 
            continue;  // skip if there are no service instances for the application

        // create a linear expression for the instance selection constraint
        vector<double> instCoeffs(numInsts, 1.0);  // coefficients for the constraint
        vector<GRBVar> varArray(numInsts);
        for (int i = 0; i < numInsts; i++) {    // i-th local index of the service instance
            varArray[i] = vars[instsPerApp[i]];  // get the variable for the service instance
        }

        GRBLinExpr instConstraint;
        instConstraint.addTerms(instCoeffs.data(), varArray.data(), numInsts);
        model.addConstr(instConstraint <= 1.0, "App_" + std::to_string(appIndex) + "_Constraint");  // add instance selection constraint to the model
    }

    // set the objective function to maximize the utility of the service instances
    // since we have already set the objective coefficients when adding the variables,
    // we can directly set the model sense to maximize
    model.set(GRB_IntAttr_ModelSense, GRB_MAXIMIZE);  // maximize
    model.update();  // update the model

    // ========== solve the model ============
    try {
        model.optimize();  // optimize the model
    } catch (GRBException& e) {
        EV << NOW << " SchemeSARound::floorRounding - Gurobi exception: " << e.getMessage() << endl;
        return {maxUtilIdx};  // return the service instance with maxmum utility if an exception occurs
    }
    
    // If no solution is found, return the service instance with maximum utility
    if (model.get(GRB_IntAttr_SolCount) <= 0)
    {
        EV << NOW << " SchemeSARound::floorRounding - No solution found, return max utility instance" << endl;
        return {maxUtilIdx};
    }

    // enumerate the variables to get the solution, discard all fractional variables
    double totalUtility = 0.0;  // Accumulate total utility of selected service instances
    for (int i = 0; i < numVars; i++) {
        if (vars[i].get(GRB_DoubleAttr_X) > 0.9999) {  // check if the variable is selected in the solution
            candidates.push_back(instGlobalIndices[i]);  // add the global index of the service instance to the candidates
            totalUtility += localInstUtils[i];  // accumulate the utility of the selected service instance
        }
    }

    // Return the candidates if their combined utility exceeds the max single utility,
    // otherwise return the single instance with maximum utility.
    if (totalUtility > maxUtil)
    {
        // EV << " SchemeSARound::floorRounding - single instance utility is smaller than combined utility" << endl;
        return candidates;
    }
    else
    {
        // EV << " SchemeSARound::floorRounding - single instance utility is larger than combined utility" << endl;
        return {maxUtilIdx};
    }
}
