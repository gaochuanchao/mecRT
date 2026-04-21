//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    Scheduler.cc / Scheduler.h
//
//  Description:
//    This file implements the global scheduler in the Mobile Edge Computing System.
//    The scheduler collects the vehicle requests and RSU status information,
//    and makes the scheduling decision periodically based on the implemented scheduling scheme.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef _MECRT_SCHEDULER_H_
#define _MECRT_SCHEDULER_H_

#include <string.h>
#include <omnetpp.h>
#include <chrono>

#include <inet/common/INETDefs.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/transportlayer/udp/UdpHeader_m.h"
#include "common/binder/Binder.h"

#include "mecrt/packets/apps/VecPacket_m.h"
#include "mecrt/packets/apps/RsuFeedback_m.h"
#include "mecrt/packets/apps/DistToken_m.h"
#include "mecrt/packets/apps/DistPV_m.h"
#include "mecrt/common/MecCommon.h"
#include "mecrt/common/Database.h"
#include "mecrt/common/NodeInfo.h"
#include <unordered_map>


using namespace omnetpp;
using namespace inet;
using namespace std;


struct RequestMeta {
    int inputSize;          // input data size of the job
    int outputSize;     // output data size
    AppId appId;
    MacNodeId vehId;
    uint32_t ueIpv4Address; // the IPv4 address of the UE
    omnetpp::simtime_t period;          // in milliseconds, the deadline of single job or period of periodic task
    string resourceType;  // whether using GPU or CPU
    string service;            // the service name, app type
    double accuracy;       // the local execution accuracy
    omnetpp::simtime_t stopTime;    // the time when the app left the simulation
    double energy;  // the energy consumption for local processing
    double offloadPower;    // the power used for data offloading
};

struct RsuResource {
    int cmpUnits;          // the remaining free computing units in the RSU
    int cmpCapacity;
    int bands;             // remaining free bands in the RSU
    int bandCapacity;
    string resourceType;   // the resource type of the RSU, e.g., GPU
    string deviceType;       // the device type of the RSU
    Ipv4Address rsuAddress; // the IPv4 address of the RSU
    omnetpp::simtime_t bandUpdateTime;  // the time of updating
    omnetpp::simtime_t cmpUpdateTime;   // the time of updating
};

struct ServiceInstance {
    AppId appId;
    MacNodeId offloadGnbId; 
    MacNodeId processGnbId;
    int cmpUnits;      // allocated computing units of the service
    int bands;         // allocated bands for accessing the service
    omnetpp::simtime_t srvGrantTime;    // the time when the grant of the service is sended
    double utility;       // the utility of the service instance per second
    double exeTime; // the execution time of the service
    double maxOffloadTime;  // the maximum offloading time results in positive energy saving
    string serviceType; // the service type of the application
};


// service instance represented by (appId, offloading rsuId, processing rsuId, bands, cmpUnits)
typedef tuple<AppId, MacNodeId, MacNodeId, int, int> srvInstance;


class SchemeBase;  // forward declaration

class Scheduler : public omnetpp::cSimpleModule
{
  public:
    Database *db_;
    unordered_map<AppId, RequestMeta> appInfo_;
    set<AppId> pendingScheduleApps_;  // the apps that are pending for scheduling
    unordered_map<MacNodeId, RsuResource> rsuStatus_;
    unordered_map<MacNodeId, int> rsuOnholdRbs_;  // {rsuId: onhold resource blocks}, the RSU resource blocks that are on hold
    unordered_map<MacNodeId, int> rsuOnholdCus_;  // {rsuId: onhold computing units}, the RSU computing units that are on hold
    unordered_map<MacNodeId, set<MacNodeId>> vehAccessRsu_;  // {vehId: set(gnbId)}, accessible RSUs/gNBs for each vehicle
    map<tuple<MacNodeId, MacNodeId>, omnetpp::simtime_t> veh2RsuTime_;  // {(vehId, gnbId): updateTime}
    map<tuple<MacNodeId, MacNodeId>, int> veh2RsuRate_;  // {(vehId, gnbId): bytePerBand}  byte rate per band per TTI
    double ttiPeriod_; // duration for each TTI
    double offloadOverhead_;    // the overhead for offloading
    int cuStep_ = 1;  // the step for computing units, default is 1
    int rbStep_ = 1;  // the step for resource blocks, default is 1
    int resourceSlack_ = 2; // the resource slack for schedule instance generation, default is 2
    double srvTimeScale_; // the scale for app execution time on servers with full resource, default is 1.0
    double virtualLinkRate_; // the rate of the virtual link in the backhaul network
    double fairFactor_; // the fairness factor for scheduling scheme with forwarding, default is 1.0
    int maxHops_ = 1; // the maximum number of hops for task forwarding in the backhaul network, default is 1
    MacNodeId rsuId_; // the RSU/gNB ID of the scheduler node

