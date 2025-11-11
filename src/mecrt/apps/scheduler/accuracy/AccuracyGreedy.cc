//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    AccuracyGreedy.cc / AccuracyGreedy.h
//
//  Description:
//    This file implements the basic scheduling scheme in the Mobile Edge Computing System with
//    backhaul network support, where tasks can be forwarded among RSUs after being offloaded to the access RSU.
//    The AccuracyGreedy class provides common functionalities for different scheduling schemes that support task forwarding.
//    By default, a greedy scheduling scheme is implemented to maximize accuracy.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/apps/scheduler/accuracy/AccuracyGreedy.h"

AccuracyGreedy::AccuracyGreedy(Scheduler *scheduler)
    : SchemeBase(scheduler)
{
    // Additional initialization specific to the forwarding scheme can be added here
    virtualLinkRate_ = scheduler_->virtualLinkRate_;  // Get the virtual link rate from the scheduler
    fairFactor_ = scheduler_->fairFactor_;  // Get the fairness factor from the scheduler

    EV << NOW << " AccuracyGreedy::AccuracyGreedy - Initialized" << endl;
}


void AccuracyGreedy::initializeData()
{
    // Initialize the scheduling data
    EV << NOW << " AccuracyGreedy::initializeData - Initializing scheduling data" << endl;

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
    instOffRsuIndex_.clear();  // Clear the offload RSU index vector
    instProRsuIndex_.clear();  // Clear the processing RSU index vector
    instServiceType_.clear();  // Clear the selected service type vector
    instExeDelay_.clear();  // Clear the execution delay vector
    
    appMaxOffTime_.clear();  // maximum allowable offloading time for the applications
    appUtility_.clear();  // utility (i.e., energy savings) for the applications
    appExeDelay_.clear();  // Clear the application execution delay vector
    appServiceType_.clear();  // Clear the application service type vector
}



