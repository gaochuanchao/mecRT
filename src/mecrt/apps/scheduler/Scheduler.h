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

#include <inet/common/INETDefs.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/transportlayer/udp/UdpHeader_m.h"
#include "common/binder/Binder.h"

#include "mecrt/packets/apps/VecPacket_m.h"
#include "mecrt/packets/apps/RsuFeedback_m.h"
#include "mecrt/common/MecCommon.h"
#include "mecrt/common/Database.h"
#include "mecrt/common/NodeInfo.h"


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
    int resourceType;  // whether using GPU or CPU
    int service;            // the service name, app type
    omnetpp::simtime_t stopTime;    // the time when the app left the simulation
    double energy;  // the energy consumption for local processing
    double offloadPower;    // the power used for data offloading
};

struct RsuResource {
    int cmpUnits;          // the remaining free computing units in the RSU
    int cmpCapacity;
    int bands;             // remaining free bands in the RSU
    int bandCapacity;
    int resourceType;   // the resource type of the RSU, e.g., GPU
    int deviceType;       // the device type of the RSU
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
    double energySaved;
    double exeTime; // the execution time of the service
    double maxOffloadTime;  // the maximum offloading time results in positive energy saving
};


class SchemeBase;  // forward declaration

class Scheduler : public omnetpp::cSimpleModule
{
  public:
    Database *db_;
    map<AppId, RequestMeta> appInfo_;
    set<AppId> unscheduledApps_;  // the apps that have not been scheduled
    map<MacNodeId, RsuResource> rsuStatus_;
    map<MacNodeId, int> rsuOnholdRbs_;  // {rsuId: onhold resource blocks}, the RSU resource blocks that are on hold
    map<MacNodeId, int> rsuOnholdCus_;  // {rsuId: onhold computing units}, the RSU computing units that are on hold
    map<MacNodeId, set<MacNodeId>> vehAccessRsu_;  // {vehId: set(gnbId)}, accessible RSUs/gNBs for each vehicle
    map<tuple<MacNodeId, MacNodeId>, omnetpp::simtime_t> veh2RsuTime_;  // {(vehId, gnbId): updateTime}
    map<tuple<MacNodeId, MacNodeId>, int> veh2RsuRate_;  // {(vehId, gnbId): bytePerBand}  byte rate per band per TTI
    double ttiPeriod_; // duration for each TTI
    double offloadOverhead_;    // the overhead for offloading
    omnetpp::simtime_t connOutdateInterval_;    /// the interval for the connection (veh to gNB) to be outdated
    int cuStep_ = 1;  // the step for computing units, default is 1
    int rbStep_ = 1;  // the step for resource blocks, default is 1
    double srvTimeScale_; // the scale for app execution time on servers with full resource, default is 1.0
    bool enableBackhaul_ = false; // whether to enable the backhaul network, default is false
    double virtualLinkRate_; // the rate of the virtual link in the backhaul network
    double fairFactor_; // the fairness factor for scheduling scheme with forwarding, default is 1.0
    int maxHops_ = 1; // the maximum number of hops for task forwarding in the backhaul network, default is 1

  protected:
    bool enableInitDebug_;
    inet::UdpSocket socket_;
    int socketId_;

    omnetpp::simsignal_t vecSchedulingTimeSignal_;
    omnetpp::simsignal_t vecSchemeTimeSignal_;
    omnetpp::simsignal_t vecInsGenerateTimeSignal_;
    omnetpp::simsignal_t vecSavedEnergySignal_;
    omnetpp::simsignal_t vecPendingAppCountSignal_;
    omnetpp::simsignal_t vecGrantedAppCountSignal_;

    /*
     * Data Structures
     */
    Binder *binder_;
    SchemeBase *scheme_;  // the scheduling scheme used
    string schemeName_;  // the name of the scheduling scheme

    NodeInfo *nodeInfo_;  // the node information of the scheduler node
    int localPort_;  // the port number used by the scheduler module

	  vector<ServiceInstance> vecSchedule_;	// store the temporary schedule generated by the scheduler
    set<AppId> appsWaitInitFb_; // the apps that are waiting for initialization feedback
    map<MacNodeId, set<AppId>> rsuWaitInitFbApps_; // {rsuId: set(appId)}, apps pending for initialization feedback for each RSU
	  map<AppId, ServiceInstance> srvInInitiating_;	// store the service instance that is initiating
	
	  set<AppId> appsWaitStopFb_; // the apps that are waiting for stop feedback
    set<AppId> allocatedApps_;  // the apps that have been allocated
    map<AppId, ServiceInstance> runningService_;	// store the service instance that is running (a grant feedback has been received)
    
    bool periodicScheduling_;     /// whether to schedule the tasks periodically, if false, then change to event trigger mode
    bool newAppPending_;    /// whether new application has arrived when the schedule scheme is running, used for event trigger mode
	  bool rescheduleAll_;    /// whether to reschedule all the apps (stop those running apps first), used for reset mode
    bool countExeTime_;    /// whether to count the execution time of schedule scheme, default is true

    /// the interval for scheduling the tasks, this interval should be larger than the schedule scheme execution time
    omnetpp::simtime_t schedulingInterval_;   
    omnetpp::simtime_t schedulingStartTime_;   /// the time when the scheduling starts, for resource updating
    omnetpp::simtime_t schedulingTime_;    /// the time for scheduling the tasks (schemeExecTime_ + insGenerateTime_)
    omnetpp::simtime_t schemeExecTime_;    /// the execution time of the scheduling scheme
    omnetpp::simtime_t insGenerateTime_;    /// the time for generating the schedule instances
    omnetpp::simtime_t grantAckInterval_;    /// the interval for sending the grant again
	  omnetpp::simtime_t appStopInterval_;    /// the interval for stopping the running application

    omnetpp::cMessage *schedStarter_;   /// start the scheduling
    omnetpp::cMessage *schedComplete_;    /// inform the completion of scheduling
    omnetpp::cMessage *preSchedCheck_;    /// do the necessary check (stop services) before scheduling
    
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
    void initialize(int stage) override;

    /***
     * Handle the messages
     */
    virtual void handleMessage(omnetpp::cMessage *msg) override;

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
    virtual void scheduleRequest();


    /***
     * Send the grant to the vehicle
     */
    virtual void sendGrant();

    /***
     * Send the grant to the vehicle
     */
    virtual void sendGrantPacket(ServiceInstance& srv, bool isStart, bool isStop);

	/**
	 * stop the running service
	 */
	virtual void stopRunningService(AppId appId);


    /***
     * Update the RSU service initialization feedback
     */
    virtual void updateRsuSrvStatusFeedback(cMessage *msg);

    /**
     * Check if the grant is lost (no feedback from rsu for initialization status)
     */
    virtual void checkLostGrant();

};

#endif
