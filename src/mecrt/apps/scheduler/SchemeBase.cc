//
//                  VecSim
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// the basic scheduling scheme in the Vehicular Edge Computing System
//
#include "mecrt/apps/scheduler/SchemeBase.h"
#include "mecrt/apps/scheduler/Scheduler.h"


SchemeBase::SchemeBase(Scheduler *scheduler)
    : scheduler_(scheduler),
      db_(scheduler->db_),
      unscheduledApps_(scheduler->unscheduledApps_),
      appInfo_(scheduler->appInfo_),
      rsuStatus_(scheduler->rsuStatus_),
      vehAccessRsu_(scheduler->vehAccessRsu_),
      veh2RsuRate_(scheduler->veh2RsuRate_),
      veh2RsuTime_(scheduler->veh2RsuTime_),
      rsuOnholdRbs_(scheduler->rsuOnholdRbs_),
      rsuOnholdCus_(scheduler->rsuOnholdCus_),
      ttiPeriod_(scheduler->ttiPeriod_),
      offloadOverhead_(scheduler->offloadOverhead_),
      cuStep_(scheduler->cuStep_),
      rbStep_(scheduler->rbStep_),
      srvTimeScale_(scheduler->srvTimeScale_)
{
    EV << NOW << " SchemeBase::SchemeBase - Initialized" << endl;
}


void SchemeBase::initializeData()
{
    EV << NOW << " SchemeBase::initializeData - transform scheduling data" << endl;

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


void SchemeBase::generateScheduleInstances()
{
    EV << NOW << " SchemeBase::generateScheduleInstances - generate schedule instances" << endl;

    initializeData();  // transform the scheduling data

    for (int appIndex = 0; appIndex < appIds_.size(); appIndex++)    // enumerate the unscheduled apps
    {
        AppId appId = appIds_[appIndex];  // get the application ID
        
        double period = appInfo_[appId].period.dbl();
        if (period <= 0)
        {
            EV << NOW << " SchemeBase::generateScheduleInstances - invalid period for application " << appId << ", skip" << endl;
            continue;
        }

        MacNodeId vehId = appInfo_[appId].vehId;
        set<MacNodeId> outdatedLink = set<MacNodeId>(); // store the set of rsus that are outdated for this vehicle

        if (vehAccessRsu_.find(vehId) != vehAccessRsu_.end())     // if there exists RSU in access
        {
            for(MacNodeId rsuId : vehAccessRsu_[vehId])   // enumerate the RSUs in access
            {
                int rsuIndex = rsuId2Index_[rsuId];  // get the index of the RSU in the rsuIds vector
                
                // check if the link between the veh and rsu is too old
                if (simTime() - veh2RsuTime_[make_tuple(vehId, rsuId)] > scheduler_->connOutdateInterval_)
                {
                    EV << NOW << " SchemeBase::generateScheduleInstances - connection between Veh[nodeId=" << vehId << "] and RSU[nodeId=" 
                        << rsuId << "] is too old, remove the connection" << endl;
                    outdatedLink.insert(rsuId);
                    continue;
                }

                if (veh2RsuRate_[make_tuple(vehId, rsuId)] == 0)   // if the rate is 0, skip
                {
                    EV << NOW << " SchemeBase::generateScheduleInstances - rate between Veh[nodeId=" << vehId << "] and RSU[nodeId=" 
                        << rsuId << "] is 0, remove the connection" << endl;
                    outdatedLink.insert(rsuId);
                    continue;
                }

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

        // remove the outdated RSU from the access list
        for (MacNodeId rsuId : outdatedLink)
        {
            vehAccessRsu_[vehId].erase(rsuId);
            veh2RsuRate_.erase(make_tuple(vehId, rsuId));
            veh2RsuTime_.erase(make_tuple(vehId, rsuId));
        }
    }
}


double SchemeBase::computeOffloadDelay(MacNodeId vehId, MacNodeId rsuId, int bands, int dataSize)
{
    /***
     * during data transmission, there are multiple headers added to the data. In total, 33 bytes are added
     *      - UDP header (8B)
     *      - IP header (20B)
     *      - PdcpPdu header (1B)
     *      - RlcSdu header (2B) : RLC_HEADER_UM
     *      - MacPdu header (2B) : MAC_HEADER
     */
    double rate = veh2RsuRate_[make_tuple(vehId, rsuId)] * bands;  // byte per TTI
    double actualSize = dataSize + 33;
    int numFrames = ceil(actualSize / rate);

    return numFrames * ttiPeriod_;
}

double SchemeBase::computeExeDelay(AppId appId, MacNodeId rsuId, int cmpUnits)
{
    /***
     * total computing cycle = T * C
     * where T is the execution time for the full computing resource allocation, and C is the capacity
     *      time = T * C / n
     * * where n is the number of computing units allocated to the application
     */

    //check if db_ is not null
    if (!db_) {
        std::cout << " SchemeBase::computeExeDelay - db_ is null, cannot compute execution delay" << endl;
    }
    // execution time for the full computing resource allocation
    double exeTime = db_->getRsuExeTime(appInfo_[appId].service, rsuStatus_[rsuId].deviceType);
    return exeTime * rsuStatus_[rsuId].cmpCapacity / cmpUnits;  
}


double SchemeBase::computeUtility(AppId &appId, double &offloadDelay, double &exeDelay, double &period)
{
    // default implementation returns the energy savings
    double savedEnergy = appInfo_[appId].energy - appInfo_[appId].offloadPower * offloadDelay;
    
    return savedEnergy / period;   // energy saving per second
}


double SchemeBase::getMaxOffloadTime(AppId appId)
{
    auto it = appMaxOffTime_.find(appId);
    if (it != appMaxOffTime_.end())
        return it->second;  // return the maximum offloading time for the application
    else
        return 0.0;  // if not found, return 0
}


double SchemeBase::getAppUtility(AppId appId)
{
    auto it = appUtility_.find(appId);
    if (it != appUtility_.end())
        return it->second;  // return the utility for the application
    else
        return 0.0;  // if not found, return 0
}


vector<srvInstance> SchemeBase::scheduleRequests()
{
    EV << NOW << " SchemeBase::scheduleRequests - greedy schedule scheme starts" << endl;

    if (appIds_.empty()) {
        EV << NOW << " SchemeBase::scheduleRequests - no applications to schedule" << endl;
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
    EV << NOW << " SchemeBase::scheduleRequests - greedy schedule scheme ends, selected " << solution.size() 
       << " instances from " << instAppIndex_.size() << " total instances" << endl;
    
    return solution;  // return the solution set
}

