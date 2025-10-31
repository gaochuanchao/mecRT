//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    SchemeGreedy.cc / SchemeGreedy.h
//
//  Description:
//    This file implements the basic scheduling scheme in the Mobile Edge Computing System.
//    The SchemeGreedy class provides common functionalities for different scheduling schemes,
//    such as data initialization, service instance generation, and utility computation.
//    By default, a greedy scheduling scheme is implemented.
//      [scheme source: C. Gao, A. Shaan and A. Easwaran, "Deadline-constrained Multi-resource Task Mapping 
//      and Allocation for Edge-Cloud Systems," GLOBECOM 2022, doi: 10.1109/GLOBECOM48099.2022.10001137.]
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//
#include "mecrt/apps/scheduler/energy/SchemeGreedy.h"
#include "mecrt/apps/scheduler/Scheduler.h"


SchemeGreedy::SchemeGreedy(Scheduler *scheduler)
    : SchemeBase(scheduler)
{
    EV << NOW << " SchemeGreedy::SchemeGreedy - Initialized" << endl;
}


void SchemeGreedy::initializeData()
{
    EV << NOW << " SchemeGreedy::initializeData - transform scheduling data" << endl;

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
    instRsuIndex_.clear();  // RSU IDs for the service instances
    instRBs_.clear();  // resource blocks for the service instances
    instCUs_.clear();  // computing units for the service instances
    instUtility_.clear();  // energy savings for the service instances
    instMaxOffTime_.clear();  // maximum allowable offloading time for the service instances
    appMaxOffTime_.clear();  // maximum allowable offloading time for the applications
    appUtility_.clear();  // utility (i.e., energy savings) for the applications
}

void SchemeGreedy::generateScheduleInstances()
{
    EV << NOW << " SchemeGreedy::generateScheduleInstances - generate schedule instances" << endl;

    initializeData();  // transform the scheduling data

    for (int appIndex = 0; appIndex < appIds_.size(); appIndex++)    // enumerate the unscheduled apps
    {
        AppId appId = appIds_[appIndex];  // get the application ID
        
        double period = appInfo_[appId].period.dbl();
        if (period <= 0)
        {
            EV << NOW << " SchemeGreedy::generateScheduleInstances - invalid period for application " << appId << ", skip" << endl;
            continue;
        }

        MacNodeId vehId = appInfo_[appId].vehId;
        if (vehAccessRsu_.find(vehId) != vehAccessRsu_.end())     // if there exists RSU in access
        {
            for(MacNodeId rsuId : vehAccessRsu_[vehId])   // enumerate the RSUs in access
            {
                // check if the RSU still exists
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
                        instMaxOffTime_.push_back(period - exeDelay - offloadOverhead_);
                    }
                }
            }
        }
    }
}


double SchemeGreedy::computeUtility(AppId &appId, double &offloadDelay, double &exeDelay, double &period)
{
    // default implementation returns the energy savings
    double savedEnergy = appInfo_[appId].energy - appInfo_[appId].offloadPower * offloadDelay;
    
    return savedEnergy / period;   // energy saving per second
}


vector<srvInstance> SchemeGreedy::scheduleRequests()
{
    EV << NOW << " SchemeGreedy::scheduleRequests - greedy schedule scheme starts" << endl;

    if (appIds_.empty()) {
        EV << NOW << " SchemeGreedy::scheduleRequests - no applications to schedule" << endl;
        return {};
    }

    map<int, double> instEfficiency;  // map to store the efficiency of each instance
    int totalCount = instAppIndex_.size();  // total number of service instances
    vector<int> sortedInst(totalCount);
    for (int instIdx = 0; instIdx < totalCount; instIdx++)   // enumerate the service instances
    {
        double rb = instRBs_[instIdx];
        double cu = instCUs_[instIdx];
        double rbUtil = rb / rsuRBs_[instRsuIndex_[instIdx]];  // band utilization
        double cuUtil = cu / rsuCUs_[instRsuIndex_[instIdx]];  // computing unit utilization
        instEfficiency[instIdx] = instUtility_[instIdx] / (rbUtil * cuUtil);

        sortedInst[instIdx] = instIdx;  // fill sortedInstIdx with indices from 0 to size-1
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
    EV << NOW << " SchemeGreedy::scheduleRequests - greedy schedule scheme ends, selected " << solution.size() 
       << " instances from " << instAppIndex_.size() << " total instances" << endl;
    
    return solution;  // return the solution set
}
