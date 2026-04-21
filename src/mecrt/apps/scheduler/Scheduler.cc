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
#include "mecrt/apps/scheduler/distAccuracy/AccuracyDistIS.h"
#include "mecrt/apps/scheduler/accuracyNF/AccuracyFastIS.h"
#include "mecrt/apps/scheduler/accuracyNF/AccuracySARound.h"
#include "mecrt/apps/scheduler/accuracyNF/AccuracyIterative.h"
#include "mecrt/apps/scheduler/accuracyNF/AccuracyIDAssign.h"

#include "mecrt/packets/apps/Grant2Rsu_m.h"
#include "mecrt/packets/apps/ServiceStatus_m.h"
#include "inet/common/socket/SocketTag_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h" // Include the header for L3AddressInd
#include <inet/linklayer/common/InterfaceTag_m.h>

#include <numeric>


Define_Module(Scheduler);

Scheduler::Scheduler()
{
    schedStarter_ = nullptr;
    schedComplete_ = nullptr;
    preSchedCheck_ = nullptr;
    postBatchSchedule_ = nullptr;
    distInstGenTimer_ = nullptr;

    enableInitDebug_ = false;

    db_ = nullptr;
    binder_ = nullptr;
    scheme_ = nullptr;
    nodeInfo_ = nullptr;

    rsuId_ = 0;
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
    if (postBatchSchedule_)
    {
        cancelAndDelete(postBatchSchedule_);
        postBatchSchedule_ = nullptr;
    }
    if (distInstGenTimer_)
    {
        cancelAndDelete(distInstGenTimer_);
        distInstGenTimer_ = nullptr;
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
        grantAckInterval_ = par("grantAckInterval");

        faultRecoveryMargin_ = par("faultRecoveryMargin");
        rescheduleAll_ = par("rescheduleAll");
        offloadOverhead_ = par("offloadOverhead");
        cuStep_ = par("cuStep");
        rbStep_ = par("rbStep");
        resourceSlack_ = par("resourceSlack");
        srvTimeScale_ = par("srvTimeScale");
        enableBackhaul_ = par("enableBackhaul");
        optimizeObjective_ = par("optimizeObjective").stringValue();
        schemeName_ = par("scheduleScheme").stringValue();
        maxHops_ = par("maxHops");
        virtualLinkRate_ = par("virtualLinkRate");
        fairFactor_ = par("fairFactor");

        vecSchedulingTimeSignal_ = registerSignal("schedulingTime");
        vecSchemeTimeSignal_ = registerSignal("schemeTime");
        vecInsGenerateTimeSignal_ = registerSignal("instanceGenerateTime");
        vecDistSchemeExecTimeSignal_ = registerSignal("distSchemeExecTime");
        vecUtilitySignal_ = registerSignal("schemeUtility");    // total utility per second of the results
        vecPendingAppCountSignal_ = registerSignal("pendingAppCount");
        vecGrantedAppCountSignal_ = registerSignal("grantedAppCount");
        globalSchedulerReadySignal_ = registerSignal("globalSchedulerReady");
        // the expected number of jobs to be offloaded per second at each scheduling period
        expectedJobsToBeOffloadedSignal_ = registerSignal("expectedJobsToBeOffloaded");
        
        WATCH(cuStep_);
        WATCH(rbStep_);
        WATCH(fairFactor_);
        WATCH(resourceSlack_);
        WATCH(schedulingInterval_);
        WATCH(periodicScheduling_);
        WATCH(appStopInterval_);
        WATCH(appFeedbackInterval_);
        WATCH(rescheduleAll_);
        WATCH(enableBackhaul_);
        WATCH(optimizeObjective_);
        WATCH(schemeName_);
        WATCH(maxHops_);
        WATCH(virtualLinkRate_);
        WATCH(rsuId_);

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
            nodeInfo_ = getModuleFromPar<NodeInfo>(par("nodeInfoModulePath"), this);
            nodeInfo_->setLocalSchedulerPort(localPort_);
            nodeInfo_->setLocalSchedulerSocketId(socketId_);

            nodeInfo_->setScheduler(this);

            rsuId_ = nodeInfo_->getNodeId();
            enableDistScheme_ = nodeInfo_->getEnableDistScheme();

            schedulingInterval_ = nodeInfo_->getScheduleInterval();
            appStopInterval_ = nodeInfo_->getAppStopInterval();
            appFeedbackInterval_ = nodeInfo_->getAppFeedbackInterval();

            if (enableDistScheme_)
            {
                pv2Tokens_.clear();
                pvCounter_.clear();
                distUnscheduledApps_.clear();
                distBatchTimes_.clear();
                pvMax_ = 0;
                pvMin_ = 0;
                distributedSchemeStarted_ = false;
                batchSchedulingOngoing_ = false;
                targetCategory_ = "LI"; // default to LI category for candidate selection

                WATCH(distStage_);
                WATCH(pvMax_);
                WATCH(pvMin_);
                WATCH(targetPV_);
                WATCH(targetCategory_);
                WATCH(distributedSchemeStarted_);
                WATCH_MAP(pvCounter_);
                WATCH_SET(distUnscheduledApps_);
            }
        }
        catch (std::exception &e)
        {
            throw cRuntimeError("Scheduler::initialize - failed to get nodeInfo module: %s", e.what());
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

        postBatchSchedule_ = new cMessage("PostBatchSchedule");
        postBatchSchedule_->setSchedulingPriority(1);        // after other messages

        distInstGenTimer_ = new cMessage("DistInstGenTimer");
        distInstGenTimer_->setSchedulingPriority(1);        // after other messages

        newAppPending_ = false;
            
        WATCH_SET(pendingScheduleApps_);
        WATCH_SET(allocatedApps_);
        WATCH_SET(unscheduledApps_);
        WATCH_SET(appsWaitInitFb_);
        WATCH(localPort_);
        WATCH(socketId_);
        WATCH_SET(appsWaitStopFb_);
        WATCH(enableDistScheme_);

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
            EV << NOW << " Scheduler::handleMessage - scheduling start\n";
            updateNextSchedulingTime(); // schedule the next scheduling time

            // reset the time
            insGenerateTime_ = SimTime(0, SIMTIME_US);
            schemeExecTime_ = SimTime(0, SIMTIME_US);
            schedulingTime_ = SimTime(0, SIMTIME_US);
            
            if (!enableDistScheme_)
                handleCentralizedScheduling();
            else
                handleDistributedScheduling();
        }
        else if (!strcmp(msg->getName(), "ScheduleComplete"))
        {
            EV << NOW << " Scheduler::handleMessage - scheduling completed, time spend: " << schedulingTime_ << endl;
            sendGrant();
        }
        else if (!strcmp(msg->getName(), "PreScheduleCheck"))   // do necessary service check before next scheduling
        {
            EV << NOW << " Scheduler::handleMessage - pre-scheduling check\n";
            handlePreSchedulingCheck();
        }
        else if (!strcmp(msg->getName(), "PostBatchSchedule"))   // do necessary check and update after each batch scheduling in distributed scheduling
        {
            EV << NOW << " Scheduler::handleMessage - post batch scheduling processing\n";
            if (enableDistScheme_)
                postBatchScheduling();
        }
        else if (!strcmp(msg->getName(), "DistInstGenTimer"))   // generate schedule instances in distributed scheduling
        {
            EV << NOW << " Scheduler::handleMessage - schedule instances generation complete!" << endl;
            batchSchedulingOngoing_ = false;
            distStartTime_ = omnetpp::simTime(); // record the start time of distributed scheduling
            if (pvCounter_[targetPV_] == pv2Tokens_[distStage_][targetCategory_][targetPV_].size())
                performBatchScheduling();
            else
                EV << NOW << " Scheduler::handleDistributedScheduling - waiting for tokens for preference value " << targetPV_ << ", received " 
                    << pv2Tokens_[distStage_][targetCategory_][targetPV_].size() << "/" << pvCounter_[targetPV_] << endl;
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
    else if (!strcmp(msg->getName(), "DistToken"))
    {
        if (enableDistScheme_)
            recordDistToken(msg);
        
        delete msg;
        msg = nullptr;
    }
    else if (!strcmp(msg->getName(), "DistPV"))
    {
        if (enableDistScheme_)
        {
            if (distributedSchemeStarted_)
                throw cRuntimeError("Scheduler::handleMessage - received DistPV after distributed scheduling starts, which should not happen");

            Packet* pkt = check_and_cast<Packet*>(msg);
            auto distPV = pkt->peekAtFront<DistPV>();
            AppId appId = distPV->getAppId();
            distUnscheduledApps_.insert(appId);

            int pv = distPV->getPreferenceValue();
            pvCounter_[pv]++;
            if (pv > pvMax_)
                pvMax_ = pv;

            if (pvMin_ == 0)    // the pv starts from 1
                pvMin_ = pv;
            else if (pv < pvMin_)
                pvMin_ = pv;

            EV << "Scheduler::handleMessage - received distributed packet DistPV from app " << appId 
                << ", preference value " << pv << endl;
        }
        
        delete msg;
        msg = nullptr;
    }

}


void Scheduler::handleDistributedScheduling()
{
    EV << NOW << " Scheduler::handleDistributedScheduling - start distributed scheduling, scheme: " << schemeName_ << endl;

    // remove the outdated information (RSU status, UE-RSU connection, etc.)
    removeOutdatedInfo();
    pendingScheduleApps_ = distUnscheduledApps_;  // update the unscheduled apps for scheduling
    emit(vecPendingAppCountSignal_, int(pendingScheduleApps_.size()));
    db_->addPendingScheduleApps(pendingScheduleApps_);
    EV << NOW << " Scheduler::handleDistributedScheduling - pending scheduled app count: " << pendingScheduleApps_.size() << endl;

    if (pendingScheduleApps_.size() == 0)
    {
        EV << NOW << " Scheduler::handleDistributedScheduling - no request to schedule" << endl;

        emit(vecInsGenerateTimeSignal_, insGenerateTime_.dbl());
        emit(vecSchemeTimeSignal_, schemeExecTime_.dbl());
        emit(vecSchedulingTimeSignal_, schedulingTime_.dbl());

        vector<srvInstance> selectedIns;
        collectSchedulingResults(selectedIns);

        return;
    }

    // print pvCounter for logging
    EV << NOW << " Scheduler::handleDistributedScheduling - pvCounter: ";
    for (auto& pvCount : pvCounter_)
    {
        EV << "{PV " << pvCount.first << ": " << pvCount.second << "}, ";
    }
    EV << endl;

    distStage_ = "CandiSel";  // default to candidate selection stage
    targetPV_ = pvMin_;
    targetCategory_ = "LI"; // initial target category for candidate selection
    distributedSchemeStarted_ = true;
    
    // record the time for generating the schedule instances
    auto start = chrono::steady_clock::now();
    scheme_->generateScheduleInstances();
    auto end = chrono::steady_clock::now();
    insGenerateTime_ = SimTime(chrono::duration_cast<chrono::microseconds>(end - start).count(), SIMTIME_US);
    emit(vecInsGenerateTimeSignal_, insGenerateTime_.dbl());

    EV << NOW << " Scheduler::handleDistributedScheduling - schedule instances generation time: " << insGenerateTime_ << endl;
    batchSchedulingOngoing_ = true; // ensure the batch scheduling will not start until the distInstGenTimer_ is completed
    scheduleAt(simTime() + insGenerateTime_, distInstGenTimer_);
}


void Scheduler::recordDistToken(cMessage *msg)
{
    Packet* pkt = check_and_cast<Packet*>(msg);
    auto distToken = pkt->peekAtFront<DistToken>();
    auto tokenCopy = makeShared<DistToken>(*distToken);

    int pv = distToken->getPreferenceValue();
    string category = distToken->getTargetCategory();
    string stage = distToken->getStage();
    pv2Tokens_[stage][category][pv].push_back(tokenCopy);

    EV << "Scheduler::recordDistToken - received distributed packet DistToken from app " << distToken->getAppId() 
        << ", stage " << stage << ", category " << distToken->getTargetCategory() << ", preference value " << pv << endl;
    EV << "\t current received token count for stage " << stage << ", category " << category << ", preference value " << pv << ": " 
        << pv2Tokens_[stage][category][pv].size() << "/" << pvCounter_[pv] << endl;

    // only schedule tokens after the scheduling starts and all the tokens for the target preference value are received
    if (distributedSchemeStarted_ && !batchSchedulingOngoing_ && (pvCounter_[targetPV_] == pv2Tokens_[distStage_][targetCategory_][targetPV_].size()))
    {
        EV << "\t received all tokens for preference value " << targetPV_ 
            << " and no batch scheduling is ongoing, proceeding to batch scheduling" << endl;
        performBatchScheduling();
    }
}


void Scheduler::performBatchScheduling()
{
    batchSchedulingOngoing_ = true;
    
    // collect the candidate apps for the current batch
    // the mechanism to update targetPV_ ensures there will be at least one token for the target preference value
    vector<Ptr<DistToken>>& tokens = pv2Tokens_[distStage_][targetCategory_][targetPV_];
    simtime_t bacthExecTime = 0;

    EV << NOW << " Scheduler::performBatchScheduling - stage: " << distStage_ << ", preference value: " 
        << targetPV_ << ", category: " << targetCategory_ << ", number of tokens: " << tokens.size() << endl;

    // perform batch scheduling
    if (distStage_ == "CandiSel")
    {
        map<AppId, double> appUtilityMap = map<AppId, double>();
        for (auto& token : tokens)
        {
            AppId appId = token->getAppId();
            double utility = token->getUtilReduction();
            appUtilityMap[appId] = utility;
        }

        // start batch scheduling and record the execution time for candidate selection
        auto start = chrono::steady_clock::now();
        map<AppId, double> updatedUtilityMap = scheme_->candidateSelection(appUtilityMap, targetCategory_);
        auto end = chrono::steady_clock::now();
        bacthExecTime = SimTime(chrono::duration_cast<chrono::microseconds>(end - start).count(), SIMTIME_US);
        
        // update the utility reduction inside the tokens
        for (auto& token : tokens)
        {
            AppId appId = token->getAppId();
            double updatedUtility = updatedUtilityMap[appId];
            token->setUtilReduction(updatedUtility);
        }
    }
    else if (distStage_ == "SolSel")
    {
        map<AppId, bool> appSelectedMap = map<AppId, bool>();
        for (auto& token : tokens)
        {
            AppId appId = token->getAppId();
            bool selected = token->isScheduled();
            appSelectedMap[appId] = selected;
        }

        // start batch scheduling and record the execution time for solution selection
        auto start = chrono::steady_clock::now();
        map<AppId, bool> updatedSelectedMap = scheme_->solutionSelection(appSelectedMap, targetCategory_);
        auto end = chrono::steady_clock::now();
        bacthExecTime = SimTime(chrono::duration_cast<chrono::microseconds>(end - start).count(), SIMTIME_US);

        // update the selected result inside the tokens
        for (auto& token : tokens)
        {
            AppId appId = token->getAppId();
            bool updatedSelected = updatedSelectedMap[appId];
            token->setIsScheduled(updatedSelected);
        }
    }
    EV << NOW << " Scheduler::performBatchScheduling - batch execution time: " << bacthExecTime << endl;

    distBatchTimes_.push_back(bacthExecTime.dbl());
    scheduleAfter(bacthExecTime, postBatchSchedule_);
}


void Scheduler::postBatchScheduling()
{   
    auto& tokens = pv2Tokens_[distStage_][targetCategory_][targetPV_];
    EV << NOW << " Scheduler::postBatchScheduling - post batch scheduling" << ", stage: " << distStage_ << ", preference value: " 
        << targetPV_ << ", category: " << targetCategory_ << ", number of tokens: " << tokens.size() << endl;

    // send out the tokens to users
    while (!tokens.empty())
    {
        auto token = tokens.back();
        tokens.pop_back();
        // get the ue address and the app id from the token
        AppId appId = token->getAppId();
        // check if the appId is valid and the app information is available
        if (appInfo_.find(appId) == appInfo_.end())
            throw cRuntimeError("Scheduler::postBatchScheduling - appId %d in the token is not found in the appInfo", appId);
        
        Ipv4Address ueAddr = Ipv4Address(appInfo_[appId].ueIpv4Address);
        int appPort = MacCidToLcid(appId);

        Packet* pkt = new Packet("DistToken");
        pkt->insertAtFront(token);

        int nicInterfaceId = nodeInfo_->getNicInterfaceId();
        EV << "\t Sending token for app " << token->getAppId() << " to the 5G NIC with interface id " << nicInterfaceId << endl;
        // find the NIC interface id of the gNodeB
        pkt->addTagIfAbsent<InterfaceReq>()->setInterfaceId(nicInterfaceId);
        socket_.sendTo(pkt, ueAddr, appPort);
    }

    // check the termination condition for distributed scheduling
    if (distStage_ == "SolSel" && targetPV_ == pvMin_ && targetCategory_ == "LI")
    {
        schemeExecTime_ = simTime() - distStartTime_;            
        double totalDistBatchTime = accumulate(distBatchTimes_.begin(), distBatchTimes_.end(), 0.0);

        // record the real execution time of the scheduling scheme
        vector<srvInstance> selectedIns;
        schedulingTime_ = insGenerateTime_ + schemeExecTime_;
        
        if (schedulingTime_ < schedulingInterval_ - appStopInterval_)
            selectedIns = scheme_->getFinalSchedule();  // only handle the scheduling results when runtime is acceptable

        EV << "Scheduler::postBatchScheduling - distributed scheduling finished! " << "instance generation time: " 
            << insGenerateTime_ << ", scheme execution time: " << schemeExecTime_ << ", selected " << selectedIns.size()
            << "/" << pendingScheduleApps_.size() << " apps" << endl;
        
        collectSchedulingResults(selectedIns);
        scheduleAt(simTime(), schedComplete_);

        emit(vecSchemeTimeSignal_, schemeExecTime_.dbl());
        emit(vecDistSchemeExecTimeSignal_, totalDistBatchTime);
        emit(vecSchedulingTimeSignal_, schedulingTime_.dbl());

        pv2Tokens_.clear();
        pvCounter_.clear();
        distUnscheduledApps_.clear();
        pendingScheduleApps_.clear();
        distBatchTimes_.clear();
        pvMax_ = 0;
        pvMin_ = 0;
        targetPV_ = 1;
        targetCategory_ = "LI";
        distributedSchemeStarted_ = false;
        batchSchedulingOngoing_ = false;

        return;
    }

    // update the status for distributed scheduling
    if (distStage_ == "CandiSel")
    {
        if (targetPV_ < pvMax_)
        {
            // update the target preference value for the next batch scheduling
            // if there is no token received for the next preference value, 
            // skip to the next preference value until find the one with received token
            while (targetPV_ < pvMax_)
            {
                targetPV_ += 1;  
                if (pvCounter_[targetPV_] == 0)  
                    continue;
                else
                    break;
            }
        }
        else if (targetPV_ == pvMax_ && targetCategory_ == "LI")
        {
            targetPV_ = pvMin_;
            targetCategory_ = "HI";
        }
        else if (targetPV_ == pvMax_ && targetCategory_ == "HI")
            distStage_ = "SolSel";  // after finishing the candidate selection for all preference values, start the solution selection
    }
    else if (distStage_ == "SolSel")
    {
        if (targetPV_ > pvMin_)
        {
            while (targetPV_ > pvMin_)
            {
                targetPV_ -= 1;  
                if (pvCounter_[targetPV_] == 0)  
                    continue;
                else
                    break;
            }
        }
        else if (targetPV_ == pvMin_ && targetCategory_ == "HI")
        {
            targetPV_ = pvMax_;
            targetCategory_ = "LI";
        }
    }

    EV << "\t updated distributed scheduling status, next stage: " << distStage_ << ", next preference value: " 
            << targetPV_ << ", next category: " << targetCategory_ << endl;

    batchSchedulingOngoing_ = false;
    // check if the next batch scheduling is ready to start (the tokens already received)
    if (distributedSchemeStarted_ && !batchSchedulingOngoing_ && (pvCounter_[targetPV_] == pv2Tokens_[distStage_][targetCategory_][targetPV_].size()))
    {
        EV << NOW << " Scheduler::postBatchScheduling - the next batch scheduling is ready to start, proceeding to batch scheduling" << endl;
        performBatchScheduling();
    }
}


void Scheduler::handlePreSchedulingCheck()
{    
    if (rescheduleAll_)
    {
        // stop the service in initialization status
        for (AppId appId : appsWaitInitFb_)
            stopService(appId);

        // stop the running service
        for (AppId appId : allocatedApps_)
            stopService(appId);
    }
    else
    {
        // check if the stop time is reached for the allocated applications
        for (AppId appId : allocatedApps_)
        {
            if (simTime() >= appInfo_[appId].stopTime - appInfo_[appId].period)
            {
                EV << NOW << " Scheduler::handlePreSchedulingCheck - stop the expired application " << appId << endl;
                stopService(appId);
            }
        }
    }
}


void Scheduler::determinePendingScheduleApps()
{
    // determine all vehicles that have access to any of the RSU
    pendingScheduleApps_.clear();
    for (auto& rsuVehPair : vehAccessRsu_)
    {
        MacNodeId vehId = rsuVehPair.first;
        if (vehAccessRsu_[vehId].size() > 0)
            pendingScheduleApps_.insert(veh2AppIds_[vehId].begin(), veh2AppIds_[vehId].end());
    }
}


void Scheduler::handleCentralizedScheduling()
{
    EV << NOW << " Scheduler::handleCentralizedScheduling - start centralized scheduling, scheme: " << schemeName_ << endl;

    /***
     * if a service stop command is sent, need to wait for the service stop feedback to update the RSU status
     * only perform the next scheduling when all the service stop feedback is received
     * if the feedback is not received, reset the wait list and resend the stop command
     */
    if (appsWaitStopFb_.size() > 0)
        appsWaitStopFb_.clear();    // make sure to resend the stop command when next scheduling starts

    // remove the outdated information (RSU status, UE-RSU connection, etc.)
    removeOutdatedInfo();
    determinePendingScheduleApps();  // determine the pending scheduled apps based on the updated information
    emit(vecPendingAppCountSignal_, int(pendingScheduleApps_.size()));
    db_->addPendingScheduleApps(pendingScheduleApps_);
    EV << NOW << " Scheduler::handleCentralizedScheduling - start scheduling, unscheduled app count: " << pendingScheduleApps_.size() << endl;

    // generate the schedule instances and execute the scheduling scheme
    vector<srvInstance> selectedIns;
    if (pendingScheduleApps_.size() > 0)
    {
        // record the time for generating the schedule instances
        auto start = chrono::steady_clock::now();
        scheme_->generateScheduleInstances();
        auto end = chrono::steady_clock::now();
        insGenerateTime_ = SimTime(chrono::duration_cast<chrono::microseconds>(end - start).count(), SIMTIME_US);
        
        // record the time for executing the scheduling scheme
        start = chrono::steady_clock::now();
        selectedIns = scheme_->scheduleRequests();
        end = chrono::steady_clock::now();
        schemeExecTime_ = SimTime(chrono::duration_cast<chrono::microseconds>(end - start).count(), SIMTIME_US);

        EV << "Scheduler::handleCentralizedScheduling - instance generation time: " << insGenerateTime_ 
            << ", scheme execution time: " << schemeExecTime_ << endl;
        // record the real execution time of the scheduling scheme
        schedulingTime_ = insGenerateTime_ + schemeExecTime_;

        // check if the scheduling time is acceptable, if not, skip the scheduling results and clear the schedule
        if (schedulingTime_ < schedulingInterval_ - appStopInterval_)
            scheduleAt(simTime()+schedulingTime_, schedComplete_);
        else
            selectedIns.clear();  // clear the schedule if the execution time is too long
    }
    else
    {
        EV << NOW << " Scheduler::handleCentralizedScheduling - no request to schedule" << endl;
    }

    emit(vecInsGenerateTimeSignal_, insGenerateTime_.dbl());
    emit(vecSchemeTimeSignal_, schemeExecTime_.dbl());
    emit(vecSchedulingTimeSignal_, schedulingTime_.dbl());

    collectSchedulingResults(selectedIns);
}


void Scheduler::collectSchedulingResults(vector<srvInstance> &selectedIns)
{
    EV << NOW << " Scheduler::collectSchedulingResults - scheduling completed, collecting the scheduling results\n";
    
    double totalUtility = 0;
    double totalOffloadCount = 0;
    vecSchedule_.clear();
    map<AppId, double> appUtilityMap = map<AppId, double>();

    for (srvInstance ins : selectedIns)
    {
        AppId appId = get<0>(ins);
        double exeTime = scheme_->getAppExeDelay(appId);
        if (exeTime <= 0)
        {
            throw cRuntimeError("%f Scheduler::collectSchedulingResults - application %d has %f execution time, please check the scheduling scheme", simTime().dbl(), appId, exeTime);
        }

        MacNodeId offloadGnbId = get<1>(ins);
        MacNodeId processGnbId = get<2>(ins);
        
        ServiceInstance srv;
        srv.appId = appId;
        srv.offloadGnbId = offloadGnbId;
        srv.processGnbId = processGnbId;
        srv.bands = get<3>(ins);
        srv.cmpUnits = get<4>(ins);
        srv.exeTime = exeTime;
        srv.utility = scheme_->getAppUtility(appId);
        srv.serviceType = scheme_->getAppAssignedService(appId);

        if (srv.utility <= 0)
        {
            // add time as well
            throw cRuntimeError("%f Scheduler::collectSchedulingResults - application %d has 0 utility, please check the scheduling scheme", simTime().dbl(), appId);
        }

        double appMaxoffloadTime = scheme_->getMaxOffloadTime(appId);
        if (appMaxoffloadTime <= 0)
        {
            throw cRuntimeError("%f Scheduler::collectSchedulingResults - application %d has %f max offload time, please check the scheduling scheme", simTime().dbl(), appId, appMaxoffloadTime);
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


void Scheduler::updateNextSchedulingTime()
{
    if (periodicScheduling_)
    {
        /***
         * When failure occurs in the network, the backhaul network topology may be partitioned into several parts,
         * and the NEXT_SCHEDULING_TIME may be updated for few time.
         * To avoid starvation, we add a margin of 1 second to avoid NEXT_SCHEDULING_TIME being updated multiple times
         * in the same scheduling interval.
         * TODO: this margin can be further optimized.
         */
        if (NOW + 1 > NEXT_SCHEDULING_TIME)
        {
            NEXT_SCHEDULING_TIME = NEXT_SCHEDULING_TIME + schedulingInterval_;
            EV << "Scheduler::updateNextSchedulingTime - the scheduling time is updated to " << NEXT_SCHEDULING_TIME << endl;
        }
        else
        {
            EV << "Scheduler::updateNextSchedulingTime - the scheduling time has been updated by other scheduler, no need to update" << endl;
        }

        scheduleAt(NEXT_SCHEDULING_TIME, schedStarter_);
        scheduleAt(NEXT_SCHEDULING_TIME - appStopInterval_, preSchedCheck_);
        EV << NOW << " Scheduler::updateNextSchedulingTime - next scheduling time: " << NEXT_SCHEDULING_TIME << endl;
    }
}


void Scheduler::initializeSchedulingScheme()
{
    if (!enableDistScheme_)
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
        else if (!enableBackhaul_ && optimizeObjective_ == "accuracy")
        {   // benchmark shemes for distributed scheduling scheme comparison
            if (schemeName_ == "Greedy")
                scheme_ = new AccuracyGreedy(this);
            else if (schemeName_ == "GameTheory")
                scheme_ = new AccuracyGameTheory(this);
            else if (schemeName_ == "FastIS")
                scheme_ = new AccuracyFastIS(this);
            else if (schemeName_ == "SARound")
                scheme_ = new AccuracySARound(this);
            else if (schemeName_ == "Iterative")
                scheme_ = new AccuracyIterative(this);
            else if (schemeName_ == "IDAssign")
                scheme_ = new AccuracyIDAssign(this);
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
    else
    {
        if (optimizeObjective_ == "accuracy")
        {
            if (schemeName_ == "DistIS")
                scheme_ = new AccuracyDistIS(this);
            else if (schemeName_ == "Greedy")
                scheme_ = new AccuracyGreedy(this);
            else if (schemeName_ == "GameTheory")
                scheme_ = new AccuracyGameTheory(this);
            else if (schemeName_ == "FastIS")
                scheme_ = new AccuracyFastIS(this);
            else if (schemeName_ == "SARound")
                scheme_ = new AccuracySARound(this);
            else if (schemeName_ == "Iterative")
                scheme_ = new AccuracyIterative(this);
            else if (schemeName_ == "IDAssign")
                scheme_ = new AccuracyIDAssign(this);
            else
                scheme_ = new SchemeBase(this);
        }
        else
        {
            scheme_ = new SchemeBase(this);
        }
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
        /***
         * When the backhaul network is partitioned due to failure, multiple global schedulers can exist.
         * the first global scheduler recovers after failure need to set a margin for NEXT_SCHEDULING_TIME.
         */
        
        // get the time in milliseconds
        double timeNow = int(simTime().dbl() * 1000) / 1000.0;
        if (NEXT_SCHEDULING_TIME > 100000)  // the simulation just starts, initialize the NEXT_SCHEDULING_TIME
        {
            EV << "Scheduler::globalSchedulerInit - the system is initialized, set the first scheduling time" << std::endl;
            NEXT_SCHEDULING_TIME = appStopInterval_ + appFeedbackInterval_ + timeNow;
        }
        else if (simTime() > NEXT_SCHEDULING_TIME)  // the first global scheduler reset after failure.
        {
            // add a margin of 0.1s to wait for other global schedulers to be initialized
            // in case of network being partitioned
            // faultRecoveryMargin_ needs to be smaller than connOutdateInterval_
            EV << "Scheduler::globalSchedulerInit - the scheduling time is first updated after recovery" << std::endl;
            NEXT_SCHEDULING_TIME = appStopInterval_ + appFeedbackInterval_ + timeNow + faultRecoveryMargin_;
        }
        else
        {
            EV << "Scheduler::globalSchedulerInit - the scheduling time has been updated by other scheduler, no need to update" << std::endl;
        }

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
    veh2AppIds_[vehId].insert(appId);
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
            EV << NOW << " Scheduler::removeOutdatedInfo - vehicle[nodeId=" << vehId << "] left the simulation, remove the request" << endl;
            toRemove.insert(appId);
            continue;
        }
        double stopTime = appInfo_[appId].stopTime.dbl();
        double period = appInfo_[appId].period.dbl();
        if (period <= 0)
        {
            toRemove.insert(appId);
            EV << NOW << " Scheduler::removeOutdatedInfo - application " << appId << " has non-positive period, remove the request" << endl;
            continue;
        }
        double gap = max(period, schedulingInterval_);
        if (simTime() >= (stopTime - gap))  // if the stop time is reached
        {
            EV << NOW << " Scheduler::removeOutdatedInfo - application " << appId << " stop time reached, remove the request" << endl;
            toRemove.insert(appId);
        }
    }
    for (AppId appId : toRemove)
    {
        unscheduledApps_.erase(appId);
        MacNodeId vehId = appInfo_[appId].vehId;
        veh2AppIds_[vehId].erase(appId);
        appInfo_.erase(appId);
    }

    double expireInterval = appStopInterval_ + appFeedbackInterval_ + faultRecoveryMargin_;
    // ======== remove the outdated UE-RSU connection ============
    unordered_map<MacNodeId, set<MacNodeId>> vehAccessRsuCopy = vehAccessRsu_;
    for (auto const&vr : vehAccessRsuCopy)
    {
        MacNodeId vehId = vr.first;
        for (MacNodeId rsuId : vr.second)
        {
            auto link = make_tuple(vehId, rsuId);
            
            if ((simTime() - veh2RsuTime_[link] > expireInterval) || (veh2RsuRate_[link] <= 0))
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
        // MacNodeId rsuId = res.first;
        simtime_t lastBandUpdateTime = res.second.bandUpdateTime;
        simtime_t lastCmpUpdateTime = res.second.cmpUpdateTime;

        if ((simTime() - lastBandUpdateTime) > expireInterval)
        {
            EV << NOW << " Scheduler::removeOutdatedInfo - RSU[nodeId=" << res.first << "] bands information expired" << endl;
            res.second.bands = 0;  // the NIC may be turned off, so set the bands to 0
        }
            

        if ((simTime() - lastCmpUpdateTime) > expireInterval)
        {
            EV << NOW << " Scheduler::removeOutdatedInfo - RSU[nodeId=" << res.first << "] computing units information expired" << endl;
            res.second.cmpUnits = 0;  // the computing unit may be turned off, so set the cmpUnits to 0
        }
            
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
