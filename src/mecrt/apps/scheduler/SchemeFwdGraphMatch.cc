//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    SchemeFwdGraphMatch.cc / SchemeFwdGraphMatch.h
//
//  Description:
//    This file implements the Graph Matching based scheduling scheme in the Mobile Edge Computing System with
//    backhaul network support.
//    The Graph Matching scheduling scheme transform the resource allocation problem into a maximum weight
//    three-dimensional matching problem.
//      [scheme source: C. Gao and A. Easwaran, "Energy-Efficient Joint Offloading and Resource Allocation
//      for Deadline-Constrained Tasks in Multi-Access Edge Computing", RTCSA 2025]
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/apps/scheduler/SchemeFwdGraphMatch.h"

SchemeFwdGraphMatch::SchemeFwdGraphMatch(Scheduler *scheduler)
    : SchemeFwdBase(scheduler),
      env_()    // initialize the Gurobi environment
{
    // we only need to initialize the Gurobi environment once
    env_.set(GRB_IntParam_OutputFlag, 0);  // suppress output
    env_.set(GRB_IntParam_LogToConsole, 0);
    env_.set(GRB_DoubleParam_TimeLimit, 5);  // set time limit for the optimization, 5s
    env_.set(GRB_IntParam_Threads, 0);  // use default thread setting (32) for optimization
    env_.set(GRB_IntParam_Presolve, -1);  // Let Gurobi decide (default)
    /***
     * Method values:
     *      -1=automatic, 0=primal simplex, 1=dual simplex, 2=barrier, 3=concurrent, 
     *      4=deterministic concurrent, and 5=deterministic concurrent simplex
     */
    env_.set(GRB_IntParam_Method, -1);  // use dual simplex method for optimization

    // perform a dummy optimization to check if the environment is set up correctly
    warmUpGurobiEnv();

    // check the value of fairFactor_
    if (fairFactor_ > 1.0 || fairFactor_ < 0.0)
    {
        throw cRuntimeError("SchemeFwdGraphMatch::SchemeFwdGraphMatch - fairFactor_ must be in the range [0.0, 1.0]");
    }
    
    EV << NOW << " SchemeFwdGraphMatch::SchemeFwdGraphMatch - Initialized" << endl;
}


void SchemeFwdGraphMatch::warmUpGurobiEnv()
{
    GRBModel dummyModel(env_);
    GRBVar x = dummyModel.addVar(0.0, 1.0, 0.0, GRB_BINARY, "x");
    dummyModel.set(GRB_IntParam_OutputFlag, 0);
    dummyModel.setObjective(GRBLinExpr(x), GRB_MAXIMIZE);  // Set a dummy objective
    dummyModel.optimize();  // Warm-up run
    EV << NOW << " SchemeSARound::warmUpGurobiEnv - Gurobi environment warmed up" << endl;
}


void SchemeFwdGraphMatch::initializeData()
{
    // Initialize the scheduling data
    EV << NOW << " SchemeFwdGraphMatch::initializeData - Initializing scheduling data" << endl;

    // Call the base class method to initialize common data
    SchemeFwdBase::initializeData();

    // Additional initialization specific to the graph matching scheme can be added here
    instPerOffRsuIndex_.clear();  // Clear the instances per offload RSU index vector
    instPerOffRsuIndex_.resize(rsuIds_.size(), vector<int>());  // Resize to match the number of RSUs
    instPerProRsuIndex_.clear();  // Clear the instances per processing RSU index vector
    instPerProRsuIndex_.resize(rsuIds_.size(), vector<int>());  // Resize to match the number of RSUs
    instPerAppIndex_.clear();  // Clear the instances per application index vector
    instPerAppIndex_.resize(appIds_.size(), vector<int>());  // Resize to match the number of applications
}


