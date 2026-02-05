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

#include <chrono>

#include "mecrt/apps/scheduler/Scheduler.h"
#include "mecrt/apps/scheduler/SchemeBase.h"
#include "mecrt/apps/scheduler/energy/SchemeFastLR.h"
#include "mecrt/apps/scheduler/energy/SchemeGameTheory.h"
#include "mecrt/apps/scheduler/energy/SchemeIterative.h"
#include "mecrt/apps/scheduler/energy/SchemeSARound.h"
#include "mecrt/apps/scheduler/energy/SchemeFwdGreedy.h"
#include "mecrt/apps/scheduler/energy/SchemeFwdGameTheory.h"
#include "mecrt/apps/scheduler/energy/SchemeFwdQuickLR.h"
#include "mecrt/apps/scheduler/energy/SchemeFwdGraphMatch.h"
#include "mecrt/apps/scheduler/energy/SchemeGreedy.h"
#include "mecrt/apps/scheduler/accuracy/AccuracyGreedy.h"
#include "mecrt/apps/scheduler/accuracy/AccuracyFastSA.h"
#include "mecrt/apps/scheduler/accuracy/AccuracyFastSANF.h"
#include "mecrt/apps/scheduler/accuracy/AccuracyFastSAND.h"
#include "mecrt/apps/scheduler/accuracy/AccuracyGameTheory.h"
#include "mecrt/apps/scheduler/accuracy/AccuracyGraphMatch.h"

#include "mecrt/packets/apps/Grant2Rsu_m.h"
#include "mecrt/packets/apps/ServiceStatus_m.h"
#include "inet/common/socket/SocketTag_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h" // Include the header for L3AddressInd

Define_Module(Scheduler);

Scheduler::Scheduler()
{
    schedStarter_ = nullptr;
    schedComplete_ = nullptr;
    preSchedCheck_ = nullptr;
    enableInitDebug_ = false;

    db_ = nullptr;
    binder_ = nullptr;
    scheme_ = nullptr;
    nodeInfo_ = nullptr;
}

Scheduler::~Scheduler()
{
    if (enableInitDebug_)
        std::cout << "Scheduler::~Scheduler - destroying Scheduler module\n";

    if (scheme_)
    {
        delete scheme_;
        scheme_ = nullptr;
    }

    if (schedStarter_)
    {
        cancelAndDelete(schedStarter_);
        schedStarter_ = nullptr;
    }
    if (schedComplete_)
    {
        cancelAndDelete(schedComplete_);
        schedComplete_ = nullptr;
    }
    if (preSchedCheck_)
    {
        cancelAndDelete(preSchedCheck_);
        preSchedCheck_ = nullptr;
    }

    if (enableInitDebug_)
        std::cout << "Scheduler::~Scheduler - destroying Scheduler module done!\n";
}

void Scheduler::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {
        if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
        if (enableInitDebug_)
            std::cout << "Scheduler::initialize - stage: INITSTAGE_LOCAL - begins" << std::endl;
        
        periodicScheduling_ = par("periodicScheduling");

        schedulingInterval_ = getAncestorPar("scheduleInterval");
        grantAckInterval_ = par("grantAckInterval");
        connOutdateInterval_ = par("connOutdateInterval");

        appStopInterval_ = par("appStopInterval");
        if (appStopInterval_ >= periodicScheduling_)
            appStopInterval_ = periodicScheduling_/2;
        rescheduleAll_ = par("rescheduleAll");
        offloadOverhead_ = par("offloadOverhead");
        cuStep_ = par("cuStep");
        rbStep_ = par("rbStep");
        srvTimeScale_ = par("srvTimeScale");
        countExeTime_ = par("countExeTime");
        enableBackhaul_ = par("enableBackhaul");
        optimizeObjective_ = par("optimizeObjective").stringValue();
        schemeName_ = par("scheduleScheme").stringValue();
        maxHops_ = par("maxHops");
        virtualLinkRate_ = par("virtualLinkRate");
        fairFactor_ = par("fairFactor");

        vecSchedulingTimeSignal_ = registerSignal("schedulingTime");
        vecSchemeTimeSignal_ = registerSignal("schemeTime");
        vecInsGenerateTimeSignal_ = registerSignal("instanceGenerateTime");
        vecUtilitySignal_ = registerSignal("schemeUtility");    // total utility per second of the results
        vecPendingAppCountSignal_ = registerSignal("pendingAppCount");
        vecGrantedAppCountSignal_ = registerSignal("grantedAppCount");
        globalSchedulerReadySignal_ = registerSignal("globalSchedulerReady");
        // the expected number of jobs to be offloaded per second at each scheduling period
        expectedJobsToBeOffloadedSignal_ = registerSignal("expectedJobsToBeOffloaded"); 
        
        WATCH(cuStep_);
        WATCH(rbStep_);
        WATCH(fairFactor_);
        WATCH(schedulingInterval_);
        WATCH(periodicScheduling_);
        WATCH(appStopInterval_);
        WATCH(rescheduleAll_);
        WATCH(countExeTime_);
        WATCH(enableBackhaul_);
        WATCH(optimizeObjective_);
        WATCH(schemeName_);
        WATCH(maxHops_);
        WATCH(virtualLinkRate_);

        if (enableInitDebug_)
            std::cout << "Scheduler::initialize - stage: INITSTAGE_LOCAL - ends" << std::endl;
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        if (enableInitDebug_)
            std::cout << "Scheduler::initialize - stage: INITSTAGE_APPLICATION_LAYER - begins" << std::endl;
        
        localPort_ = par("localPort");
        EV << "vecReceiver::initialize - binding to port: local:" << localPort_ << endl;
        if (localPort_ != -1)
        {
            socket_.setOutputGate(gate("socketOut"));
            socket_.bind(localPort_);
            socketId_ = socket_.getSocketId();
        }

        try
        {
            nodeInfo_ = check_and_cast<NodeInfo*>(getModuleByPath(par("nodeInfoModulePath").stringValue()));
            nodeInfo_->setLocalSchedulerPort(localPort_);
            nodeInfo_->setScheduleInterval(schedulingInterval_.dbl());
            nodeInfo_->setAppStopInterval(appStopInterval_.dbl());
            nodeInfo_->setLocalSchedulerSocketId(socketId_);

            nodeInfo_->setScheduler(this);
        }
        catch (std::exception &e)
        {
            EV_WARN << "Scheduler::initialize - cannot find the NodeInfo module" << endl;
            nodeInfo_ = nullptr;
        }

        db_ = check_and_cast<Database*>(getSimulation()->getModuleByPath("database"));
        if (db_ == nullptr)
            throw cRuntimeError("VecApp::initTraffic - the database module is not found");

        if (enableInitDebug_)
            std::cout << "Scheduler::initialize - stage: INITSTAGE_APPLICATION_LAYER - ends" << std::endl;
    }
    else if (stage == INITSTAGE_LAST){
        if (enableInitDebug_)
            std::cout << "Scheduler::initialize - stage: INITSTAGE_LAST - begins" << std::endl;

        binder_ = getBinder();
        NumerologyIndex numerologyIndex = par("numerologyIndex");
        ttiPeriod_ = binder_->getSlotDurationFromNumerologyIndex(numerologyIndex);

        initializeSchedulingScheme();

        schedStarter_ = new cMessage("ScheduleStart");
        schedStarter_->setSchedulingPriority(1);        // after other messages

        schedComplete_ = new cMessage("ScheduleComplete");
        schedComplete_->setSchedulingPriority(1);        // after other messages

        preSchedCheck_ = new cMessage("PreScheduleCheck");
        preSchedCheck_->setSchedulingPriority(1);        // after other messages

        newAppPending_ = false;
            
        WATCH_SET(allocatedApps_);
        WATCH_SET(unscheduledApps_);
        WATCH_SET(appsWaitInitFb_);
        WATCH(localPort_);
        WATCH(socketId_);
        WATCH_SET(appsWaitStopFb_);

        if (enableInitDebug_)
            std::cout << "Scheduler::initialize - stage: INITSTAGE_LAST - ends" << std::endl;
    }
}