  protected:
    bool enableInitDebug_;
    inet::UdpSocket socket_;
    int socketId_;

    omnetpp::simsignal_t vecSchedulingTimeSignal_;
    omnetpp::simsignal_t vecSchemeTimeSignal_;
    omnetpp::simsignal_t vecInsGenerateTimeSignal_;
    omnetpp::simsignal_t vecDistSchemeExecTimeSignal_;
    omnetpp::simsignal_t vecUtilitySignal_;
    omnetpp::simsignal_t vecPendingAppCountSignal_;
    omnetpp::simsignal_t vecGrantedAppCountSignal_;
    omnetpp::simsignal_t globalSchedulerReadySignal_;
    omnetpp::simsignal_t expectedJobsToBeOffloadedSignal_;

    bool enableBackhaul_ = true; // whether to enable the backhaul network, default is true
    bool enableDistScheme_ = false; // whether to enable distributed scheduling, default is false
    string optimizeObjective_; // the optimization objective for scheduling scheme, default is "accuracy"

    /*
     * Data Structures
     */
    Binder *binder_;
    SchemeBase *scheme_;  // the scheduling scheme used
    string schemeName_;  // the name of the scheduling scheme

    NodeInfo *nodeInfo_;  // the node information of the scheduler node
    int localPort_;  // the port number used by the scheduler module

	  vector<ServiceInstance> vecSchedule_;	// store the temporary schedule generated by the scheduler
    unordered_map<MacNodeId, set<AppId>> rsuWaitInitFbApps_; // {rsuId: set(appId)}, apps pending for initialization feedback for each RSU
	  unordered_map<AppId, ServiceInstance> srvInInitiating_;	// store the service instance that is initiating
    unordered_map<MacNodeId, set<AppId>> veh2AppIds_; // {vehId: set(appId)}, the mapping from vehicle to its requested apps
	
    set<AppId> appsWaitInitFb_; // the apps that are waiting for initialization feedback
    set<AppId> unscheduledApps_;  // the apps that have not been scheduled
	  set<AppId> appsWaitStopFb_; // the apps that are waiting for stop feedback
    set<AppId> allocatedApps_;  // the apps that have been allocated
    unordered_map<AppId, ServiceInstance> runningService_;	// store the service instance that is running (a grant feedback has been received)
    
    bool periodicScheduling_;     /// whether to schedule the tasks periodically, if false, then change to event trigger mode
    bool newAppPending_;    /// whether new application has arrived when the schedule scheme is running, used for event trigger mode
	  bool rescheduleAll_;    /// whether to reschedule all the apps (stop those running apps first), used for reset mode

    /// the interval for scheduling the tasks, this interval should be larger than the schedule scheme execution time
    omnetpp::simtime_t schedulingStartTime_;   /// the time when the scheduling starts, for resource updating
    omnetpp::simtime_t schedulingTime_;    /// the time for scheduling the tasks (schemeExecTime_ + insGenerateTime_)
    omnetpp::simtime_t schemeExecTime_;    /// the execution time of the scheduling scheme
    omnetpp::simtime_t insGenerateTime_;    /// the time for generating the schedule instances
    omnetpp::simtime_t grantAckInterval_;    /// the interval for sending the grant again
    double schedulingInterval_;  
	  double appStopInterval_;    /// the interval for stopping the running application
    double appFeedbackInterval_;    /// the interval for checking the feedback of the running application
    double faultRecoveryMargin_; // the margin for recovering from the fault