void SchemeFwdGraphMatch::generateScheduleInstances()
{
    EV << NOW << " SchemeFwdGraphMatch::generateScheduleInstances - Generating schedule instances" << endl;

    initializeData();  // transform the scheduling data

    int instCount = 0;  // instance count
    for (int appIndex = 0; appIndex < appIds_.size(); appIndex++)    // enumerate the unscheduled apps
    {
        AppId appId = appIds_[appIndex];  // get the application ID
        
        double period = appInfo_[appId].period.dbl();
        if (period <= 0)
        {
            EV << NOW << " SchemeFwdBase::generateScheduleInstances - invalid period for application " << appId << ", skip" << endl;
            continue;
        }

        MacNodeId vehId = appInfo_[appId].vehId;
        set<MacNodeId> outdatedLink = set<MacNodeId>(); // store the set of rsus that are outdated for this vehicle

        if (vehAccessRsu_.find(vehId) != vehAccessRsu_.end())     // if there exists RSU in access
        {
            for(MacNodeId offRsuId : vehAccessRsu_[vehId])   // enumerate the RSUs in access
            {
                int offRsuIndex = rsuId2Index_[offRsuId];  // get the index of the RSU in the rsuIds vector

                // check if the link between the veh and rsu is too old
                if (simTime() - veh2RsuTime_[make_tuple(vehId, offRsuId)] > scheduler_->connOutdateInterval_)
                {
                    EV << NOW << " SchemeFwdBase::generateScheduleInstances - connection between Veh[nodeId=" << vehId << "] and RSU[nodeId=" 
                        << offRsuId << "] is too old, remove the connection" << endl;
                    outdatedLink.insert(offRsuId);
                    continue;
                }

                if (veh2RsuRate_[make_tuple(vehId, offRsuId)] == 0)   // if the rate is 0, skip
                {
                    EV << NOW << " SchemeFwdBase::generateScheduleInstances - rate between Veh[nodeId=" << vehId << "] and RSU[nodeId=" 
                        << offRsuId << "] is 0, remove the connection" << endl;
                    outdatedLink.insert(offRsuId);
                    continue;
                }

                // find the accessible RSU from the offload RSU
                map<MacNodeId, int> accessibleProRsus = reachableRsus_[offRsuId]; // {procRsuId: hopCount}
                int maxRB = floor(rsuRBs_[offRsuIndex] * fairFactor_);  // maximum resource blocks for the offload RSU
                for (int resBlocks = maxRB; resBlocks > 0; resBlocks -= rbStep_)   // enumerate the resource blocks, counting down
                {
                    double offloadDelay = computeOffloadDelay(vehId, offRsuId, resBlocks, appInfo_[appId].inputSize);

                    if (offloadDelay + offloadOverhead_ > period)
                        break;  // if the offload delay is too long, break

                    for (auto& pair : accessibleProRsus)
                    {
                        MacNodeId procRsuId = pair.first;
                        int hopCount = pair.second;

                        double fwdDelay = computeForwardingDelay(hopCount, appInfo_[appId].inputSize);
                        if (fwdDelay + offloadDelay + offloadOverhead_ > period)
                            continue;  // if the forwarding delay is too long, skip

                        int procRsuIndex = rsuId2Index_[procRsuId];  // get the index of the processing RSU
                        // enumerate the computation units, counting down
                        int maxCU = floor(rsuCUs_[procRsuIndex] * fairFactor_);  // maximum computing units for the processing RSU
                        for (int cmpUnits = maxCU; cmpUnits > 0; cmpUnits -= cuStep_)
                        {
                            double exeDelay = computeExeDelay(appId, procRsuId, cmpUnits);
                            double totalDelay = offloadDelay + fwdDelay + exeDelay + offloadOverhead_;
                            if (totalDelay > period)
                                break;  // if the total delay is too long, break

                            double utility = computeUtility(appId, offloadDelay, exeDelay, period);
                            if (utility <= 0)   // if the saved energy is less than 0, skip
                                continue;

                            // AppInstance instance = {appIndex, offRsuIndex, procRsuIndex, resBlocks, cmpUnits};
                            instAppIndex_.push_back(appIndex);
                            instOffRsuIndex_.push_back(offRsuIndex);
                            instProRsuIndex_.push_back(procRsuIndex);
                            instRBs_.push_back(resBlocks);
                            instCUs_.push_back(cmpUnits);
                            instUtility_.push_back(utility);  // energy savings for the instance
                            instMaxOffTime_.push_back(period - fwdDelay - exeDelay - offloadOverhead_);  // maximum offloading time for the instance

                            instPerOffRsuIndex_[offRsuIndex].push_back(instCount);  // add the instance index to the offload RSU ID
                            instPerProRsuIndex_[procRsuIndex].push_back(instCount);  // add the instance index to the processing RSU ID
                            instPerAppIndex_[appIndex].push_back(instCount);  // add the instance index to the application ID
                            instCount++;  // increment the instance count
                        }
                    }
                } 
            }
        }

        // remove the outdated RSU from the access list
        for (MacNodeId rsuId : outdatedLink)
        {
            vehAccessRsu_[vehId].erase(rsuId);
            veh2RsuRate_.erase(make_tuple(vehId, rsuId));
            veh2RsuTime_.erase(make_tuple(vehId, rsuId));
        }
    }
}


vector<srvInstance> SchemeFwdGraphMatch::scheduleRequests()
{
    EV << NOW << " SchemeFwdGraphMatch::scheduleRequests - graph matching schedule scheme starts" << endl;

    if (appIds_.empty())
    {
        EV << NOW << " SchemeFwdGraphMatch::scheduleRequests - no applications to schedule, returning empty vector" << endl;
        return {};  // return an empty vector if no applications are available
    }

    // Perform graph matching scheduling here
    // 1. Solve the LP problem to get the fractional solution
    map<int, double> lpSolution;  // map to store the LP solution
    solvingLP(lpSolution);  // solve the LP problem

    // 2. construct bipartite graphs
    BipartiteGraph offGraph, proGraph;
    
    // initialize app node vectors, make sure both graphs have the same application nodes list
    set<int> appNodeSet;  // set to store unique application nodes
    vector<int> appNodeVec;  // vector to store the application nodes
    map<int, int> appNode2VecIdx;  // map to store the application index to node list index mapping
    for (auto & pair : lpSolution)  // iterate through the LP solution
    {
        int instIdx = pair.first;  // get the instance index
        int appIndex = instAppIndex_[instIdx];  // get the application index
        appNodeSet.insert(appIndex);  // add the application node to the bipartite graph
    }
    for (int appIndex : appNodeSet)  // enumerate the application nodes
    {
        appNodeVec.push_back(appIndex);  // add the application node to the list
        appNode2VecIdx[appIndex] = appNodeVec.size() - 1;  // map the application index to the list index
    }
    offGraph.appNodeSet = appNodeSet;  // set the application nodes in the offload RSU graph
    offGraph.appNodeVec = appNodeVec;  // set the application node list in the offload RSU graph
    offGraph.appNode2VecIdx = appNode2VecIdx;  // set the application index to node list index mapping in the offload RSU graph
    proGraph.appNodeSet = appNodeSet;  // set the application nodes in the processing RSU graph
    proGraph.appNodeVec = appNodeVec;  // set the application node list in the processing RSU graph
    proGraph.appNode2VecIdx = appNode2VecIdx;  // set the application index to node list index mapping in the processing RSU graph

    map<int, vector<int>> instIdx2OffEdgeVecIdx, instIdx2ProEdgeVecIdx;  // maps to store the edges created based on service instances
    constructBipartiteGraph(offGraph, instIdx2OffEdgeVecIdx, lpSolution, true);  // construct the offload RSU graph
    constructBipartiteGraph(proGraph, instIdx2ProEdgeVecIdx, lpSolution, false);  // construct the processing RSU graph

    // 3. construct the tripartite graph by merging the offload and processing RSU graphs
    TripartiteGraph triGraph;
    triGraph.appNodeVec = offGraph.appNodeVec;  // set the application node list in the tripartite graph
    mergeBipartiteGraphs(triGraph, offGraph, instIdx2OffEdgeVecIdx, proGraph, instIdx2ProEdgeVecIdx, lpSolution);

    // 4. Solve the tripartite graph matching problem to get the final solution
    map<int, double> tgmSolution;    // {edge index: solution value}
    solvingRelaxedTripartiteGraphMatching(triGraph, tgmSolution);  // solve the tripartite graph matching problem
    
    // 5. obtain the scheduled instances based on the fractional local ratio method
    vector<srvInstance> solution = fractionalLocalRatioMethod(triGraph, tgmSolution);  // get the scheduled instances

    EV << NOW << " SchemeFwdGraphMatch::scheduleRequests - graph matching schedule scheme ends, selected " 
       << solution.size() << " instances from " << instAppIndex_.size() << " total instances" << endl;

    return solution;
}


