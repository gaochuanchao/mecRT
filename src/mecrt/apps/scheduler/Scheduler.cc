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
#include "mecrt/apps/scheduler/SchemeFastLR.h"
#include "mecrt/apps/scheduler/SchemeGameTheory.h"
#include "mecrt/apps/scheduler/SchemeIterative.h"
#include "mecrt/apps/scheduler/SchemeSARound.h"
#include "mecrt/apps/scheduler/SchemeFwdBase.h"
#include "mecrt/apps/scheduler/SchemeFwdGameTheory.h"
#include "mecrt/apps/scheduler/SchemeFwdQuickLR.h"
#include "mecrt/apps/scheduler/SchemeFwdGraphMatch.h"

#include "mecrt/packets/apps/Grant2Rsu_m.h"
#include "mecrt/packets/apps/ServiceStatus_m.h"
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
        virtualLinkRate_ = par("virtualLinkRate");
        fairFactor_ = par("fairFactor");

        vecSchedulingTimeSignal_ = registerSignal("schedulingTime");
        vecSchemeTimeSignal_ = registerSignal("schemeTime");
        vecInsGenerateTimeSignal_ = registerSignal("instanceGenerateTime");
        vecSavedEnergySignal_ = registerSignal("savedEnergy");
        vecPendingAppCountSignal_ = registerSignal("pendingAppCount");
        vecGrantedAppCountSignal_ = registerSignal("grantedAppCount");

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

        try
        {
            nodeInfo_ = check_and_cast<NodeInfo*>(getModuleByPath(par("nodeInfoModulePath").stringValue()));
            nodeInfo_->setLocalSchedulerPort(localPort_);
            nodeInfo_->setScheduleInterval(schedulingInterval_.dbl());
        }
        catch (std::exception &e)
        {
            EV_WARN << "Scheduler::initialize - cannot find the NodeInfo module" << endl;
            nodeInfo_ = nullptr;
        }

        binder_ = getBinder();
        NumerologyIndex numerologyIndex = par("numerologyIndex");
        ttiPeriod_ = binder_->getSlotDurationFromNumerologyIndex(numerologyIndex);

        schemeName_ = par("scheduleScheme").stringValue();

        if (!enableBackhaul_)
        {
            if (schemeName_ == "Greedy")
                scheme_ = new SchemeBase(this);
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
        else
        {
            if (schemeName_ == "FwdGreedy")
                scheme_ = new SchemeFwdBase(this);
            else if (schemeName_ == "FwdGameTheory")
                scheme_ = new SchemeFwdGameTheory(this);
            else if (schemeName_ == "FwdQuickLR")
                scheme_ = new SchemeFwdQuickLR(this);
            else if (schemeName_ == "FwdGraphMatch")
                scheme_ = new SchemeFwdGraphMatch(this);
            else
                scheme_ = new SchemeFwdBase(this);
        }

        schedStarter_ = new cMessage("ScheduleStart");
        schedStarter_->setSchedulingPriority(1);        // after other messages

        schedComplete_ = new cMessage("ScheduleComplete");
        schedComplete_->setSchedulingPriority(1);        // after other messages

        preSchedCheck_ = new cMessage("PreScheduleCheck");
        preSchedCheck_->setSchedulingPriority(1);        // after other messages

        newAppPending_ = false;

        // if (periodicScheduling_)
        // {
        //     scheduleAt(simTime() + schedulingInterval_, schedStarter_);
        //     scheduleAt(simTime() + schedulingInterval_ - appStopInterval_, preSchedCheck_);
        // }
            
        WATCH_SET(allocatedApps_);
        WATCH_SET(unscheduledApps_);
        WATCH_SET(appsWaitInitFb_);
        WATCH(schemeName_);
        WATCH(cuStep_);
        WATCH(rbStep_);
        WATCH(fairFactor_);
        WATCH(localPort_);
        WATCH(schedulingInterval_);

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
            /***
             * if a service stop command is sent, need to wait for the service stop feedback to update the RSU status
             * only perform the next scheduling when all the service stop feedback is received
             * if the feedback is not received, reset the wait list and resend the stop command
             */
            if (appsWaitStopFb_.size() == 0)
            {
                // reset the time
                insGenerateTime_ = SimTime(0, SIMTIME_US);
                schemeExecTime_ = SimTime(0, SIMTIME_US);
                schedulingTime_ = SimTime(0, SIMTIME_US);
                
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
                    vecSchedule_.clear();  // clear the schedule if the execution time is too long
            }
            else
            {
                appsWaitStopFb_.clear();
            }

            if (periodicScheduling_)
            {
                NEXT_SCHEDULING_TIME = simTime() + schedulingInterval_;
                scheduleAt(NEXT_SCHEDULING_TIME, schedStarter_);
                scheduleAt(NEXT_SCHEDULING_TIME - appStopInterval_, preSchedCheck_);
                EV << NOW << " Scheduler::handleMessage - next scheduling time: " << NEXT_SCHEDULING_TIME << endl;
            }
        }
        else if (!strcmp(msg->getName(), "ScheduleComplete"))
        {
            EV << NOW << " Scheduler::handleMessage - scheduling completed, execution time " << schemeExecTime_ << endl;
            sendGrant();
        }
        else if (!strcmp(msg->getName(), "PreScheduleCheck"))   // do necessary service check before next scheduling
        {
            // check if the stop time is reached for the allocated applications
            for (AppId appId : allocatedApps_)
            {
                if (simTime() >= appInfo_[appId].stopTime - appInfo_[appId].period)
                {
                    EV << NOW << " Scheduler::stopExpiredApp - stop the expired application " << appId << endl;
                    stopRunningService(appId);
                }
            }
            
            if (rescheduleAll_)
            {
                // stop the service in initialization status
                for (AppId appId : appsWaitInitFb_)
                {
                    stopRunningService(appId);
                }

                // stop the running service
                for (AppId appId : allocatedApps_)
                {
                    // check if the app is already in the waiting list
                    if (appsWaitStopFb_.find(appId) == appsWaitStopFb_.end())
                        stopRunningService(appId);
                }
            }
        }
    }
    else if (!strcmp(msg->getName(), "SrvReq"))    // request from vehicle
    {
        recordVehRequest(msg);
        delete msg;
    }
    else if (!strcmp(msg->getName(), "RsuFD"))  // feedback from RSU
    {
        /***
         * TODO stop update until granted service is initialized
         */
        recordRsuStatus(msg);
        delete msg;
    }
    else if (!strcmp(msg->getName(), "SrvFD"))    // service status report from RSU
    {
        updateRsuSrvStatusFeedback(msg);
        delete msg;
    }
}


