//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    SchemeFwdGreedy.cc / SchemeFwdGreedy.h
//
//  Description:
//    This file implements the basic scheduling scheme in the Mobile Edge Computing System with
//    backhaul network support, where tasks can be forwarded among RSUs after being offloaded to the access RSU.
//    The SchemeFwdGreedy class provides common functionalities for different scheduling schemes that support task forwarding.
//    By default, a greedy scheduling scheme is implemented.
//      [scheme source: C. Gao, A. Shaan and A. Easwaran, "Deadline-constrained Multi-resource Task Mapping 
//      and Allocation for Edge-Cloud Systems," GLOBECOM 2022, doi: 10.1109/GLOBECOM48099.2022.10001137.]
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/apps/scheduler/energy/SchemeFwdGreedy.h"

SchemeFwdGreedy::SchemeFwdGreedy(Scheduler *scheduler)
    : SchemeBase(scheduler)
{
    // Additional initialization specific to the forwarding scheme can be added here
    virtualLinkRate_ = scheduler_->virtualLinkRate_;  // Get the virtual link rate from the scheduler
    fairFactor_ = scheduler_->fairFactor_;  // Get the fairness factor from the scheduler

    EV << NOW << " SchemeFwdGreedy::SchemeFwdGreedy - Initialized" << endl;
}


void SchemeFwdGreedy::initializeData()
{
    // Initialize the scheduling data
    EV << NOW << " SchemeFwdGreedy::initializeData - Initializing scheduling data" << endl;

        // initialize the apps vector, using vector index number to represent the real application ID
    appIds_.clear();
    appId2Index_.clear();  // clear the mapping from application ID to index
    appIds_.reserve(unscheduledApps_.size());  // reserve memory for the apps vector
    for (AppId appId : unscheduledApps_)    // enumerate the unscheduled apps
    {
        appIds_.push_back(appId);
        appId2Index_[appId] = appIds_.size() - 1;  // map the application ID to the index in the apps vector
    }

    // initialize the rsuIds vector, using vector index number to represent the real RSU IDs
    rsuIds_.clear();
    rsuId2Index_.clear();  // clear the mapping from RSU ID to index
    rsuRBs_.clear();
    rsuCUs_.clear();
    for (auto &rsuPair : rsuStatus_)   // enumerate the RSUs
    {
        MacNodeId rsuId = rsuPair.first;  // get the RSU ID
        rsuIds_.push_back(rsuId);  // push the RSU ID
        rsuId2Index_[rsuId] = rsuIds_.size() - 1;  // map the RSU ID to the index in the rsuIds vector
        rsuRBs_.push_back(rsuPair.second.bands - rsuOnholdRbs_[rsuId]);  // push the RSU band capacity
        rsuCUs_.push_back(rsuPair.second.cmpUnits - rsuOnholdCus_[rsuId]);  // push the RSU computing capacity
    }

    // clear the service instances vectors
    instAppIndex_.clear();  // application IDs for the service instances
    instRBs_.clear();  // resource blocks for the service instances
    instCUs_.clear();  // computing units for the service instances
    instUtility_.clear();  // energy savings for the service instances
    instMaxOffTime_.clear();  // maximum allowable offloading time for the service instances
    appMaxOffTime_.clear();  // maximum allowable offloading time for the applications
    appUtility_.clear();  // utility (i.e., energy savings) for the applications
    instOffRsuIndex_.clear();  // Clear the offload RSU index vector
    instProRsuIndex_.clear();  // Clear the processing RSU index vector
    appExeDelay_.clear();  // execution delay for the applications
    instExeDelay_.clear();  // execution delay for the service instances
}


void SchemeFwdGreedy::generateScheduleInstances()
{
    EV << NOW << " SchemeFwdGreedy::generateScheduleInstances - Generating schedule instances" << endl;

    initializeData();  // transform the scheduling data

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
            for(MacNodeId offRsuId : vehAccessRsu_[vehId])   // enumerate the RSUs in access
            {
                if (rsuStatus_.find(offRsuId) == rsuStatus_.end())
                    continue;  // if not found, skip

                // find the accessible RSU from the offload RSU
                map<MacNodeId, int> accessibleProRsus = reachableRsus_[offRsuId]; // {procRsuId: hopCount}, the accessible processing RSUs from the offload RSU
                int offRsuIndex = rsuId2Index_[offRsuId];  // get the index of the RSU in the rsuIds vector
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
                            instExeDelay_.push_back(exeDelay);  // execution delay for the instance
                        }
                    }
                }
            }
        }
    }
}