    omnetpp::cMessage *schedStarter_;   /// start the scheduling
    omnetpp::cMessage *schedComplete_;    /// inform the completion of scheduling
    omnetpp::cMessage *preSchedCheck_;    /// do the necessary check (stop services) before scheduling
    omnetpp::cMessage *postBatchSchedule_; /// the timer for post-processing after each batch scheduling in distributed scheduling
    omnetpp::cMessage *distInstGenTimer_;  // the timer for generating the schedule instance in distributed scheduling

    // Distributed scheduling related variables
    string distStage_;  // the stage of distributed scheduling, e.g., "CandiSel", "SolSel"
    int pvMax_; // the maximum preference value received
    int pvMin_; // the minimum preference value received
    int targetPV_; // the target preference value for candidate selection
    bool distributedSchemeStarted_; // whether the distributed scheduling scheme has started
    bool batchSchedulingOngoing_; // whether the batch scheduling is ongoing
    string targetCategory_; // the target category for candidate selection, e.g., "LI", "HI"
    map<string, map<string, map<int, vector<Ptr<DistToken>>>>> pv2Tokens_;  // {stage: {category: {pv: vector of tokens}}}
    map<int, int> pvCounter_;  // {pv: count}, the count of received tokens for each preference value
    vector<double> distBatchTimes_; // the batch times for distributed scheduling, used for performance evaluation
    set<AppId> distUnscheduledApps_;  // the unscheduled apps in distributed scheduling, used to update unscheduledApps_
    omnetpp::simtime_t distStartTime_; // the start time of distributed scheduling
    
  public:

    Scheduler();
    ~Scheduler();

    // when the global scheduler is elected, do the necessary initialization
    virtual void globalSchedulerInit();
    // when the topology change and the global scheduler is changed, do the necessary finishing operation if 
    // this node is the previous global scheduler
    virtual void globalSchedulerReset();

    // reset the network topology information
    virtual void resetNetTopology(const map<MacNodeId, map<MacNodeId, double>>& topology);

    // virtual string srvId2Str(VecServiceType service);
    // virtual string resId2Str(VecResourceType resource);
    // virtual string devId2Str(VecDeviceType device);

  protected:

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    /***
     * Initialize the module
     */
    virtual void initialize(int stage) override;
    virtual void initializeSchedulingScheme();

    /***
     * Handle the messages
     */
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    /***
    * Handle the scheduling process
    */
    virtual void handleCentralizedScheduling();
    virtual void handleDistributedScheduling();

    virtual void handlePreSchedulingCheck();

    virtual void finish() override;

    /***
     * Record the vehicle request
     */
    virtual void recordVehRequest(cMessage *msg);

    /***
     * Record the RSU status
     */
    virtual void recordRsuStatus(cMessage *msg);

    /***
     * Schedule the request
     */
    virtual void collectSchedulingResults(vector<srvInstance> &selectedIns);
    virtual void updateNextSchedulingTime();

    /**
     * Remove the outdated vehicle-RSU connection, RSU status, and vehicle requests
     */
    virtual void removeOutdatedInfo();

    /***
     * Determine Pending Schedule Apps
     */
    virtual void determinePendingScheduleApps();

    /***
     * Send the grant to the vehicle
     */
    virtual void sendGrant();

    /***
     * Send the grant to the vehicle
     */
    virtual void sendGrantPacket(ServiceInstance& srv, bool isStart, bool isStop);

    /**
     * stop the running/in-initializing service
     */
    virtual void stopService(AppId appId);

    /***
     * Update the RSU service initialization feedback
     */
    virtual void updateRsuSrvStatusFeedback(cMessage *msg);

    /**
     * Check if the grant is lost (no feedback from rsu for initialization status)
     */
    virtual void checkLostGrant();


    /**
     * Record the distribution token for distributed scheduling
     */
    virtual void recordDistToken(cMessage *msg);

    /***
     * Start Batch Scheduling for distributed scheduling
     */
    virtual void performBatchScheduling();
    virtual void postBatchScheduling();
};

#endif // _MECRT_SCHEDULER_H_