/***
 * handle self message and message from vehicles and rsus
 */ 
void Scheduler::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "ScheduleStart"))
        {
            EV << NOW << " Scheduler::handleMessage - start scheduling\n";
            handleSchedulingStart();
        }
        else if (!strcmp(msg->getName(), "ScheduleComplete"))
        {
            EV << NOW << " Scheduler::handleMessage - scheduling completed, execution time " << schemeExecTime_ << endl;
            sendGrant();
        }
        else if (!strcmp(msg->getName(), "PreScheduleCheck"))   // do necessary service check before next scheduling
        {
            EV << NOW << " Scheduler::handleMessage - pre-scheduling check\n";
            handlePreSchedulingCheck();
        }
    }
    else if (!strcmp(msg->getName(), "SrvReq"))    // request from vehicle
    {
        recordVehRequest(msg);
        delete msg;
        msg = nullptr;
    }
    else if (!strcmp(msg->getName(), "RsuFD"))  // feedback from RSU
    {
        /***
         * TODO stop update until granted service is initialized
         */
        recordRsuStatus(msg);
        delete msg;
        msg = nullptr;
    }
    else if (!strcmp(msg->getName(), "SrvFD"))    // service status report from RSU
    {
        updateRsuSrvStatusFeedback(msg);
        delete msg;
        msg = nullptr;
    }
}


void Scheduler::handlePreSchedulingCheck()
{
    /***
     * The NEXT_SCHEDULING_TIME may be updated by other global scheduler when the whole topology is divided
     * into several parts due to some links or nodes failure. In this case, we need to check if we have reached
     * the latest NEXT_SCHEDULING_TIME - appStopInterval_.
     */
    // commenting out the following code to avoid starvation issue
    // if (simTime() < NEXT_SCHEDULING_TIME - appStopInterval_)
    // {
    //     scheduleAt(NEXT_SCHEDULING_TIME - appStopInterval_, preSchedCheck_);
    //     EV << NOW << " Scheduler::handleMessage - the NEXT_SCHEDULING_TIME has been updated by other global scheduler!\n"
    //         << "\t the updated pre-scheduling check time is not reached, re-schedule at " 
    //         << NEXT_SCHEDULING_TIME - appStopInterval_ << endl;
        
    //     // also reset the schedule start timer
    //     if (schedStarter_->isScheduled())
    //     {
    //         cancelEvent(schedStarter_);
    //         scheduleAt(NEXT_SCHEDULING_TIME, schedStarter_);
    //         EV << NOW << " Scheduler::handleMessage - re-schedule the scheduling start time at " 
    //             << NEXT_SCHEDULING_TIME << endl;
    //     }
    //     return;
    // }
    
    // check if the stop time is reached for the allocated applications
    for (AppId appId : allocatedApps_)
    {
        if (simTime() >= appInfo_[appId].stopTime - appInfo_[appId].period)
        {
            EV << NOW << " Scheduler::stopExpiredApp - stop the expired application " << appId << endl;
            stopService(appId);
        }
    }
    
    if (rescheduleAll_)
    {
        // stop the service in initialization status
        for (AppId appId : appsWaitInitFb_)
            stopService(appId);

        // stop the running service
        for (AppId appId : allocatedApps_)
            stopService(appId);
    }

}