vector<srvInstance> SchemeFwdGreedy::scheduleRequests()
{
    EV << NOW << " SchemeFwdGreedy::scheduleRequests - greedy schedule scheme starts" << endl;

    if (appIds_.empty())
    {
        EV << NOW << " SchemeFwdGreedy::scheduleRequests - no applications to schedule, returning empty vector" << endl;
        return {};  // return an empty vector if no applications are available
    }

    map<int, double> instEfficiency;  // map to store the efficiency of each instance
    int totalCount = instAppIndex_.size();  // total number of service instances
    vector<int> sortedInst(totalCount, 0);  // vector to store the sorted instance indices
    for (int instIdx = 0; instIdx < totalCount; instIdx++)   // enumerate the service instances
    {
        double rb = instRBs_[instIdx];
        double cu = instCUs_[instIdx];
        double availableRB = rsuRBs_[instOffRsuIndex_[instIdx]];
        double availableCU = rsuCUs_[instProRsuIndex_[instIdx]];
        sortedInst[instIdx] = instIdx;  // fill sortedInstIdx with indices from 0 to size-1
        if (availableRB <= 0 || availableCU <= 0)   // if the available resources are not enough, skip
        {
            instEfficiency[instIdx] = 0;  // set efficiency to 0 if resources are not enough
            continue;
        }
        double rbUtil = rb / availableRB;  // band utilization
        double cuUtil = cu / availableCU;  // computing unit utilization
        instEfficiency[instIdx] = instUtility_[instIdx] / (rbUtil * cuUtil);
    }

    // sort the instances by efficiency in descending order
    sort(sortedInst.begin(), sortedInst.end(),
         [&instEfficiency](int a, int b) { return instEfficiency[a] > instEfficiency[b]; }  // sort in descending order
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
        appExeDelay_[appIds_[appIndex]] = instExeDelay_[instIdx];  // store the execution delay for the application

        // update the RSU status
        rsuRBs_[rsuOffIndex] -= resBlocks;
        rsuCUs_[rsuProIndex] -= cmpUnits;
    }

    EV << NOW << " SchemeFwdGreedy::scheduleRequests - greedy schedule scheme ends, selected " << solution.size() 
       << " instances from " << instAppIndex_.size() << " total instances" << endl;

    return solution;  // return the solution set
}


double SchemeFwdGreedy::computeExeDelay(AppId appId, MacNodeId rsuId, double cmpUnits)
{
    /***
     * total computing cycle = T * C
     * where T is the execution time for the full computing resource allocation, and C is the capacity
     *      time = T * C / n
     * * where n is the number of computing units allocated to the application
     */

    //check if db_ is not null
    if (!db_) 
    {
        throw std::runtime_error("SchemeFwdGreedy::computeExeDelay - db_ is null, cannot compute execution delay");
    }
    // execution time for the full computing resource allocation
    double exeTime = db_->getGnbExeTime(appInfo_[appId].service, rsuStatus_[rsuId].deviceType);
    if (exeTime <= 0) 
    {
        stringstream ss;
        ss << NOW << " SchemeFwdGreedy::computeExeDelay - the demanded service " << appInfo_[appId].service 
           << " is not supported on RSU[nodeId=" << rsuId << "], return INFINITY";
        throw std::runtime_error(ss.str());
    }

    if (rsuStatus_[rsuId].cmpCapacity <= 0 || cmpUnits <= 0) {
        return INFINITY;
    }

    return exeTime * rsuStatus_[rsuId].cmpCapacity / cmpUnits;
}


double SchemeFwdGreedy::computeForwardingDelay(int hopCount, int dataSize)
{
    /***
     * Compute the data forwarding delay from the offloading RSU to the processing RSU
     * The forwarding delay consists:
     *      1. The transmission delay within each network hop
     *      2. The propagation delay within each network hop, about 3 microseconds, too small, omit it
     *      3. The switching delay at each RSU within the path, about 1 microsecond, too small, omit it
     *      4. (optional, not used here) The queuing delay at each RSU within the path
     */
    if (hopCount == 0)
        return 0;

    return (dataSize / virtualLinkRate_) * hopCount;
}


double SchemeFwdGreedy::computeUtility(AppId &appId, double &offloadDelay, double &exeDelay, double &period)
{
    // default implementation returns the energy savings
    double savedEnergy = appInfo_[appId].energy - appInfo_[appId].offloadPower * offloadDelay;
    
    return savedEnergy / period;   // energy saving per second
}