void AccuracyGreedy::generateScheduleInstances()
{
    EV << NOW << " AccuracyGreedy::generateScheduleInstances - Generating schedule instances" << endl;

    initializeData();  // transform the scheduling data

    bool debugMode = false;

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
            
            for(MacNodeId offRsuId : vehAccessRsu_[vehId])   // enumerate the RSUs in access
            {
                if (rsuStatus_.find(offRsuId) == rsuStatus_.end())
                    continue;  // if not found, skip

                int offRsuIndex = rsuId2Index_[offRsuId];  // get the index of the RSU in the rsuIds vector
                if (rsuRBs_[offRsuIndex] <= 0)
                    continue;  // if there is no resource blocks available, skip

                int maxRB = floor(rsuRBs_[offRsuIndex] * fairFactor_);  // maximum resource blocks for the offload RSU
                // find the accessible RSU from the offload RSU, {procRsuId: hopCount}, the accessible processing RSUs from the offload RSU
                map<MacNodeId, int> accessibleProRsus = reachableRsus_[offRsuId];
                for (auto& pair : accessibleProRsus)
                {
                    // check if the processing RSU is available
                    if (rsuStatus_.find(pair.first) == rsuStatus_.end())
                        continue;  // if not found, skip
                    
                    int procRsuId = pair.first;
                    int procRsuIndex = rsuId2Index_[procRsuId];  // get the index of the processing RSU
                    if (rsuCUs_[procRsuIndex] <= 0)
                        continue;  // if there is no computing units available, skip
                    int maxCU = floor(rsuCUs_[procRsuIndex] * fairFactor_);  // maximum computing units for the processing RSU

                    int hopCount = pair.second;
                    double fwdDelay = computeForwardingDelay(hopCount, appInfo_[appId].inputSize);

                    if (debugMode)
                        EV << "\t period: " << period << ", offload RSU " << offRsuId << " to process RSU " << procRsuId
                            << " (maxRB: " << maxRB << ", maxCU: " << maxCU << ", fwdDelay: " << fwdDelay << "s)" << endl;
                    // if maxRB/rbStep_ is smaller than maxCU/cuStep_, enumerate RB
                    if (maxRB / rbStep_ < maxCU / cuStep_)
                    {
                        for (int resBlocks = maxRB; resBlocks > 0; resBlocks -= rbStep_)
                        {
                            double offloadDelay = computeOffloadDelay(vehId, offRsuId, resBlocks, appInfo_[appId].inputSize);
                            if (debugMode)
                            {
                                EV << "\t\tenumerate resBlocks " << resBlocks << ", offloadDelay: " << offloadDelay << "s" << endl;
                            }
                                
                            if (fwdDelay + offloadDelay + offloadOverhead_ >= period)
                                break;  // if the forwarding delay is too long, break

                            double exeDelayThreshold = period - offloadDelay - fwdDelay - offloadOverhead_;
                            // enumerate all possible service types for the application
                            set<string> serviceTypes = db_->getGnbServiceTypes();
                            for (const string& serviceType : serviceTypes)
                            {
                                int minCU = computeMinRequiredCUs(procRsuId, exeDelayThreshold, serviceType);
                                if (debugMode)
                                {
                                    EV << "\t\t\tservice type " << serviceType << ", minCU: " << minCU << ", exeDelayThreshold: " << exeDelayThreshold << endl;
                                    debugMode = false;  // only print once
                                }

                                if (minCU > maxCU)
                                    continue;  // if the minimum computing units required is larger than the maximum computing units available, skip

                                double exeDelay = computeExeDelay(procRsuId, minCU, serviceType);
                                double utility = computeUtility(appId, serviceType) / period;   // utility per second
                                if (utility <= 0)   // if the saved energy is less than 0, skip
                                    continue;

                                // AppInstance instance = {appIndex, offRsuIndex, procRsuIndex, resBlocks, cmpUnits};
                                instAppIndex_.push_back(appIndex);
                                instOffRsuIndex_.push_back(offRsuIndex);
                                instProRsuIndex_.push_back(procRsuIndex);
                                instRBs_.push_back(resBlocks);
                                instCUs_.push_back(minCU);
                                instUtility_.push_back(utility);  // energy savings for the instance
                                instMaxOffTime_.push_back(period - fwdDelay - exeDelay - offloadOverhead_);  // maximum offloading time for the instance
                                instServiceType_.push_back(serviceType);  // selected service type for the instance
                                instExeDelay_.push_back(exeDelay);  // execution delay for the instance
                            }
                        }
                    }
                    else    // else enumerate CUs
                    {
                        // enumerate all possible service types for the application
                        set<string> serviceTypes = db_->getGnbServiceTypes();
                        for (const string& serviceType : serviceTypes)
                        {
                            for (int cmpUnits = maxCU; cmpUnits > 0; cmpUnits -= cuStep_)
                            {
                                double exeDelay = computeExeDelay(procRsuId, cmpUnits, serviceType);
                                if (exeDelay + fwdDelay + offloadOverhead_ >= period)
                                    break;  // if the total execution and forwarding time is too long, skip

                                // determine the smallest resource blocks required to meet the deadline
                                double offloadTimeThreshold = period - exeDelay - fwdDelay - offloadOverhead_;
                                int minRB = computeMinRequiredRBs(vehId, offRsuId, offloadTimeThreshold, appInfo_[appId].inputSize);
                                if (minRB > maxRB)
                                    break;  // if the minimum resource blocks required is larger than the maximum resource blocks available, break

                                double utility = computeUtility(appId, serviceType) / period;   // utility per second
                                if (utility <= 0)   // if the saved energy is less than 0, skip
                                    continue;

                                // AppInstance instance = {appIndex, offRsuIndex, procRsuIndex, resBlocks, cmpUnits, serviceType};
                                instAppIndex_.push_back(appIndex);
                                instOffRsuIndex_.push_back(offRsuIndex);
                                instProRsuIndex_.push_back(procRsuIndex);
                                instRBs_.push_back(minRB);
                                instCUs_.push_back(cmpUnits);
                                instUtility_.push_back(utility);  // energy savings for the instance
                                instMaxOffTime_.push_back(offloadTimeThreshold);  // maximum offloading time for the instance
                                instServiceType_.push_back(serviceType);  // selected service type for the instance
                                instExeDelay_.push_back(exeDelay);  // execution delay for the instance
                            }
                        }
                    }
                }
            }
        }
    }
}



vector<srvInstance> AccuracyGreedy::scheduleRequests()
{
    EV << NOW << " AccuracyGreedy::scheduleRequests - greedy schedule scheme starts" << endl;

    if (appIds_.empty())
    {
        EV << NOW << " AccuracyGreedy::scheduleRequests - no applications to schedule, returning empty vector" << endl;
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
        appServiceType_[appIds_[appIndex]] = instServiceType_[instIdx];  // store the service type for the application

        // EV << NOW << " AccuracyGreedy::scheduleRequests - selected application " << appIds_[appIndex]
        //    << " offloaded to RSU " << rsuIds_[rsuOffIndex] << " and processed on RSU " << rsuIds_[rsuProIndex]
        //    << " (RBs: " << resBlocks << ", CUs: " << cmpUnits << ", utility: " << instUtility_[instIdx] << ")" << endl;
        // EV << "\t instance index: " << instIdx << ", max offload time: " << instMaxOffTime_[instIdx]
        //    << "s, execution delay: " << instExeDelay_[instIdx] << "s, service type: " << instServiceType_[instIdx] << endl;

        // update the RSU status
        rsuRBs_[rsuOffIndex] -= resBlocks;
        rsuCUs_[rsuProIndex] -= cmpUnits;
    }

    EV << NOW << " AccuracyGreedy::scheduleRequests - greedy schedule scheme ends, selected " << solution.size() 
       << " instances from " << instAppIndex_.size() << " total instances" << endl;

    return solution;  // return the solution set
}