void Scheduler::handleSchedulingStart()
{
    /***
     * TODO: consider how to synchronize multiple global schedulers
     * currently this does not work, as the scheduling time may be updated by other global scheduler
     * this scheduler might never been executed if the scheduling time is always updated to a future time
     *
     * check if the NEXT_SCHEDULING_TIME has been updated by other global scheduler
     * if so, then reschedule the scheduling start time
     */
    // if (simTime() < NEXT_SCHEDULING_TIME)
    // {
    //     scheduleAt(NEXT_SCHEDULING_TIME, schedStarter_);
    //     EV << NOW << " Scheduler::handleMessage - the NEXT_SCHEDULING_TIME has been updated by other global scheduler!\n"
    //         << "\t the scheduled time is not reached, re-schedule at " << NEXT_SCHEDULING_TIME << endl;
    //     return;
    // }
    
    /***
     * if a service stop command is sent, need to wait for the service stop feedback to update the RSU status
     * only perform the next scheduling when all the service stop feedback is received
     * if the feedback is not received, reset the wait list and resend the stop command
     */
    if (appsWaitStopFb_.size() > 0)
        appsWaitStopFb_.clear();    // make sure to resend the stop command when next scheduling starts

    // reset the time
    insGenerateTime_ = SimTime(0, SIMTIME_US);
    schemeExecTime_ = SimTime(0, SIMTIME_US);
    schedulingTime_ = SimTime(0, SIMTIME_US);
    
    removeOutdatedInfo();
    // record the real execution time of the scheduling scheme
    auto start = chrono::steady_clock::now();
    scheduleRequest();
    auto end = chrono::steady_clock::now();
    schedulingTime_ = SimTime(chrono::duration_cast<chrono::microseconds>(end - start).count(), SIMTIME_US);
    emit(vecSchedulingTimeSignal_, schedulingTime_.dbl());

    schemeExecTime_ = schedulingTime_ - insGenerateTime_;
    emit(vecSchemeTimeSignal_, schemeExecTime_.dbl());
    if (schemeExecTime_ < schedulingInterval_ - appStopInterval_)
    {
        if (countExeTime_)
            scheduleAt(simTime()+schemeExecTime_, schedComplete_);
        else
            scheduleAt(simTime(), schedComplete_);
    }
    else
    {
        vecSchedule_.clear();  // clear the schedule if the execution time is too long
    }
        
    if (periodicScheduling_)
    {
        // make sure the NEXT_SCHEDULING_TIME has not been updated by other global scheduler
        // add 1 second margin to avoid starvation
        if (NOW + 1 > NEXT_SCHEDULING_TIME) 
            NEXT_SCHEDULING_TIME = NEXT_SCHEDULING_TIME + schedulingInterval_.dbl();

        scheduleAt(NEXT_SCHEDULING_TIME, schedStarter_);
        scheduleAt(NEXT_SCHEDULING_TIME - appStopInterval_.dbl(), preSchedCheck_);
        EV << NOW << " Scheduler::handleMessage - next scheduling time: " << NEXT_SCHEDULING_TIME << endl;
    }
}


void Scheduler::initializeSchedulingScheme()
{
    if (!enableBackhaul_ && optimizeObjective_ == "energy")
    {
        if (schemeName_ == "Greedy")
            scheme_ = new SchemeGreedy(this);
        else if (schemeName_ == "FastLR")
            scheme_ = new SchemeFastLR(this);
        else if (schemeName_ == "GameTheory")
            scheme_ = new SchemeGameTheory(this);
        else if (schemeName_ == "Iterative")
            scheme_ = new SchemeIterative(this);
        else if (schemeName_ == "SARound")
            scheme_ = new SchemeSARound(this);
        else
            scheme_ = new SchemeBase(this);
    }
    else if (enableBackhaul_ && optimizeObjective_ == "energy")
    {
        if (schemeName_ == "FwdGreedy")
            scheme_ = new SchemeFwdGreedy(this);
        else if (schemeName_ == "FwdGameTheory")
            scheme_ = new SchemeFwdGameTheory(this);
        else if (schemeName_ == "FwdQuickLR")
            scheme_ = new SchemeFwdQuickLR(this);
        else if (schemeName_ == "FwdGraphMatch")
            scheme_ = new SchemeFwdGraphMatch(this);
        else
            scheme_ = new SchemeBase(this);
    }
    else if (enableBackhaul_ && optimizeObjective_ == "accuracy")
    {
        if (schemeName_ == "Greedy")
            scheme_ = new AccuracyGreedy(this);
        else if (schemeName_ == "FastSA")
            scheme_ = new AccuracyFastSA(this);
        else if (schemeName_ == "FastSANF")
            scheme_ = new AccuracyFastSANF(this);
        else if (schemeName_ == "FastSAND")
            scheme_ = new AccuracyFastSAND(this);
        else if (schemeName_ == "GameTheory")
            scheme_ = new AccuracyGameTheory(this);
        else if (schemeName_ == "GraphMatch")
            scheme_ = new AccuracyGraphMatch(this);
        else
            scheme_ = new SchemeBase(this);
    }
    else
    {
        scheme_ = new SchemeBase(this);
    }
}