void SchemeFwdGraphMatch::solvingLP(map<int, double> & lpSolution)
{
    /***
     * First solving the relaxed LP problem to get the fractional solution
     */
    GRBModel model(env_);
    
    // ========== add all variables to the model ============
    int numVars = instAppIndex_.size();  // number of variables (service instances with positive utility)
    vector<double> lb(numVars, 0.0);        // lower bounds
    vector<double> ub(numVars, 1.0);        // upper bounds
    vector<char> vtype(numVars, GRB_CONTINUOUS); // variable types
    GRBVar* vars = model.addVars(lb.data(), ub.data(), instUtility_.data(), vtype.data(), nullptr, numVars);

    // ========== add constraints to the model ============
    for (int rsuIndex = 0; rsuIndex < rsuIds_.size(); rsuIndex++)
    {
        // 1. the bandwidth constraint for each offload RSU
        int bandCoeffSize = instPerOffRsuIndex_[rsuIndex].size();
        if (bandCoeffSize > 0)
        {
            vector<double> rbCoeffs;  // coefficients for the resource block constraint
            vector<GRBVar> rbVars;  // variables for the resource block constraint
            rbCoeffs.reserve(bandCoeffSize);
            rbVars.reserve(bandCoeffSize);

            for (int idx : instPerOffRsuIndex_[rsuIndex])  // enumerate the instances for the offload RSU
            {
                rbCoeffs.push_back(instRBs_[idx]); // resource blocks for the instance
                rbVars.push_back(vars[idx]);  // add the variable to the resource block constraint
            }

            GRBLinExpr rbConstraint;  // linear expression for the bandwidth constraint
            rbConstraint.addTerms(rbCoeffs.data(), rbVars.data(), bandCoeffSize);  // add terms for the resource block constraint
            double rbLimit = ceil(rsuRBs_[rsuIndex] * (1 - fairFactor_));  // maximum resource blocks for the offload RSU
            model.addConstr(rbConstraint <= rbLimit, "RB_Constraint_" + to_string(rsuIndex));  // add the constraint to the model
        }

        // 2. the computing unit constraint for each processing RSU
        int cmpCoeffSize = instPerProRsuIndex_[rsuIndex].size();
        if (cmpCoeffSize > 0)
        {
            vector<double> cuCoeffs;  // coefficients for the computing unit constraint
            vector<GRBVar> cuVars;  // variables for the computing unit constraint
            cuCoeffs.reserve(cmpCoeffSize);
            cuVars.reserve(cmpCoeffSize);

            for (int idx : instPerProRsuIndex_[rsuIndex])  // enumerate the instances for the processing RSU
            {
                cuCoeffs.push_back(instCUs_[idx]); // computing units for the instance
                cuVars.push_back(vars[idx]);  // add the variable to the computing unit constraint
            }

            GRBLinExpr cuConstraint;  // linear expression for the computing unit constraint
            cuConstraint.addTerms(cuCoeffs.data(), cuVars.data(), cmpCoeffSize);  // add terms for the computing unit constraint
            double cuLimit = ceil(rsuCUs_[rsuIndex] * (1 - fairFactor_));  // maximum computing units for the processing RSU
            model.addConstr(cuConstraint <= cuLimit, "CU_Constraint_" + to_string(rsuIndex));  // add the constraint to the model
        }
    }

    // 3. the service instance constraint
    for (int appIndex = 0; appIndex < appIds_.size(); appIndex++)  // enumerate the applications
    {
        int instSize = instPerAppIndex_[appIndex].size();  // number of instances for the application
        if (instSize > 0)
        {
            vector<double> appCoeffs(instSize, 1.0);  // coefficients for the service instance constraint
            vector<GRBVar> appVars;  // variables for the service instance constraint
            appVars.reserve(instSize);  // reserve space for the variables

            for (int idx : instPerAppIndex_[appIndex])  // enumerate the instances for the application
            {
                appVars.push_back(vars[idx]);  // get the variable for the instance
            }

            GRBLinExpr serviceConstraint;  // linear expression for the service instance constraint
            serviceConstraint.addTerms(appCoeffs.data(), appVars.data(), instSize);  // add terms for the service instance constraint
            model.addConstr(serviceConstraint <= 1.0, "Service_Constraint_" + to_string(appIndex));  // add the constraint to the model
        }
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
        EV << NOW << " SchemeFwdGraphMatch::solvingLP - Gurobi exception: " << e.getMessage() << endl;
        return;
    }

    // get all positive variables
    for (int i = 0; i < numVars; i++)
    {
        if (vars[i].get(GRB_DoubleAttr_X) > 0)  // if the variable value is greater than a small threshold
        {
            lpSolution[i] = vars[i].get(GRB_DoubleAttr_X);  // store the solution in the map
        }
    }
}