double AccuracyGreedy::computeExeDelay(MacNodeId rsuId, double cmpUnits, string serviceType)
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
        throw std::runtime_error("AccuracyGreedy::computeExeDelay - db_ is null, cannot compute execution delay");
    }
    
    // Get the execution time from the database
    double baseExeTime = db_->getGnbExeTime(serviceType, rsuStatus_[rsuId].deviceType);
    if (baseExeTime <= 0) 
    {
        stringstream ss;
        ss << NOW << " AccuracyGreedy::computeExeDelay - the demanded service " << serviceType 
           << " is not supported on RSU[nodeId=" << rsuId << "], return INFINITY";
        throw std::runtime_error(ss.str());
    }

    if (rsuStatus_[rsuId].cmpCapacity <= 0 || cmpUnits <= 0) {
        return INFINITY;
    }

    return baseExeTime * rsuStatus_[rsuId].cmpCapacity / cmpUnits;
}


int AccuracyGreedy::computeMinRequiredCUs(MacNodeId rsuId, double exeTimeThreshold, string serviceType)
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
        throw std::runtime_error("AccuracyGreedy::computeExeDelay - db_ is null, cannot compute execution delay");
    }
    
    // Get the execution time from the database
    double baseExeTime = db_->getGnbExeTime(serviceType, rsuStatus_[rsuId].deviceType);
    if (baseExeTime <= 0) 
    {
        stringstream ss;
        ss << NOW << " AccuracyGreedy::computeExeDelay - the demanded service " << serviceType 
           << " is not supported on RSU[nodeId=" << rsuId << "], return INFINITY";
        throw std::runtime_error(ss.str());
    }

    if (rsuStatus_[rsuId].cmpCapacity <= 0 || exeTimeThreshold <= 0) {
        return INFINITY;
    }

    return ceil(baseExeTime * rsuStatus_[rsuId].cmpCapacity / exeTimeThreshold);
}


int AccuracyGreedy::computeMinRequiredRBs(MacNodeId vehId, MacNodeId rsuId, double offloadTimeThreshold, int dataSize)
{
    /**
     * offload time = dataSize / rate
     * rate = rb * virtualLinkRate_
     * => offload time = dataSize / (rb * virtualLinkRate_)
     * => rb = dataSize / (offload time * virtualLinkRate_)
     */

    /***
     * during data transmission, there are multiple headers added to the data. In total, 33 bytes are added
     *      - UDP header (8B)
     *      - IP header (20B)
     *      - PdcpPdu header (1B)
     *      - RlcSdu header (2B) : RLC_HEADER_UM
     *      - MacPdu header (2B) : MAC_HEADER
     */
    if (offloadTimeThreshold <= 0) {
        return INFINITY;
    }

    double actualSize = dataSize + 33;
    double bytesPerRb = offloadTimeThreshold / ttiPeriod_ * veh2RsuRate_[make_tuple(vehId, rsuId)];
    
    return ceil(actualSize / bytesPerRb);
}


double AccuracyGreedy::computeUtility(AppId appId, string serviceType)
{
    /**
     * The utility is defined as the accuracy improvement brought by offloading
     * Utility = accuracy_offload - accuracy_local
     */
    if (!db_) 
    {
        throw std::runtime_error("AccuracyGreedy::computeUtility - db_ is null, cannot compute utility");
    }

    double serviceAccuracy = db_->getGnbServiceAccuracy(serviceType);
    if (serviceAccuracy <= 0) 
    {
        stringstream ss;
        ss << NOW << " AccuracyGreedy::computeUtility - the demanded service " << serviceType 
           << " is not supported, cannot compute utility";
        throw std::runtime_error(ss.str());
    }

    return serviceAccuracy - appInfo_[appId].accuracy;
}


double AccuracyGreedy::computeForwardingDelay(int hopCount, int dataSize)
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