void Scheduler::globalSchedulerInit()
{
    // this function is called by other modules (the nodeInfo_), so we need to use
    // Enter_Method or Enter_Method_Silent to tell the simulation kernel "switch context to this module"
    Enter_Method("globalSchedulerInit");
    EV << "Scheduler::globalSchedulerInit - do the necessary initialization for global scheduler" << std::endl;

    globalSchedulerReset();

    if (periodicScheduling_)
    {
        // get the time in milliseconds
        double timeNow = int(simTime().dbl() * 1000) / 1000.0;
        NEXT_SCHEDULING_TIME = appStopInterval_.dbl() + timeNow;
        scheduleAt(NEXT_SCHEDULING_TIME, schedStarter_);

        EV << "Scheduler::globalSchedulerInit - next scheduling time: " << NEXT_SCHEDULING_TIME << std::endl;
    }

    emit(globalSchedulerReadySignal_, simTime().dbl());
}


void Scheduler::globalSchedulerReset()
{
    // this function is called by other modules (the nodeInfo_), so we need to use
    // Enter_Method or Enter_Method_Silent to tell the simulation kernel "switch context to this module"
    Enter_Method("globalSchedulerReset");
    EV << "Scheduler::globalSchedulerReset - reset the scheduler status" << std::endl;

    // cancel the scheduled self message
    if (schedStarter_->isScheduled())
        cancelEvent(schedStarter_);
    if (preSchedCheck_->isScheduled())
        cancelEvent(preSchedCheck_);
    if (schedComplete_->isScheduled())
        cancelEvent(schedComplete_);

    rsuStatus_.clear();
    rsuOnholdRbs_.clear();
    rsuOnholdCus_.clear();
    vehAccessRsu_.clear();
    veh2RsuTime_.clear();
    veh2RsuRate_.clear();
    rsuWaitInitFbApps_.clear();
    vecSchedule_.clear();
    srvInInitiating_.clear();
    runningService_.clear();

    // reset all app status
    for (AppId appId : appsWaitInitFb_)
    {
        unscheduledApps_.insert(appId);
    }
    for (AppId appId : allocatedApps_)
    {
        unscheduledApps_.insert(appId);
    }

    appsWaitInitFb_.clear();
    allocatedApps_.clear();
}


void Scheduler::resetNetTopology(const map<MacNodeId, map<MacNodeId, double>>& topology)
{
    Enter_Method("resetNetTopology");
    EV << NOW << " Scheduler::resetNetTopology - reset the backhaul network topology" << endl;
    scheme_->updateReachableRsus(topology);
}


void Scheduler::recordVehRequest(cMessage *msg)
{
    Packet* pkt = check_and_cast<Packet*>(msg);
    auto vecReq = pkt->popAtFront<VecRequest>();

    /***
     * appId is obtained by idToMacCid(srcId, localPort_):
     *          idToMacCid(MacNodeId nodeId, LogicalCid lcid) {return (((MacCid) nodeId << 16) | lcid);}
     * here srcId ~ nodeId, localPort_ ~ lcid
     */
    AppId appId = vecReq->getAppId();
    if (appInfo_.find(appId) != appInfo_.end())
    {
        EV << NOW << " Scheduler::recordVehRequest - request from appId: " << appId << " is already buffered, ignore it!" << endl;
        return;
    }
    MacNodeId vehId = MacCidToNodeId(appId);

    RequestMeta reqMeta;
    reqMeta.inputSize = vecReq->getInputSize();
    reqMeta.outputSize = vecReq->getOutputSize();
    reqMeta.period = vecReq->getPeriod();
    reqMeta.resourceType = vecReq->getResourceType();
    reqMeta.service = vecReq->getService();
    reqMeta.accuracy = vecReq->getAccuracy();
    reqMeta.appId = appId;
    reqMeta.vehId = vehId;
    reqMeta.stopTime = vecReq->getStopTime();
    reqMeta.energy = vecReq->getEnergy();
    reqMeta.offloadPower = vecReq->getOffloadPower();
    reqMeta.ueIpv4Address = vecReq->getUeIpAddress();
    appInfo_[appId] = reqMeta;
    unscheduledApps_.insert(appId);

    EV << NOW << " Scheduler::recordVehRequest - request from Veh[nodeId=" << vehId << "] is received, appId: " << appId
        << ", inputSize: " << reqMeta.inputSize << ", outputSize: " << reqMeta.outputSize
        << ", period: " << reqMeta.period << ", stop time: " <<  reqMeta.stopTime << ", ue address: " 
        << Ipv4Address(reqMeta.ueIpv4Address) <<  ", resourceType: " << reqMeta.resourceType
        << ", service: " << reqMeta.service << endl;

    // if ((!periodicScheduling_) && (!schedStarter_->isScheduled()))  // only for event trigger mode
    // {
    //     if (schedulingCompleted_)   // no schedule scheme is running
    //         scheduleAt(simTime(), schedStarter_);
    //     else                        // schedule scheme is running, what for the completion
    //         newAppPending_ = true;
    // }
}

/***
 * record the RSU status and the connection between vehicle and RSU
 * TODO what if messages come after scheduling (the status changed)
 */