void SchemeFwdGraphMatch::constructBipartiteGraph(BipartiteGraph& bg, map<int, vector<int>>& instIdx2EdgeVecIdx, 
    map<int, double>& lpSolution, bool isOffload)
{
    vector<int> & instResource = isOffload ? instRBs_ : instCUs_;  // select the resource vector based on offloading
    vector<int> & instRsuIndex = isOffload ? instOffRsuIndex_ : instProRsuIndex_;  // select the RSU index vector based on offloading
    
    map<array<int, 2>, int> rsuNode2VecIdx;  // map to store the RSU index to node vector index mapping
    map<array<int, 3>, int> edge2VecIdx;  // map to store the edge to edge vector index mapping

    // 1. analyze the LP solution
    map<int, vector<int>> rsuIdx2InstIdx;  // map to store the RSU to instance index mapping
    map<int, double> rsuIdx2fracSum;  // map to store the RSU to fraction sum
    for (auto & pair : lpSolution)  // iterate through the LP solution
    {
        int instIdx = pair.first;  // get the instance index
        double value = pair.second;  // get the variable value
        int rsuIndex = instRsuIndex[instIdx];  // get the RSU index

        if (rsuIdx2InstIdx.find(rsuIndex) == rsuIdx2InstIdx.end())  // if the RSU index is not in the map
        {
            rsuIdx2InstIdx[rsuIndex] = vector<int>();  // create a new vector for the RSU index
            rsuIdx2fracSum[rsuIndex] = 0.0;  // initialize the fraction sum for the RSU index
        }
        rsuIdx2InstIdx[rsuIndex].push_back(instIdx);  // add the instance index to the RSU index
        rsuIdx2fracSum[rsuIndex] += value;  // accumulate the fraction sum for the RSU index
    }

    // 2. construct the bipartite graph
    for (auto & pair : rsuIdx2fracSum)  // iterate through the RSU to fraction sum map
    {
        int rsuIndex = pair.first;  // get the RSU index
        int totalRank = ceil(pair.second);  // get the rank of the RSU node based on the fraction sum
        if (totalRank <= 0)  // if the rank is less than or equal to 0, skip
            continue;

        // define the RSU node in the bipartite graph
        for (int i = 0; i < totalRank; i++)  // fill the vector with the rank values
        {
            bg.rsuNodeSet.insert({rsuIndex, i});  // set the rank values (0-based)
            bg.rsuNodeVec.push_back({rsuIndex, i});  // add the RSU node to the vector
            rsuNode2VecIdx[{rsuIndex, i}] = bg.rsuNodeVec.size() - 1;  // map the RSU index and rank to the node index
        }

        // sort all instance indices for the RSU node in descending order of resource demand
        sort(rsuIdx2InstIdx[rsuIndex].begin(), rsuIdx2InstIdx[rsuIndex].end(), [&](int a, int b) {
                return instResource[a] > instResource[b];  // sort in descending order of resource blocks
            });

        int fracSum = 0;  // initialize the fraction sum for the RSU node
        for (int instIdx : rsuIdx2InstIdx[rsuIndex])  // iterate through the instance indices for the RSU node
        {
            int appIndex = instAppIndex_[instIdx];  // get the application index
            int fracSumCeil = ceil(fracSum);  // calculate the ceiling of the fraction sum
            double oldFracSum = fracSum;  // store the old fraction sum
            fracSum += lpSolution[instIdx];  // accumulate the fraction sum for the RSU node
            instIdx2EdgeVecIdx[instIdx] = vector<int>();  // create a new vector for the instance index

            if (oldFracSum < fracSumCeil)
            {
                int rsuRank = fracSumCeil - 1;  // get the RSU rank based on the fraction sum, which is 0-based
                int appVecIdx = bg.appNode2VecIdx[appIndex];  // get the application node index in the bipartite graph
                int rsuVecIdx = rsuNode2VecIdx[{rsuIndex, rsuRank}];  // get the RSU node index in the bipartite graph
                // check if the edge already exists in the bipartite graph
                if (bg.edgeSet.find({appVecIdx, rsuVecIdx}) == bg.edgeSet.end())
                {
                    bg.edgeSet.insert({appVecIdx, rsuVecIdx});  // add the edge to the bipartite graph
                    bg.edgeVec.push_back({appVecIdx, rsuVecIdx});  // add the edge to the edge list
                    bg.resDemand.push_back(instResource[instIdx]);  // set the resource demand for the edge
                    edge2VecIdx[{appVecIdx, rsuVecIdx}] = bg.edgeVec.size() - 1;  // map the edge to the edge list index
                }
                instIdx2EdgeVecIdx[instIdx].push_back(edge2VecIdx[{appVecIdx, rsuVecIdx}]);

                if (fracSum > fracSumCeil)
                {
                    rsuVecIdx = rsuNode2VecIdx[{rsuIndex, rsuRank + 1}];  // get the next RSU node index in the bipartite graph
                    // add the edge to the bipartite graph
                    bg.edgeSet.insert({appVecIdx, rsuVecIdx});
                    bg.edgeVec.push_back({appVecIdx, rsuVecIdx});  // add the edge to the edge list
                    bg.resDemand.push_back(instResource[instIdx]);  // set the resource demand for the edge
                    edge2VecIdx[{appVecIdx, rsuVecIdx}] = bg.edgeVec.size() - 1;  // map the edge to the edge list index
                    instIdx2EdgeVecIdx[instIdx].push_back(bg.edgeVec.size() - 1);
                }
            }
            else    // oldFracSum == fracSumCeil
            {
                // get the RSU rank based on the fraction sum, which is 0-based
                int rsuRank = fracSumCeil;
                int rsuVecIdx = rsuNode2VecIdx[{rsuIndex, rsuRank}];  // get the RSU node index in the bipartite graph
                int appVecIdx = bg.appNode2VecIdx[appIndex];  // get the application node index in the bipartite graph
                // add the edge to the bipartite graph
                bg.edgeSet.insert({appVecIdx, rsuVecIdx});
                bg.edgeVec.push_back({appVecIdx, rsuVecIdx});  // add the edge to the edge list
                bg.resDemand.push_back(instResource[instIdx]);  // set the resource demand for the edge
                edge2VecIdx[{appVecIdx, rsuVecIdx}] = bg.edgeVec.size() - 1;  // map the edge to the edge list index
                instIdx2EdgeVecIdx[instIdx].push_back(bg.edgeVec.size() - 1);
            }
        }
    }
}


