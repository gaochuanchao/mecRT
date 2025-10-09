//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    SchemeFwdBase.cc / SchemeFwdBase.h
//
//  Description:
//    This file implements the basic scheduling scheme in the Mobile Edge Computing System with
//    backhaul network support, where tasks can be forwarded among RSUs after being offloaded to the access RSU.
//    The SchemeFwdBase class provides common functionalities for different scheduling schemes that support task forwarding.
//    By default, a greedy scheduling scheme is implemented.
//      [scheme source: C. Gao, A. Shaan and A. Easwaran, "Deadline-constrained Multi-resource Task Mapping 
//      and Allocation for Edge-Cloud Systems," GLOBECOM 2022, doi: 10.1109/GLOBECOM48099.2022.10001137.]
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/apps/scheduler/SchemeFwdBase.h"

SchemeFwdBase::SchemeFwdBase(Scheduler *scheduler)
    : SchemeBase(scheduler)
{
    // Additional initialization specific to the forwarding scheme can be added here
    virtualLinkRate_ = scheduler_->virtualLinkRate_;  // Get the virtual link rate from the scheduler
    fairFactor_ = scheduler_->fairFactor_;  // Get the fairness factor from the scheduler

    EV << NOW << " SchemeFwdBase::SchemeFwdBase - Initialized" << endl;
}


void SchemeFwdBase::initializeData()
{
    // Initialize the scheduling data
    EV << NOW << " SchemeFwdBase::initializeData - Initializing scheduling data" << endl;

    // Call the base class method to initialize common data
    SchemeBase::initializeData();

    instOffRsuIndex_.clear();  // Clear the offload RSU index vector
    instProRsuIndex_.clear();  // Clear the processing RSU index vector
}


void SchemeFwdBase::generateScheduleInstances()
{
    EV << NOW << " SchemeFwdBase::generateScheduleInstances - Generating schedule instances" << endl;

    initializeData();  // transform the scheduling data

    // int indexDebug = 11977;
    for (int appIndex = 0; appIndex < appIds_.size(); appIndex++)    // enumerate the unscheduled apps
    {
        AppId appId = appIds_[appIndex];  // get the application ID
        
        double period = appInfo_[appId].period.dbl();
        EV << NOW << " SchemeFwdBase::generateScheduleInstances - AppId=" << appId << ", Period=" << period << endl;
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
                map<MacNodeId, int> accessibleProRsus = reachableRsus_[offRsuId]; // {procRsuId: hopCount}, the accessible processing RSUs from the offload RSU
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

                            // if (instAppIndex_.size() == (indexDebug+1))  // debug
                            // {
                            //     EV << NOW << " SchemeFwdBase::generateScheduleInstances - Debug instance: AppIdx=" << instAppIndex_[indexDebug] 
                            //        << ", OffRsuIdx=" << instOffRsuIndex_[indexDebug] << ", ProRsuIdx=" << instProRsuIndex_[indexDebug]
                            //        << ", RBs=" << instRBs_[indexDebug] << ", CUs=" << instCUs_[indexDebug]
                            //        << ", OffDelay=" << offloadDelay << ", FwdDelay=" << fwdDelay 
                            //        << ", ExeDelay=" << exeDelay << ", TotalDelay=" << totalDelay 
                            //        << ", Utility=" << instUtility_[indexDebug] 
                            //        << ", MaxOffTime=" << (period - fwdDelay - exeDelay - offloadOverhead_) << endl;
                            //     EV << "\t AppId=" << appId
                            //        << ", OffRsuId=" << offRsuId
                            //        << ", ProRsuId=" << procRsuId << endl;
                            // }
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


vector<srvInstance> SchemeFwdBase::scheduleRequests()
{
    EV << NOW << " SchemeFwdBase::scheduleRequests - greedy schedule scheme starts" << endl;

    if (appIds_.empty())
    {
        EV << NOW << " SchemeFwdBase::scheduleRequests - no applications to schedule, returning empty vector" << endl;
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

        // EV << NOW << "  Scheduled instance: AppIdx=" << appIndex << ", OffRsuIdx=" << rsuOffIndex 
        //    << ", ProRsuIdx=" << rsuProIndex << ", RBs=" << resBlocks << ", CUs=" << cmpUnits << endl;

        // EV << "\t Period=" << appInfo_[appIds_[appIndex]].period.dbl() << "s, MaxOffTime="
        //     << instMaxOffTime_[instIdx] << "s, Utility=" << instUtility_[instIdx] << endl;
        // EV << "\t Execution Delay=" << computeExeDelay(appIds_[appIndex], rsuIds_[rsuProIndex], cmpUnits) << endl;
        // EV << "\t SelectedIndex=" << instIdx << endl;

        // EV << NOW << "  Scheduled instance: AppId=" << appIds_[appIndex] << ", OffRsuId=" << rsuIds_[rsuOffIndex] 
        //    << ", ProRsuId=" << rsuIds_[rsuProIndex] << ", RBs=" << resBlocks << ", CUs=" << cmpUnits << endl;

        // add the instance to the solution set
        solution.emplace_back(appIds_[appIndex], rsuIds_[rsuOffIndex], rsuIds_[rsuProIndex], resBlocks, cmpUnits);
        selectedApps.insert(appIndex);  // mark the application as selected
        appMaxOffTime_[appIds_[appIndex]] = instMaxOffTime_[instIdx];  // store the maximum offloading time for the application
        appUtility_[appIds_[appIndex]] = instUtility_[instIdx];  // store the utility for the application

        // update the RSU status
        rsuRBs_[rsuOffIndex] -= resBlocks;
        rsuCUs_[rsuProIndex] -= cmpUnits;
    }

    EV << NOW << " SchemeFwdBase::scheduleRequests - greedy schedule scheme ends, selected " << solution.size() 
       << " instances from " << instAppIndex_.size() << " total instances" << endl;

    return solution;  // return the solution set
}


double SchemeFwdBase::computeForwardingDelay(int hopCount, int dataSize)
{
    /***
     * Compute the data forwarding delay from the offloading RSU to the processing RSU
     * The forwarding delay consists:
     *      1. The transmission delay within each network hop
     *      2. The propagation delay within each network hop, about 3 microseconds, too small, omit it
     *      3. The switching delay at each RSU within the path, about 1 microsecond, too small, omit it
     *      4. (optional, not used here) The queuing delay at each RSU within the path
     */
    
    return (dataSize / virtualLinkRate_) * hopCount;
}