void Scheduler::recordRsuStatus(cMessage *msg)
{
    Packet* pkt = check_and_cast<Packet*>(msg);
    auto rsuStat = pkt->popAtFront<RsuFeedback>();
    simtime_t bandUpdateTime = rsuStat->getBandUpdateTime();
    simtime_t cmpUnitUpdateTime = rsuStat->getCmpUnitUpdateTime();
    MacNodeId gnbId = rsuStat->getGnbId();

    // get gnb module by gnbId
    cModule* gnbMod = binder_->getModuleByMacNodeId(gnbId);
    if (gnbMod == nullptr)
        throw cRuntimeError("Scheduler::recordRsuStatus - cannot find the RSU module by gnbId: %d", gnbId);
    // get the gnb index in the gnb vector
    int gnbIndex = gnbMod->getIndex();

    /***
     * update rsu information
     */
    if(rsuStatus_.find(gnbId) == rsuStatus_.end())  // first time to record the RSU status
    {
        RsuResource rsuRes;
        rsuRes.bands = rsuStat->getAvailBands();
        rsuRes.bandCapacity = rsuStat->getTotalBands();
        rsuRes.cmpUnits = rsuStat->getFreeCmpUnits();
        rsuRes.cmpCapacity = rsuStat->getTotalCmpUnits();
        rsuRes.deviceType = rsuStat->getDeviceType();
        rsuRes.resourceType = rsuStat->getResourceType();
        rsuRes.rsuAddress = Ipv4Address(rsuStat->getRsuAddr());
        rsuRes.bandUpdateTime = bandUpdateTime;
        rsuRes.cmpUpdateTime = cmpUnitUpdateTime;
        rsuStatus_[gnbId] = rsuRes;

        EV << NOW << " Scheduler::recordRsuStatus - RSU[" << gnbIndex << "] nodeId=" << gnbId << " status recorded for the first time, bands: " << rsuRes.bands
        << ", cmpUnits: " << rsuRes.cmpUnits << ", deviceType: " << rsuRes.deviceType
        << ", resourceType: " << rsuRes.resourceType << ", rsuAddress: " << rsuRes.rsuAddress << endl;

        rsuWaitInitFbApps_[gnbId] = set<AppId>();
        // initialize the onhold resource blocks and computing units
        rsuOnholdRbs_[gnbId] = 0;  // no RSU resource blocks are on hold
        rsuOnholdCus_[gnbId] = 0;  // no RSU computing units are on hold
    }
    else
    {
        RsuResource& rsuRes = rsuStatus_[gnbId];
        if (bandUpdateTime > rsuRes.bandUpdateTime)
        {
            rsuRes.bands = rsuStat->getAvailBands();
            rsuRes.bandUpdateTime = bandUpdateTime;

            EV << NOW << " Scheduler::recordRsuStatus - RSU[" << gnbIndex << "] nodeId=" << gnbId << " status updated, bands: " << rsuRes.bands
                << ", bandCapacity: " << rsuRes.bandCapacity << ", deviceType: " << rsuRes.deviceType
                << ", resourceType: " << rsuRes.resourceType << endl;
        }
        else
        {
            EV << NOW << " Scheduler::recordRsuStatus - RSU[" << gnbIndex << "] nodeId=" << gnbId << " bands information is outdated, ignore!" << endl;
        }

        if (cmpUnitUpdateTime > rsuRes.cmpUpdateTime)
        {
            rsuRes.cmpUnits = rsuStat->getFreeCmpUnits();
            rsuRes.cmpUpdateTime = cmpUnitUpdateTime;

            EV << NOW << " Scheduler::recordRsuStatus - RSU[" << gnbIndex << "] nodeId=" << gnbId << " status updated, cmpUnits: " << rsuRes.cmpUnits
                << ", cmpCapacity: " << rsuRes.cmpCapacity << ", deviceType: " << rsuRes.deviceType
                << ", resourceType: " << rsuRes.resourceType << endl;
        }
        else
        {
            EV << NOW << " Scheduler::recordRsuStatus - RSU[" << gnbIndex << "] nodeId=" << gnbId << " cmpUnits information is outdated, ignore!" << endl;
        }
    }

    MacNodeId vehId = rsuStat->getVehId();
    if (vehId == 0) // a status update from node only, no need to update the connection
    {
        EV << NOW << " Scheduler::recordRsuStatus - RSU[" << gnbIndex << "] nodeId=" << gnbId 
            << " status update from node only, no need to update the connection!" << endl;
        return;
    }

    // update the connection between vehicle and rsu
    if (vehAccessRsu_.find(vehId) == vehAccessRsu_.end())
    {
        EV << "\t store the connection between Veh[nodeId=" << vehId << "] and RSU[nodeId=" 
            << gnbId << "] for the first time" << endl;
        vehAccessRsu_[vehId] = {gnbId};
        veh2RsuRate_[make_tuple(vehId, gnbId)] = rsuStat->getBytePerBand();
        veh2RsuTime_[make_tuple(vehId, gnbId)] = bandUpdateTime;
    }
    else
    {
        EV << "\t connection between Veh[nodeId=" << vehId << "] and RSU[nodeId=" 
            << gnbId << "] already exists, update the connection information" << endl;
        vehAccessRsu_[vehId].insert(gnbId);
        veh2RsuRate_[make_tuple(vehId, gnbId)] = rsuStat->getBytePerBand();
        veh2RsuTime_[make_tuple(vehId, gnbId)] = bandUpdateTime;
    }
        
    EV << "\t Veh[nodeId=" << vehId << "] access to RSU[nodeId=" << gnbId << "] updated, bytePerBand(per TTI): "
        << veh2RsuRate_[make_tuple(vehId, gnbId)] << endl;
}