void SchemeFwdGraphMatch::mergeBipartiteGraphs(TripartiteGraph& triGraph, const BipartiteGraph& offGraph, const map<int, vector<int>>& instIdx2OffEdgeVecIdx, 
		const BipartiteGraph& proGraph, const map<int, vector<int>>& instIdx2ProEdgeVecIdx, map<int, double>& lpSolution)
{
    triGraph.offRsuNodeVec = offGraph.rsuNodeVec;  // copy the offload RSU nodes from the offload graph
    triGraph.proRsuNodeVec = proGraph.rsuNodeVec;  // copy the processing RSU nodes from the processing graph
    triGraph.edges4App.resize(triGraph.appNodeVec.size());  // resize the edges for applications
    triGraph.edges4OffRsu.resize(triGraph.offRsuNodeVec.size());  // resize the edges for offload RSUs
    triGraph.edges4ProRsu.resize(triGraph.proRsuNodeVec.size());  // resize the edges for processing RSUs

    // 1. sort all instance indices in lpSolution based on the utility in descending order
    vector<int> sortedInstIdx;  // vector to store the sorted instance indices
    sortedInstIdx.reserve(lpSolution.size());  // reserve space for the sorted instance indices
    for (const auto& pair : lpSolution)  // iterate through the LP solution
    {
        sortedInstIdx.push_back(pair.first);  // add the instance index to the sorted vector
    }
    sort(sortedInstIdx.begin(), sortedInstIdx.end(), [&](int a, int b) {
        return instUtility_[a] > instUtility_[b];  // sort in descending order of utility
    });

    // 2. construct the tripartite graph
    for (int instIdx : sortedInstIdx)  // iterate through the sorted instance indices
    {
        for (int offEdgeVecIdx : instIdx2OffEdgeVecIdx.at(instIdx))  // iterate through the offload edges
        {
            array<int, 2> offEdge = offGraph.edgeVec[offEdgeVecIdx];  // get the offload edge, {appVecIdx, rsuVecIdx}
            int appVecIdx = offEdge[0];  // get the offload RSU ID
            int offRsuVecIdx = offEdge[1];  // get the offload RSU rank

            for (int proEdgeVecIdx : instIdx2ProEdgeVecIdx.at(instIdx))  // iterate through the processing edges
            {
                const auto& proEdge = proGraph.edgeVec[proEdgeVecIdx];  // get the processing edge, {appVecIdx, rsuVecIdx}
                int proRsuVecIdx = proEdge[1];  // get the processing RSU rank

                // create a hyper edge in the tripartite graph
                array<int, 3> hyperEdge = {appVecIdx, offRsuVecIdx, proRsuVecIdx};
                // check if the hyper edge already exists in the tripartite graph
                if (triGraph.edgeSet.find(hyperEdge) != triGraph.edgeSet.end())
                    continue;  // if the hyper edge already exists, skip

                triGraph.edgeSet.insert(hyperEdge);  // add the hyper edge to the tripartite graph
                triGraph.edgeVec.push_back(hyperEdge);  // add the hyper edge to the edge list
                int edgeIndex = triGraph.edgeVec.size() - 1;  // get the edge count (0-based index)

                triGraph.edges4App[appVecIdx].push_back(edgeIndex);  // add the hyper edge to the application map
                triGraph.edges4OffRsu[offRsuVecIdx].push_back(edgeIndex);  // add the hyper edge to the offload RSU map
                triGraph.edges4ProRsu[proRsuVecIdx].push_back(edgeIndex);  // add the hyper edge to the processing RSU map

                // set the resource demand for the hyper edge
                triGraph.rbDemand.push_back(offGraph.resDemand[offEdgeVecIdx]);  // set the bandwidth resource demand for the hyper edge
                triGraph.cuDemand.push_back(proGraph.resDemand[proEdgeVecIdx]);  // set the computing resource demand for the hyper edge

                // compute the utility for the hyper edge
                int appIndex = triGraph.appNodeVec[appVecIdx];  // get the application index
                AppId appId = appIds_[appIndex];  // get the application ID
                int offRsuIndex = offGraph.rsuNodeVec[offRsuVecIdx][0];  // get the offload RSU index
                int proRsuIndex = proGraph.rsuNodeVec[proRsuVecIdx][0];  // get the processing RSU index
                double offloadDelay = computeOffloadDelay(appInfo_[appId].vehId, rsuIds_[offRsuIndex], offGraph.resDemand[offEdgeVecIdx], appInfo_[appId].inputSize);
                double exeDelay = computeExeDelay(appId, rsuIds_[proRsuIndex], proGraph.resDemand[proEdgeVecIdx]);
                double period = appInfo_[appId].period.dbl();
                double utility = computeUtility(appId, offloadDelay, exeDelay, period);
                triGraph.weight.push_back(utility);  // set the utility for the hyper edge
            }
        }
    }
}


