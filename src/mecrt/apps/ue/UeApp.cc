//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    UeApp.cc / UeApp.h
//
//  Description:
//    This file implements the user equipment (UE) application in the MEC system.
//    The UE application is responsible for generating and sending tasks to the ES (RSU),
//    as well as receiving and processing responses from the ES (RSU).
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include <cmath>
#include <inet/common/TimeTag_m.h>
#include "mecrt/apps/ue/UeApp.h"

#define round(x) floor((x) + 0.5)

Define_Module(UeApp);
using namespace inet;
using namespace std;


UeApp::UeApp()
{
    initialized_ = false;
    selfSender_ = nullptr;
    initRequest_ = nullptr;
    serviceGranted_ = false;

    outputSize_ = 0;
    resourceType_ = GPU;
    localConsumedEnergy_ = 0;

    processGnbAddr_ = inet::Ipv4Address::UNSPECIFIED_ADDRESS; // the address of the gNB processing the job
    processGnbPort_ = -1; // the port of the gNB processing the job

    MAX_UDP_CHUNK = 65527;  // 65535 - 8
    MAX_IPv4_CHUNK = 1480;  // 1500 - 20

    enableInitDebug_ = false;
}

UeApp::~UeApp()
{
    if (enableInitDebug_)
        std::cout << "UeApp::~UeApp - destroying UE application\n";

    if (selfSender_)
    {
        cancelAndDelete(selfSender_);
        selfSender_ = nullptr;
    }
    if (initRequest_)
    {
        cancelAndDelete(initRequest_);
        initRequest_ = nullptr;
    }

    if (enableInitDebug_)
        std::cout << "UeApp::~UeApp - destroying UE application done!\n";
}

void UeApp::initialize(int stage)
{

    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {
        if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
        if (enableInitDebug_)
            std::cout << "UeApp::initialize - stage: INITSTAGE_LOCAL - begins" << std::endl;

        nframes_ = 0;
        iDframe_ = 0;
        localPort_ = par("localPort");
        // schedulerPort_ = par("schedulerPort");

        startOffset_ = intuniform(0, 50) * TTI; // 0~50ms
        serviceGranted_ = false;
        txBytes_ = 0;

        selfSender_ = new cMessage("selfSender");
        selfSender_->setSchedulingPriority(1);    // after other messages
        initRequest_ = new cMessage("initRequest");

        // register signals
        offloadPower_ = getParentModule()->getSubmodule("cellularNic")->getSubmodule("nrPhy")->par("offloadPower");

        localProcessSignal_ = registerSignal("localProcessCount");
        offloadSignal_ = registerSignal("offloadCount");
        savedEnergySignal_ = registerSignal("vehSavedEnergy");
        energyConsumedIfLocalSignal_ = registerSignal("vehEnergyConsumedIfLocal");
        dlScale_ = par("dlScale");

        if (enableInitDebug_)
            std::cout << "UeApp::initialize - stage: INITSTAGE_LOCAL - ends" << std::endl;
    }
    else if (stage == INITSTAGE_LAST){
        if (enableInitDebug_)
            std::cout << "UeApp::initialize - stage: INITSTAGE_LAST - begins" << std::endl;

        EV << "VEC Application initialize: stage " << stage << endl;

        nodeInfo_ = getModuleFromPar<NodeInfo>(par("nodeInfoModulePath"), this);
        // destAddress_ = MEC_UE_OFFLOAD_ADDR; // the address of the offloading RSU server

        MacNodeId srcId = nodeInfo_->getNodeId();
        appId_ = idToMacCid(srcId, localPort_);
        EV << "UeApp::initialize - macNodeId " << srcId << ", portId " << localPort_ << ", appId_ " << appId_ <<endl;

        binder_ = getBinder();
        mobility_ = check_and_cast<MecMobility*>(getParentModule()->getSubmodule("mobility"));
        moveStartTime_ = mobility_->getMoveStartTime();
        moveStoptime_ = mobility_->getMoveStopTime();

        EV << "\t start time " << moveStartTime_ << " stop time " << moveStoptime_ << " job release time " 
            << moveStartTime_+startOffset_ << endl;

        initTraffic();

        db_ = check_and_cast<Database*>(getSimulation()->getModuleByPath("database"));
        if (db_ == nullptr)
            throw cRuntimeError("UeApp::initTraffic - the database module is not found");
        imgIndex_ = intuniform(0, db_->getNumVehExeData() - 1);
        vector<double> & srvInfo = db_->getVehExeData(imgIndex_);

        int typeId = intuniform(0, SERVICE_COUNTER - 1);
        appType_ = static_cast<VecServiceType>(typeId);
        inputSize_ = srvInfo[0] * 1024;  // in bytes
        inputSize_ = inputSize_ + computeExtraBytes(inputSize_);
        localExecTime_ = srvInfo[typeId*2+1] / 1000.0;    // in seconds, as the profiling time is in milliseconds
        localExecPower_ = srvInfo[typeId*2+2];  // in mW
        period_ = db_->appDeadline.at(appType_) / dlScale_; // in seconds
        period_ = round(period_ * 1000.0) / 1000.0; // round to 3 decimal places
        localExecTime_ = min(localExecTime_, period_.dbl()); // the local execution time should not exceed the period
        localConsumedEnergy_ = localExecPower_ * localExecTime_;
        nframes_ = (moveStoptime_ - moveStartTime_ - startOffset_) / period_;

        EV << "UeApp::initialize - image index " << imgIndex_ << " input size " << inputSize_ << " output size " << outputSize_ 
            << " local exec time " << localExecTime_ << " local exec power " << localExecPower_ << " period " << period_ 
            << " application type " << appType_ << endl;

        WATCH(appType_);
        WATCH(inputSize_);
        WATCH(localExecTime_);
        WATCH(localExecPower_);
        WATCH(period_);
        WATCH(offloadPower_);
        WATCH(serviceGranted_);
        WATCH(processGnbAddr_);
        WATCH(processGnbPort_);

        if (enableInitDebug_)
            std::cout << "UeApp::initialize - stage: INITSTAGE_LAST - ends" << std::endl;
    }
}

void UeApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (NOW > moveStoptime_ - period_)
        {
            EV << "UeApp::handleMessage - stop traffic for app " << appId_ << "!" <<endl;
            return;
        }
        
        if (!strcmp(msg->getName(), "selfSender"))
        {
            // EV << "UeApp::handleMessage - now[" << simTime() << "] <= finish[" << finishTime_ <<"]" <<endl;
            sendJobPacket();
            scheduleAt(simTime() + period_, selfSender_);
        }
        else if(!strcmp(msg->getName(), "initRequest"))
        {
            EV << "UeApp::handleMessage - now[" << simTime() << "] <= finish[" << moveStoptime_ <<"]" <<endl;
            sendServiceRequest();
            scheduleAt(omnetpp::simTime() + startOffset_, selfSender_);
        }
    }
    else
    {
        if(!strcmp(msg->getName(), "VehGrant"))
        {
            Packet* pkt = check_and_cast<Packet*>(msg);
            auto grant = pkt->popAtFront<Grant2Veh>();
            bool newGrant = grant->getNewGrant();
            bool updateGrant = grant->getGrantUpdate();

            if (grant->getGrantStop())
            {
                // specify both the offload and process gNB IDs
                EV << "UeApp::handleMessage - service grant for app " << grant->getAppId() << " that is offloaded to RSU " 
                    << grant->getOffloadGnbId() << " and processed on RSU " 
                    << grant->getProcessGnbId() << " is stopped!" <<endl;
                
                serviceGranted_ = false;
                processGnbAddr_ = inet::Ipv4Address::UNSPECIFIED_ADDRESS; // reset the process gNB address
                processGnbPort_ = -1; // reset the process gNB port
            }
            else if (grant->getPause())
            {
                EV << "UeApp::handleMessage - service grant for app " << grant->getAppId() << " that is offloaded to RSU " 
                    << grant->getOffloadGnbId() << " and processed on RSU " 
                    << grant->getProcessGnbId() << " is paused!" <<endl;

                serviceGranted_ = false;
            }
            else if(newGrant || updateGrant)  // service is granted
            {
                EV << "UeApp::handleMessage - new service grant for app " << grant->getAppId() << endl;
                EV << "\t offloadGnbId: " << grant->getOffloadGnbId() << ", processGnbId: " << grant->getProcessGnbId() 
                   << ", processGnbPort: " << grant->getProcessGnbPort() << ", processGnbAddr: " << inet::Ipv4Address(grant->getProcessGnbAddr())
                   << ", inputSize: " << grant->getInputSize() << endl;

                serviceGranted_ = true;
                processGnbAddr_ = inet::Ipv4Address(grant->getProcessGnbAddr());
                processGnbPort_ = grant->getProcessGnbPort();
            }
        }

        delete msg;
    }
}