void Scheduler::updateRsuSrvStatusFeedback(cMessage *msg)
{
    auto srvStatus = check_and_cast<Packet*>(msg)->popAtFront<ServiceStatus>();
    AppId appId = srvStatus->getAppId();
    bool success = srvStatus->getSuccess();

    bool isSrvInInitiating = (srvInInitiating_.find(appId) != srvInInitiating_.end());
    if (!isSrvInInitiating && allocatedApps_.find(appId) == allocatedApps_.end())
    {
        EV << NOW << " Scheduler::updateRsuSrvStatusFeedback - application " << appId 
            << " is not in the initiating or allocated list, ignore the feedback!" << endl;
        return;
    }

    MacNodeId processGnbId = srvStatus->getProcessGnbId();
    MacNodeId offloadGnbId = srvStatus->getOffloadGnbId();

    // update rsu resource
    simtime_t bandUpdateTime = srvStatus->getOffloadGnbRbUpdateTime();
    simtime_t cmpUnitUpdateTime = srvStatus->getProcessGnbCuUpdateTime();
    if (bandUpdateTime >= rsuStatus_[offloadGnbId].bandUpdateTime)
    {
        rsuStatus_[offloadGnbId].bands = srvStatus->getAvailBand();
        rsuStatus_[offloadGnbId].bandUpdateTime = bandUpdateTime;
    }

    if (cmpUnitUpdateTime >= rsuStatus_[processGnbId].cmpUpdateTime)
    {
        rsuStatus_[processGnbId].cmpUnits = srvStatus->getAvailCmpUnit();
        rsuStatus_[processGnbId].cmpUpdateTime = cmpUnitUpdateTime;
    }

    EV << NOW << " Scheduler::updateRsuSrvStatusFeedback - offloading RSU[nodeId=" << offloadGnbId 
                << "] updated bands: " << rsuStatus_[offloadGnbId].bands
                << ", processing RSU[nodeId=" << processGnbId << "] updated cmpUnits: " << rsuStatus_[processGnbId].cmpUnits << endl;

    /***
     * Handle service initialization feedback
     */
    if (isSrvInInitiating)
    {
        if (success)    // service initialization success
        {
            // if initialization success, add the app to the allocated list
            allocatedApps_.insert(appId);
            runningService_[appId] = srvInInitiating_[appId];
            EV << "\t service initialization success for application " << appId << endl;
        }
        else    // service initialization failed
        {
            EV << "\t service initialization failed for application " << appId << endl;
            unscheduledApps_.insert(appId);
        }

        // update the RSU onhold resource blocks and computing units
        rsuOnholdRbs_[offloadGnbId] = max(rsuOnholdRbs_[offloadGnbId] - srvInInitiating_[appId].bands, 0);
        rsuOnholdCus_[processGnbId] = max(rsuOnholdCus_[processGnbId] - srvInInitiating_[appId].cmpUnits, 0);
        // remove the application and its service from the waiting list
        appsWaitInitFb_.erase(appId);
        rsuWaitInitFbApps_[offloadGnbId].erase(appId);
        rsuWaitInitFbApps_[processGnbId].erase(appId);
        srvInInitiating_.erase(appId);
    }
    /***
     * Handle service band adjustment feedback
     */
    else
    {
        if (success)    // service band adjustment success
        {
            EV << "\t service band adjustment success for application " << appId << endl;
            // update the running service information
            runningService_[appId].bands = srvStatus->getGrantedBand();
        }
        else
        {
            // check if it is because a stop feedback is received
            if (appsWaitStopFb_.find(appId) != appsWaitStopFb_.end())
            {
                appsWaitStopFb_.erase(appId);
                EV << "\t service stop feedback received for application " << appId << endl;
            }
            else
            {
                EV << "\t service band adjustment failed for granted application " 
                    << appId << ", stop the service!" << endl;
            }
            
            // if initialization failed, the app is put back to unscheduled list
            unscheduledApps_.insert(appId);
            allocatedApps_.erase(appId);
            runningService_.erase(appId);
        }
    }
}

void Scheduler::checkLostGrant()
{
    for(AppId appId : appsWaitInitFb_)
    {
        /**
         * check the grant time, if the grant is not received for a long time, then send the grant again
         */
        ServiceInstance& srv = srvInInitiating_[appId];
        if (simTime() - srv.srvGrantTime > grantAckInterval_)
        {
            EV << NOW << " Scheduler::checkLostGrant - grant feedback lost for application, resend grant" << appId << endl;
            sendGrantPacket(srv, true, false);
        }
    }
}