void SchemeFwdGraphMatch::solvingRelaxedTripartiteGraphMatching(TripartiteGraph& triGraph, map<int, double>& lpSolution)
{
    /***
     * First solving the relaxed tripartite graph matching problem to get the fractional solution
     * array<int, 5> hyperEdge = {appIndex, offRsuIndex, offRsuRank, proRsuIndex, proRsuRank}
     */
    GRBModel model(env_);

    // ========== add all variables to the model ============
    int numVars = triGraph.edgeVec.size();  // number of variables (hyper edges)
    vector<double> lb(numVars, 0.0);        // lower bounds
    vector<double> ub(numVars, 1.0);        // upper bounds
    vector<char> vtype(numVars, GRB_CONTINUOUS); // variable types
    GRBVar* vars = model.addVars(lb.data(), ub.data(), triGraph.weight.data(), vtype.data(), nullptr, numVars);

    // ========== add constraints to the model ============
    // for each node in the tripartite graph, at most one hyper edge can be selected
    for (int appVecIdx = 0; appVecIdx < triGraph.appNodeVec.size(); appVecIdx++)  // iterate through the application nodes
    {
        int edgeCount = triGraph.edges4App[appVecIdx].size();  // number of hyper edges for the application
        if (edgeCount <= 0)
            continue;  // if there are no hyper edges for the application, skip

        vector<double> appCoeffs(edgeCount, 1.0);  // coefficients for the application constraint
        vector<GRBVar> appVars;  // variables
        appVars.reserve(edgeCount);  // reserve space for the variables
        for (int edgeIndex : triGraph.edges4App[appVecIdx])  // iterate through the hyper edges for the application
        {
            appVars.push_back(vars[edgeIndex]);  // add the variable for the hyper edge
        }

        GRBLinExpr appConstraint;  // linear expression for the application constraint
        appConstraint.addTerms(appCoeffs.data(), appVars.data(), edgeCount);  // add terms for the application constraint
        model.addConstr(appConstraint <= 1.0, "App_Constraint_" + to_string(appVecIdx));  // add the constraint to the model
    }

    for (int offRsuVecIdx = 0; offRsuVecIdx < triGraph.offRsuNodeVec.size(); offRsuVecIdx++)  // iterate through the offload RSU nodes
    {
        int edgeCount = triGraph.edges4OffRsu[offRsuVecIdx].size();  // number of hyper edges for the offload RSU
        if (edgeCount <= 0)
            continue;  // if there are no hyper edges for the offload RSU, skip

        vector<double> offCoeffs(edgeCount, 1.0);  // coefficients for the resource block constraint
        vector<GRBVar> offVars;  // variables for the resource block constraint
        offVars.reserve(edgeCount);  // reserve space for the variables
        for (int edgeIndex : triGraph.edges4OffRsu[offRsuVecIdx])  // iterate through the hyper edges for the offload RSU
        {
            offVars.push_back(vars[edgeIndex]);  // add the variable for the hyper edge
        }

        GRBLinExpr offConstraint;  // linear expression for the resource block constraint
        offConstraint.addTerms(offCoeffs.data(), offVars.data(), edgeCount);  // add terms for the resource block constraint
        // add the constraint to the model
        model.addConstr(offConstraint <= 1.0, "Offload_RSU_Constraint_" + to_string(offRsuVecIdx));
    }

    for (int proRsuVecIdx = 0; proRsuVecIdx < triGraph.proRsuNodeVec.size(); proRsuVecIdx++)  // iterate through the processing RSU nodes
    {
        int edgeCount = triGraph.edges4ProRsu[proRsuVecIdx].size();  // number of hyper edges for the processing RSU
        if (edgeCount <= 0)
            continue;  // if there are no hyper edges for the processing RSU, skip

        vector<double> proCoeffs(edgeCount, 1.0);  // coefficients for the computing unit constraint
        vector<GRBVar> proVars;  // variables for the computing unit constraint
        proVars.reserve(edgeCount);  // reserve space for the variables
        for (int edgeIndex : triGraph.edges4ProRsu[proRsuVecIdx])  // iterate through the hyper edges for the processing RSU
        {
            proVars.push_back(vars[edgeIndex]);  // add the variable for the hyper edge
        }

        GRBLinExpr proConstraint;  // linear expression for the computing unit constraint
        proConstraint.addTerms(proCoeffs.data(), proVars.data(), edgeCount);  // add terms for the computing unit constraint
        // add the constraint to the model
        model.addConstr(proConstraint <= 1.0, "Processing_RSU_Constraint_" + to_string(proRsuVecIdx));
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
        EV << NOW << " SchemeFwdGraphMatch::solvingLP - Gurobi exception: " << e.getMessage() << endl;
        return;
    }

    // get all positive variables
    for (int i = 0; i < numVars; i++)
    {
        if (vars[i].get(GRB_DoubleAttr_X) > 0)  // if the variable value is greater than a small threshold
        {
            lpSolution[i] = vars[i].get(GRB_DoubleAttr_X);  // store the solution in the map
        }
    }
}