void Scheduler::globalSchedulerInit()
{
    EV << "Scheduler::globalSchedulerInit - do the necessary initialization for global scheduler" << std::endl;

    if (periodicScheduling_)
    {
        NEXT_SCHEDULING_TIME = simTime() + appStopInterval_;
        scheduleAt(NEXT_SCHEDULING_TIME, schedStarter_);
        scheduleAt(simTime(), preSchedCheck_);

        EV << "Scheduler::globalSchedulerInit - next scheduling time: " << NEXT_SCHEDULING_TIME << std::endl;
    }
}


void Scheduler::globalSchedulerFinish()
{
    EV << "Scheduler::globalSchedulerFinish - do the necessary finishing operation for previous global scheduler" << std::endl;

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
    MacNodeId vehId = MacCidToNodeId(appId);

    RequestMeta reqMeta;
    reqMeta.inputSize = vecReq->getInputSize();
    reqMeta.outputSize = vecReq->getOutputSize();
    reqMeta.period = vecReq->getPeriod();
    reqMeta.resourceType = static_cast<VecResourceType>(vecReq->getResourceType());
    reqMeta.service = static_cast<VecServiceType>(vecReq->getService());
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
        << ", period: " << reqMeta.period << ", stop time: " <<  reqMeta.stopTime <<  ", resourceType: " 
        << db_->resourceType.at(reqMeta.resourceType)
        << ", service: " << db_->appType.at(reqMeta.service) << endl;

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
    MacNodeId vehId = rsuStat->getVehId();

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
        rsuRes.deviceType = static_cast<VecDeviceType>(rsuStat->getDeviceType());
        rsuRes.resourceType = static_cast<VecResourceType>(rsuStat->getResourceType());
        rsuRes.bandUpdateTime = bandUpdateTime;
        rsuRes.cmpUpdateTime = cmpUnitUpdateTime;
        rsuStatus_[gnbId] = rsuRes;

        // update rsu address related information
        RsuAddr addr;
        auto l3Addr = pkt->getTag<L3AddressInd>();
        addr.rsuAddress = l3Addr->getSrcAddress().toIpv4();
        addr.serverPort = rsuStat->getServerPort();
        rsuAddr_[gnbId] = addr;

        EV << NOW << " Scheduler::addRsuAddr - add new RSU address, RSU[" << gnbIndex << "] nodeId=" << gnbId << " address: "
            << addr.rsuAddress.str() << ":" << addr.serverPort << endl;

        EV << NOW << " Scheduler::recordRsuStatus - RSU[" << gnbIndex << "] nodeId=" << gnbId << " status recorded for the first time, bands: " << rsuRes.bands
        << ", cmpUnits: " << rsuRes.cmpUnits << ", deviceType: " << db_->deviceType.at(rsuRes.deviceType)
        << ", resourceType: " << db_->resourceType.at(rsuRes.resourceType) << endl;

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
                << ", bandCapacity: " << rsuRes.bandCapacity << ", deviceType: " << db_->deviceType.at(rsuRes.deviceType)
                << ", resourceType: " << db_->resourceType.at(rsuRes.resourceType) << endl;
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
                << ", cmpCapacity: " << rsuRes.cmpCapacity << ", deviceType: " << db_->deviceType.at(rsuRes.deviceType)
                << ", resourceType: " << db_->resourceType.at(rsuRes.resourceType) << endl;
        }
        else
        {
            EV << NOW << " Scheduler::recordRsuStatus - RSU[" << gnbIndex << "] nodeId=" << gnbId << " cmpUnits information is outdated, ignore!" << endl;
        }
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