void Scheduler::scheduleRequest()
{
    map<AppId, double> appUtilityMap = map<AppId, double>();
    if (unscheduledApps_.size() == 0)
    {
        EV << NOW << " Scheduler::scheduleRequest - no request to schedule" << endl;
        db_->addGrantedAppInfo(appUtilityMap);
        emit(vecPendingAppCountSignal_, 0);
        return;
    }

    emit(vecPendingAppCountSignal_, int(unscheduledApps_.size()));

    vecSchedule_.clear();
    double totalUtility = 0;
    double totalOffloadCount = 0;

    // record the time for generating the schedule instances
    auto start = chrono::steady_clock::now();
    scheme_->generateScheduleInstances();
    auto end = chrono::steady_clock::now();
    insGenerateTime_ = SimTime(chrono::duration_cast<chrono::microseconds>(end - start).count(), SIMTIME_US);
    emit(vecInsGenerateTimeSignal_, insGenerateTime_.dbl());

    vector<srvInstance> selectedIns = scheme_->scheduleRequests();
    for (srvInstance ins : selectedIns)
    {
        AppId appId = get<0>(ins);
        MacNodeId offloadGnbId = get<1>(ins);
        MacNodeId processGnbId = get<2>(ins);
        
        ServiceInstance srv;
        srv.appId = appId;
        srv.offloadGnbId = offloadGnbId;
        srv.processGnbId = processGnbId;
        srv.bands = get<3>(ins);
        srv.cmpUnits = get<4>(ins);
        srv.exeTime = scheme_->getAppExeDelay(appId);
        srv.utility = scheme_->getAppUtility(appId);
        srv.serviceType = scheme_->getAppAssignedService(appId);

        if (srv.utility <= 0)
        {
            // add time as well
            throw cRuntimeError("%f Scheduler::scheduleRequest - application %d has 0 utility, please check the scheduling scheme", simTime().dbl(), appId);
        }

        double appMaxoffloadTime = scheme_->getMaxOffloadTime(appId);
        if (appMaxoffloadTime <= 0)
        {
            throw cRuntimeError("%f Scheduler::scheduleRequest - application %d has 0 max offload time, please check the scheduling scheme", simTime().dbl(), appId);
        }

        srv.maxOffloadTime = appMaxoffloadTime;
        // determine the offloading delay result in positive energy saving
        if (optimizeObjective_ == "energy")
        {
            double energyMaxOffloadTime = appInfo_[appId].energy / appInfo_[appId].offloadPower;
            srv.maxOffloadTime = min(energyMaxOffloadTime, appMaxoffloadTime);
        }
        
        vecSchedule_.push_back(srv);
        appUtilityMap[appId] = srv.utility;
        totalUtility += srv.utility;
        totalOffloadCount += 1.0 / appInfo_[appId].period.dbl();
    }

    db_->addGrantedAppInfo(appUtilityMap);

    int grantedAppCount = vecSchedule_.size();
    if (!rescheduleAll_)
    {
        grantedAppCount += allocatedApps_.size();
        for (AppId appId : allocatedApps_)
        {
            totalUtility += runningService_[appId].utility;
        }
    }
    
    emit(vecGrantedAppCountSignal_, grantedAppCount);
    emit(vecUtilitySignal_, totalUtility);
    emit(expectedJobsToBeOffloadedSignal_, totalOffloadCount);
}


void Scheduler::removeOutdatedInfo()
{
    EV << NOW << " Scheduler::removeOutdatedInfo - remove any expired request and outdated UE-GNB connection info" << endl;
    
    // ======== remove the expired request ============
    // check the unscheduled request, if the vehicle left the simulation or the stop time is reached, remove the request
    set<AppId> toRemove = set<AppId>();
    for (AppId appId : unscheduledApps_)
    {
        MacNodeId vehId = appInfo_[appId].vehId;
        if (binder_->getOmnetId(vehId) == 0)    // check if the vehicle left the simulation or not
        {
            EV << NOW << " Scheduler::scheduleRequest - vehicle[nodeId=" << vehId << "] left the simulation, remove the request" << endl;
            toRemove.insert(appId);
            continue;
        }
        simtime_t stopTime = appInfo_[appId].stopTime;
        simtime_t period = appInfo_[appId].period;
        if (period <= 0)
        {
            toRemove.insert(appId);
            EV << NOW << " Scheduler::scheduleRequest - application " << appId << " has non-positive period, remove the request" << endl;
            continue;
        }
        simtime_t gap = max(period, schedulingInterval_);
        if (simTime() >= (stopTime - gap))  // if the stop time is reached
        {
            EV << NOW << " Scheduler::scheduleRequest - application " << appId << " stop time reached, remove the request" << endl;
            toRemove.insert(appId);
        }
    }
    for (AppId appId : toRemove)
    {
        unscheduledApps_.erase(appId);
        appInfo_.erase(appId);
    }

    // ======== remove the outdated UE-RSU connection ============
    map<MacNodeId, set<MacNodeId>> vehAccessRsuCopy = vehAccessRsu_;
    for (auto const&vr : vehAccessRsuCopy)
    {
        MacNodeId vehId = vr.first;
        for (MacNodeId rsuId : vr.second)
        {
            auto link = make_tuple(vehId, rsuId);
            if ((simTime() - veh2RsuTime_[link] > connOutdateInterval_) || (veh2RsuRate_[link] <= 0))
            {
                EV << NOW << " Scheduler::removeOutdatedInfo - connection between vehicle[nodeId=" << vehId 
                    << "] and RSU[nodeId=" << rsuId << "] expired, remove the connection info" << endl;
                veh2RsuRate_.erase(link);
                veh2RsuTime_.erase(link);
                vehAccessRsu_[vehId].erase(rsuId);
            }
        }

        if (vehAccessRsu_[vehId].empty())
            vehAccessRsu_.erase(vehId);
    }

    // ======== remove the RSU that is outdated ============
    // set<MacNodeId> rsuToRemove = set<MacNodeId>();
    for (auto &res : rsuStatus_)
    {        
        MacNodeId rsuId = res.first;
        simtime_t lastBandUpdateTime = res.second.bandUpdateTime;
        simtime_t lastCmpUpdateTime = res.second.cmpUpdateTime;

        if (simTime() - lastBandUpdateTime > 2 * appStopInterval_)
            res.second.bands = 0;  // the NIC may be turned off, so set the bands to 0

        if (simTime() - lastCmpUpdateTime > 2 * appStopInterval_)
            res.second.cmpUnits = 0;  // the computing unit may be turned off, so set the cmpUnits to 0

        // if ((simTime() - lastBandUpdateTime > 2 * appStopInterval_) && (simTime() - lastCmpUpdateTime > 2 * appStopInterval_))
        // {
        //     EV << NOW << " Scheduler::removeOutdatedInfo - RSU[nodeId=" << rsuId << "] status expired, remove the RSU info" << endl;
        //     rsuToRemove.insert(rsuId);
        // }
    }

    // for (MacNodeId rsuId : rsuToRemove)
    // {
    //     rsuStatus_.erase(rsuId);
    // }
}