vector<srvInstance> SchemeFwdGraphMatch::fractionalLocalRatioMethod(TripartiteGraph& triGraph, map<int, double>& lpSolution)
{
    // 1. analyze the LP solution for the relaxed tripartite graph matching problem
    // lpSolution is a map of {edge vector index: fractional value}
    int candiCount = lpSolution.size();  // number of candidates (hyper edges)
    vector<int> posiEdgeIdx;  // vector to store the positive edge indices
    map<int, double> posiEdgeWeight;  // map to store the edge index to weight mapping
    posiEdgeIdx.reserve(candiCount);  // reserve space for the positive edge indices
    double totalFracValue = 0.0;  // total fractional value
    for (const auto& pair : lpSolution)  // iterate through the LP solution
    {
        int edgeIdx = pair.first;  // get the edge index
        posiEdgeIdx.push_back(edgeIdx);  // add the edge index to the positive edge indices
        posiEdgeWeight[edgeIdx] = triGraph.weight[edgeIdx];  // store the weight for the edge index
        totalFracValue += pair.second;  // accumulate the total fractional value
    }

    map<int, set<int>> posiEdge4App;  // {app vector index: {positive edge indices}}
    map<int, set<int>> posiEdge4OffRsu;  // {offload RSU vector index: {positive edge indices}}
    map<int, set<int>> posiEdge4ProRsu;  // {processing RSU vector index: {positive edge indices}}

    for (int i = 0; i < candiCount; i++)  // iterate through the solution indices
    {
        int edgeIdx = posiEdgeIdx[i];  // get the edge index
        array<int, 3> edge = triGraph.edgeVec[edgeIdx];  // get the hyper edge, {appVecIdx, offRsuVecIdx, proRsuVecIdx}

        posiEdge4App[edge[0]].insert(edgeIdx);
        posiEdge4OffRsu[edge[1]].insert(edgeIdx);
        posiEdge4ProRsu[edge[2]].insert(edgeIdx);
    }

    // 2. sort the positive edge indices based on the fraction sum of its neighbors in descending order
    set<int> edgeIdxSet;  // set to store the indices
    map<int, double> edgeIdx2FracSum;  // map to store the solution index to the fraction sum
    map<int, set<int>> edgeIdx2Neighbors;  // map to store the solution index to the neighbors
    int minFracIndex = -1;  // initialize the minimum fraction index
    double minFracValue = totalFracValue;  // initialize the minimum fraction value
    for (int i = 0; i < candiCount; i++)  // iterate through the candidates
    {
        int edgeIdx = posiEdgeIdx[i];  // get the edge index
        edgeIdxSet.insert(edgeIdx);  // add the edge index to the set
        double fracSum = 0;  // initialize the fraction sum for the edge index

        // find neighbors
        set<int> neighbors;  // set to store the neighbors
        array<int, 3> edge = triGraph.edgeVec[edgeIdx];  // get the hyper edge
        for (int idx : posiEdge4App[edge[0]])  // find neighbors
        {
            neighbors.insert(idx);
        }
        for (int idx : posiEdge4OffRsu[edge[1]])  // find neighbors
        {
            neighbors.insert(idx);
        }
        for (int idx : posiEdge4ProRsu[edge[2]])  // find neighbors
        {
            neighbors.insert(idx);
        }

        for (int idx : neighbors)
        {
            fracSum += lpSolution[idx];  // accumulate the fraction sum for the edge index
        }

        edgeIdx2FracSum[edgeIdx] = fracSum;  // store the fraction sum for the edge index
        edgeIdx2Neighbors[edgeIdx] = neighbors;  // store the neighbors for the edge index

        if (fracSum < minFracValue)  // if the fraction sum is less than the minimum fraction value
        {
            minFracValue = fracSum;  // update the minimum fraction value
            minFracIndex = edgeIdx;  // update the minimum fraction index
        }
    }

    vector<int> sortList;  // vector to store the candidates (indices of the lp solution vector)
    while (sortList.size() < candiCount)
    {
        int selectedIndex = minFracIndex;  // select the minimum fraction index
        sortList.push_back(selectedIndex);  // add the minimum fraction index to the candidates
        edgeIdxSet.erase(selectedIndex);  // remove the minimum fraction index from the set

        if (edgeIdxSet.empty())  // if the set is empty, break
            break;

        // update the neighbors and fraction sum for the remaining indices
        for (int neighbor : edgeIdx2Neighbors[selectedIndex])  // iterate through the neighbors
        {
            edgeIdx2FracSum[neighbor] -= lpSolution[selectedIndex];  // subtract the fractional value of the selected index
        }

        minFracValue = totalFracValue;  // reset the minimum fraction value
        minFracIndex = -1;  // reset the minimum fraction index
        // find the new minimum fraction index
        for (int edgeIdx : edgeIdxSet)  // iterate through the remaining indices
        {
            if (edgeIdx2FracSum[edgeIdx] < minFracValue)  // if the fraction sum is less than the minimum fraction value
            {
                minFracValue = edgeIdx2FracSum[edgeIdx];  // update the minimum fraction value
                minFracIndex = edgeIdx;  // update the minimum fraction index
            }
        }
    }
    
    // 3. apply the fractional local ratio method to select the service instances
    vector<int> selectedEdgeIdx;  // vector to store the selected edge indices
    vector<int> candidates;  // vector to store the selected candidates (indices of the lp solution vector)
    for (int edgeIdx : sortList)  // iterate through the sorted candidates
    {
        if (posiEdgeWeight[edgeIdx] <= 0)  // if the fraction sum is less than or equal to 0, skip
            continue;

        candidates.push_back(edgeIdx);  // add the index to the candidates
        // update the weights of the neighbors
        for (int neighbor : edgeIdx2Neighbors[edgeIdx])  // iterate through the neighbors
        {
            posiEdgeWeight[neighbor] -= posiEdgeWeight[edgeIdx];  // subtract the weight of the selected index from the neighbors
        }
    }
    // consider the candidates in reverse order
    vector<bool> appVecIdxUsed(triGraph.appNodeVec.size(), false);  // vector to track used application indices
    vector<bool> offRsuVecIdxUsed(triGraph.offRsuNodeVec.size(), false);  // vector to track used offload RSU indices
    vector<bool> proRsuVecIdxUsed(triGraph.proRsuNodeVec.size(), false);  // vector to track used processing RSU indices
    for (int i = candidates.size() - 1; i >= 0; i--)  // iterate through the candidates in reverse order
    {
        int edgeIdx = candidates[i];  // get the edge index
        array<int, 3> edge = triGraph.edgeVec[edgeIdx];  // get the hyper edge, {appVecIdx, offRsuVecIdx, proRsuVecIdx}
        int appVecIdx = edge[0];  // get the application vector index
        int offRsuVecIdx = edge[1];  // get the offload RSU vector index
        int proRsuVecIdx = edge[2];  // get the processing RSU vector index

        if (appVecIdxUsed[appVecIdx] || offRsuVecIdxUsed[offRsuVecIdx] || proRsuVecIdxUsed[proRsuVecIdx])  // if any of the indices are already used, skip
            continue;

        selectedEdgeIdx.push_back(edgeIdx);  // add the edge index to the selected edge indices
        appVecIdxUsed[appVecIdx] = true;  // mark the application vector index as used
        offRsuVecIdxUsed[offRsuVecIdx] = true;  // mark the offload RSU vector index as used
        proRsuVecIdxUsed[proRsuVecIdx] = true;  // mark the processing RSU vector index as used
    }

    // 4. construct the scheduled instances based on the selected edge indices
    vector<srvInstance> solution;
    set<int> selectedApps;  // vector to store the selected application IDs
    for (int edgeIdx : selectedEdgeIdx)  // iterate through the selected edge indices
    {
        array<int, 3> edge = triGraph.edgeVec[edgeIdx];  // get the hyper edge, {appVecIdx, offRsuVecIdx, proRsuVecIdx}
        int appIdx = triGraph.appNodeVec[edge[0]];  // get the application ID
        if (selectedApps.find(appIdx) != selectedApps.end())  // if the application ID is already selected, skip
            continue;

        int offRsuIdx = triGraph.offRsuNodeVec[edge[1]][0];  // get the offload RSU index
        int proRsuIdx = triGraph.proRsuNodeVec[edge[2]][0];  // get the processing RSU index
        int rbDemand = triGraph.rbDemand[edgeIdx];  // get the resource block demand for the hyper edge
        int cuDemand = triGraph.cuDemand[edgeIdx];  // get the computing unit demand for the hyper edge

        if (rbDemand > rsuRBs_[offRsuIdx] || cuDemand > rsuCUs_[proRsuIdx])  // if the resource demand exceeds the RSU capacity, skip
            continue;

        // compute the maximum offload delay
        AppId appId = appIds_[appIdx];
        double processDelay = computeExeDelay(appId, rsuIds_[proRsuIdx], cuDemand);
        MacNodeId srcId = rsuIds_[offRsuIdx];
        MacNodeId dstId = rsuIds_[proRsuIdx];
        int hopCount = reachableRsus_[srcId][dstId];
        double fwdDelay = computeForwardingDelay(hopCount, appInfo_[appId].inputSize);
        double maxOffloadDelay = appInfo_[appId].period.dbl() - processDelay - fwdDelay - offloadOverhead_;

        if (maxOffloadDelay <= 0 || triGraph.weight[edgeIdx] <= 0)  // if the maximum offload delay is less than or equal to 0, or the utility is less than or equal to 0, skip
            continue;

        solution.emplace_back(appId, rsuIds_[offRsuIdx], rsuIds_[proRsuIdx], rbDemand, cuDemand);
        appUtility_[appId] = triGraph.weight[edgeIdx];  // set the utility for the application ID
        appMaxOffTime_[appId] = maxOffloadDelay;  // set the maximum offload time for the application ID

        selectedApps.insert(appIdx);  // add the application ID to the selected application IDs
        rsuRBs_[offRsuIdx] -= rbDemand;  // update the resource blocks for the offload RSU
        rsuCUs_[proRsuIdx] -= cuDemand;  // update the computing units for the processing RSU
    }

    return solution;  // return the scheduled instances
}