void Scheduler::addRsuAddr(inet::Ptr<const RsuFeedback> rsuStat)
{
    MacNodeId gnbId = rsuStat->getGnbId();

    // check if the RSU address is already in the list
    if (rsuAddr_.find(gnbId) == rsuAddr_.end())
    {
        RsuAddr addr;
        // binder_->getModuleNameByMacNodeId(gnbId) -> "VehicularEdgeComputing.gnb[0]"
        addr.rsuAddress = inet::L3AddressResolver().resolve(binder_->getModuleNameByMacNodeId(gnbId)).toIpv4();
        addr.serverPort = rsuStat->getServerPort();
        rsuAddr_[gnbId] = addr;

        EV << "Scheduler::addRsuAddr - add new RSU address, RSU[nodeId=" << gnbId << "] address: "
            << addr.rsuAddress.str() << ":" << addr.serverPort << endl;
    }
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
    if (unscheduledApps_.size() == 0)
    {
        EV << NOW << " Scheduler::scheduleRequest - no request to schedule" << endl;

        emit(vecPendingAppCountSignal_, 0);
        return;
    }

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
        }
        simtime_t stopTime = appInfo_[appId].stopTime;
        simtime_t period = appInfo_[appId].period;
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

    emit(vecPendingAppCountSignal_, int(unscheduledApps_.size()));

    vecSchedule_.clear();
    double totalEnergy = 0;

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
        srv.exeTime = scheme_->computeExeDelay(appId, processGnbId, srv.cmpUnits);
        srv.energySaved = scheme_->getAppUtility(appId);

        if (srv.energySaved <= 0)
        {
            // add time as well
            throw cRuntimeError("%f Scheduler::scheduleRequest - application %d has 0 utility, please check the scheduling scheme", simTime().dbl(), appId);
        }

        double appMaxoffloadTime = scheme_->getMaxOffloadTime(appId);
        if (appMaxoffloadTime <= 0)
        {
            throw cRuntimeError("%f Scheduler::scheduleRequest - application %d has 0 max offload time, please check the scheduling scheme", simTime().dbl(), appId);
        }

        // determine the offloading delay result in positive energy saving
        double energyMaxOffloadTime = appInfo_[appId].energy / appInfo_[appId].offloadPower;
        srv.maxOffloadTime = min(energyMaxOffloadTime, appMaxoffloadTime);

        vecSchedule_.push_back(srv);
        totalEnergy += srv.energySaved * schedulingInterval_.dbl();
    }

    int grantedAppCount = vecSchedule_.size();

    if (!rescheduleAll_)
    {
        grantedAppCount += allocatedApps_.size();
        for (AppId appId : allocatedApps_)
        {
            totalEnergy += runningService_[appId].energySaved * schedulingInterval_.dbl();
        }
    }
    
    emit(vecGrantedAppCountSignal_, grantedAppCount);
    emit(vecSavedEnergySignal_, totalEnergy);
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
    grant->setOffloadGnbId(offloadGnbId);
    grant->setProcessGnbId(processGnbId);
    grant->setResourceType(appInfo_[appId].resourceType);
    grant->setService(appInfo_[appId].service);
    grant->setCmpUnits(srv.cmpUnits);
    grant->setBands(srv.bands);
    grant->setDeadline(appInfo_[appId].period);
    grant->setOutputSize(appInfo_[appId].outputSize);
    grant->setInputSize(appInfo_[appId].inputSize);
    grant->setStart(isStart);
    grant->setStop(isStop);
    grant->setExeTime(srv.exeTime);
    grant->setMaxOffloadTime(srv.maxOffloadTime);
    
    pkt->insertAtBack(grant);

    EV << NOW << " Scheduler::sendGrantPacket - send grant packet to RSU[nodeId=" << processGnbId 
        << "], appId: " << appId << ", processGnbId: " << processGnbId << ", offloadGnbId: " << offloadGnbId
        << ", cmpUnits: " << srv.cmpUnits << ", bands: " << srv.bands
        << ", exeTime: " << srv.exeTime << ", maxOffloadTime: " << srv.maxOffloadTime
        << ", resourceType: " << db_->resourceType.at(appInfo_[appId].resourceType)
        << ", service: " << db_->appType.at(appInfo_[appId].service) 
        << endl;

    auto addr = rsuAddr_[processGnbId];
    socket_.sendTo(pkt, addr.rsuAddress, addr.serverPort);
}

void Scheduler::stopRunningService(AppId appId)
{
    EV << NOW << " Scheduler::stopRunningService - stop the service for application " << appId << endl;
    sendGrantPacket(runningService_[appId], false, true);

    appsWaitStopFb_.insert(appId);
}


void Scheduler::finish()
{

}