void UeApp::initTraffic()
{
    // std::string schedAddress = par("schedulerAddr").stringValue();
    // cModule* schedModule = findModuleByPath(par("schedulerAddr").stringValue());
    // if (schedModule == nullptr)
    //     throw cRuntimeError("UeApp::initTraffic - %s address not found", schedAddress.c_str());

    // schedulerAddr_ = inet::L3AddressResolver().resolve(par("schedulerAddr").stringValue());
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort_);

    /***
     * used by function Ipv4::encapsulate() to set the typeOfService parameter in Ipv4Header tag
     */
    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    EV << "UeApp::initialize - binding to port: local:" << localPort_ << endl;

    scheduleAt(moveStartTime_, initRequest_);
}

void UeApp::sendJobPacket()
{
    if (serviceGranted_)
    {
        EV << "UeApp::sendJobPacket - sending job packet!" <<endl;
        
        Packet* packet = new Packet("AppData");
        auto vecApp = makeShared<JobPacket>();
        vecApp->setNframes(nframes_);
        vecApp->setIDframe(iDframe_++);
        vecApp->setJobInitTimestamp(simTime());
        vecApp->setAbsDeadline(simTime() + period_);
        vecApp->setInputSize(inputSize_);
        vecApp->setOutputSize(outputSize_);
        vecApp->setChunkLength(B(inputSize_));
        vecApp->setAppId(appId_);
        vecApp->addTag<CreationTimeTag>()->setCreationTime(simTime());
        packet->insertAtBack(vecApp);

        if( simTime() > getSimulation()->getWarmupPeriod() )
        {
            txBytes_ += inputSize_;
        }
        socket.sendTo(packet, processGnbAddr_, processGnbPort_);

        emit(offloadSignal_, 1);
        emit(localProcessSignal_, 0);
        emit(savedEnergySignal_, localConsumedEnergy_);
    }
    else
    {
        EV << "UeApp::sendJobPacket - service for app " << appId_ << " is not granted, processed locally!" <<endl;
        emit(offloadSignal_, 0);
        emit(localProcessSignal_, 1);
        emit(savedEnergySignal_, 0);
    }

    emit(energyConsumedIfLocalSignal_, localConsumedEnergy_);
}

void UeApp::sendServiceRequest()
{
    EV << "UeApp::sendJobPacket - sending vehicle request, application type: " << appType_ <<endl;

    Packet* packet = new Packet("SrvReq");
    auto vecReq = makeShared<VecRequest>();
    vecReq->setInputSize(inputSize_);
    vecReq->setOutputSize(outputSize_);
    vecReq->setPeriod(period_);
    vecReq->setResourceType(resourceType_);
    vecReq->setService(appType_);
    vecReq->setAppId(appId_);
    vecReq->setStopTime(moveStoptime_);
    vecReq->setEnergy(localConsumedEnergy_);
    vecReq->setOffloadPower(offloadPower_);
    vecReq->setUeIpAddress(0); // will be filled by the NPC module
    // vecReq->addTag<CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(vecReq);

    socket.sendTo(packet, MEC_UE_OFFLOAD_ADDR, MEC_NPC_PORT);
}


/***
 * compute the extra bytes for the IP and UDP headers
 * To accelerate the simulation, we manually increased the upper limit of the packet size allowed by UDP and IP
 * such that the packet is not fragmented. However, the additional headers should be computed to reflect the real
 * data size that is transmited after going through the UDP and IPv4 modules.
 */
int UeApp::computeExtraBytes(int dataSize)
{
    int chunkSize = dataSize - MAX_UDP_CHUNK;
    int extraUdp = 0;
    while (chunkSize > 0)
    {
        chunkSize -= MAX_UDP_CHUNK;
        extraUdp += 8;  // every additional UDP header is 8 bytes
    }

    chunkSize = dataSize + extraUdp - MAX_IPv4_CHUNK;
    int extraIp = 0;
    while (chunkSize > 0)
    {
        chunkSize -= MAX_IPv4_CHUNK;
        extraIp += 20;  // every additional IPv4 header is 20 bytes
    }
    
    return extraUdp + extraIp;
}


void UeApp::finish()
{

}