void Scheduler::sendGrant()
{
    if(vecSchedule_.empty())
        return;

    for(ServiceInstance& srv : vecSchedule_)
    {
        MacNodeId processGnbId = srv.processGnbId;
        MacNodeId offloadGnbId = srv.offloadGnbId;
        AppId appId = srv.appId;

        if (rsuStatus_[offloadGnbId].bands < srv.bands)
        {
            EV << NOW << " Scheduler::sendGrant - RSU[nodeId=" << offloadGnbId 
                << "] does not have enough resource blocks for app " << appId << endl;
            continue;
        }

        if (rsuStatus_[processGnbId].cmpUnits < srv.cmpUnits)
        {
            EV << NOW << " Scheduler::sendGrant - RSU[nodeId=" << processGnbId 
                << "] does not have enough computing units for app " << appId << endl;
            continue;
        }

        EV << NOW << " Scheduler::sendGrant - service for application " << appId << " is granted" << endl;

        sendGrantPacket(srv, true, false);

        // update pending task list and allocated task list
        unscheduledApps_.erase(appId);
        // tasks being scheduled, wait for initialization completion
        appsWaitInitFb_.insert(appId);
        srv.srvGrantTime = simTime();
        srvInInitiating_[appId] = srv;
        rsuWaitInitFbApps_[processGnbId].insert(appId);
        rsuWaitInitFbApps_[offloadGnbId].insert(appId);
        rsuOnholdRbs_[offloadGnbId] = min(rsuOnholdRbs_[offloadGnbId] + srvInInitiating_[appId].bands, rsuStatus_[offloadGnbId].bands);
        rsuOnholdCus_[processGnbId] = min(rsuOnholdCus_[processGnbId] + srvInInitiating_[appId].cmpUnits, rsuStatus_[processGnbId].cmpUnits);
    }

    vecSchedule_.clear();
}

void Scheduler::sendGrantPacket(ServiceInstance& srv, bool isStart, bool isStop)
{
    MacNodeId processGnbId = srv.processGnbId;
    MacNodeId offloadGnbId = srv.offloadGnbId;
    AppId appId = srv.appId;
    
    Packet* pkt = new Packet("SrvGrant");
    auto grant = makeShared<Grant2Rsu>();
    grant->setAppId(appId);
    grant->setUeAddr(appInfo_[appId].ueIpv4Address);
    grant->setOffloadGnbId(offloadGnbId);
    grant->setOffloadGnbAddr(rsuStatus_[offloadGnbId].rsuAddress.getInt());
    grant->setProcessGnbId(processGnbId);
    grant->setResourceType(appInfo_[appId].resourceType.c_str());
    grant->setService(srv.serviceType.c_str());
    grant->setCmpUnits(srv.cmpUnits);
    grant->setBands(srv.bands);
    grant->setDeadline(appInfo_[appId].period);
    grant->setOutputSize(appInfo_[appId].outputSize);
    grant->setInputSize(appInfo_[appId].inputSize);
    grant->setStart(isStart);
    grant->setStop(isStop);
    grant->setExeTime(srv.exeTime);
    grant->setMaxOffloadTime(srv.maxOffloadTime);
    grant->setUtility(srv.utility);
    
    pkt->insertAtBack(grant);

    EV << NOW << " Scheduler::sendGrantPacket - send grant packet to RSU[nodeId=" << processGnbId 
        << "], appId: " << appId << ", processGnbId: " << processGnbId << ", offloadGnbId: " << offloadGnbId
        << ", cmpUnits: " << srv.cmpUnits << ", bands: " << srv.bands
        << ", exeTime: " << srv.exeTime << ", maxOffloadTime: " << srv.maxOffloadTime
        << ", resourceType: " << appInfo_[appId].resourceType << ", service: " << srv.serviceType
        << ", utility: " << srv.utility << endl;

    // check if the processing server is the local server
    Ipv4Address processGnbAddr = rsuStatus_[processGnbId].rsuAddress;
    if (processGnbAddr == nodeInfo_->getNodeAddr())  // the processing RSU is the local RSU
    {
        EV << NOW << " Scheduler::sendGrantPacket - the processing RSU is the local RSU, send to local processing module" << endl;
        pkt->addTagIfAbsent<SocketInd>()->setSocketId(nodeInfo_->getServerSocketId());
        send(pkt, "socketOut");
        return;
    }
    socket_.sendTo(pkt, processGnbAddr, MEC_NPC_PORT);
}

void Scheduler::stopService(AppId appId)
{
    EV << NOW << " Scheduler::stopService - stop the service for application " << appId << endl;

    if (appsWaitStopFb_.find(appId) != appsWaitStopFb_.end())
    {
        EV << NOW << " Scheduler::stopService - application " << appId 
            << " is already in the waiting stop feedback list, ignore the stop request!" << endl;
        return;
    }

    if (allocatedApps_.find(appId) != allocatedApps_.end())
    {
        sendGrantPacket(runningService_[appId], false, true);
    }
    else if (appsWaitInitFb_.find(appId) != appsWaitInitFb_.end())
    {
        sendGrantPacket(srvInInitiating_[appId], false, true);
    }
    else
    {
        EV << NOW << " Scheduler::stopService - application " << appId 
            << " is neither in allocatedApps_ nor in appsWaitInitFb_, cannot stop the service!" << endl;
        return;
    }
    
    appsWaitStopFb_.insert(appId);
}


void Scheduler::finish()
{

}
