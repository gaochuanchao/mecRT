//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    Server.cc / Server.h
//
//  Description:
//    This file implements the server in the edge server (RSU) in MEC.
//    The server handles the service initialization and termination, maintains resource status,
//    and communicates with the scheduler and the 5G NIC module.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//
#include "mecrt/apps/server/Server.h"

#include "mecrt/packets/apps/RsuFeedback_m.h"
#include "mecrt/packets/apps/ServiceStatus_m.h"
#include "mecrt/packets/apps/Grant2Veh.h"

#include "inet/common/TimeTag_m.h"
#include "corenetwork/gtp/GtpUserMsg_m.h"
#include <inet/linklayer/common/InterfaceTag_m.h>

Define_Module(Server);

using namespace inet;

Server::Server()
{
    srvInitComplete_ = nullptr;
    enableInitDebug_ = false;
    nodeInfo_ = nullptr;
}

Server::~Server()
{
    if (enableInitDebug_)
        std::cout << "Server::~Server - destroying Server module\n";

    if (srvInitComplete_)
    {
        cancelAndDelete(srvInitComplete_);
        srvInitComplete_ = nullptr;
    }

    if (enableInitDebug_)
        std::cout << "Server::~Server - destroying Server module done!\n";
}

void Server::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {
        if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
        if (enableInitDebug_)
            std::cout << "Server::initialize - stage: INITSTAGE_LOCAL - begins" << std::endl;

        //filterOut_ = gate("filterOut");
        localPort_ = par("localPort");
        cmpUnitTotal_ = par("cmpUnitTotal");
        cmpUnitFree_ = cmpUnitTotal_;
        int resType = par("resourceType");
        resourceType_ = static_cast<VecResourceType>(resType);
        int devType = intuniform(0, DEVICE_COUNTER - 1);
        deviceType_ = static_cast<VecDeviceType>(devType);

        int minTime = par("serviceInitMinTime");
        int maxTime = par("serviceInitMaxTime");
        initServiceStartingTime(minTime, maxTime);
        WATCH(cmpUnitFree_);
        WATCH(deviceType_);

        meetDlPktSignal_ = registerSignal("meetDlPkt");
        failedSrvDownSignal_ = registerSignal("failedSrvDownPkt");
        missDlPktSignal_ = registerSignal("missDlPkt");

        if (enableInitDebug_)
            std::cout << "Server::initialize - stage: INITSTAGE_LOCAL - ends" << std::endl;
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        if (enableInitDebug_)
            std::cout << "Server::initialize - stage: INITSTAGE_APPLICATION_LAYER - begins" << std::endl;

        EV << "Server::initialize - binding to port: local:" << localPort_ << endl;
        if (localPort_ != -1)
        {
            socket.setOutputGate(gate("socketOut"));
            socket.bind(localPort_);
        }

        nodeInfo_ = getModuleFromPar<NodeInfo>(par("nodeInfoModulePath"), this);
        nodeInfo_->setServerPort(localPort_);

        binder_ = getBinder();
        gnbId_ = getAncestorPar("macNodeId");

        serverAddr_ = inet::L3AddressResolver().resolve(getParentModule()->getFullName());
        EV << "Server::initialize - RSU server local address " << serverAddr_.toIpv4().str() << ", RSU MacNodeId " << gnbId_ << endl;

        srvInitComplete_ = new cMessage("ServiceInitComplete");
        srvInitComplete_->setSchedulingPriority(1);  // after other messages

        srvInInitVector_ = std::vector<AppId>();
        srvInitCompleteTime_ = std::map<AppId, simtime_t>();
        appsWaitMacInitFb_ = std::set<AppId>();

        WATCH_VECTOR(srvInInitVector_);
        WATCH_MAP(srvInitCompleteTime_);

        if (enableInitDebug_)
            std::cout << "Server::initialize - stage: INITSTAGE_APPLICATION_LAYER - ends" << std::endl;
    }
}

void Server::initServiceStartingTime(int minTime, int maxTime)
{
    for(int i = 0; i < SERVICE_COUNTER; ++i)
    {
        int time = uniform(minTime, maxTime);
        serviceInitTime_[i] = SimTime(time, SIMTIME_MS);
    }
}

void Server::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if(!strcmp(msg->getName(), "ServiceInitComplete")){
            AppId appId = srvInInitVector_[0];
            srvInInitVector_.erase(srvInInitVector_.begin());
            srvInitCompleteTime_.erase(appId);

            if (srvInInitVector_.size() > 0)
                scheduleAt(srvInitCompleteTime_[srvInInitVector_[0]], srvInitComplete_);

            if (appsWaitStop_.find(appId) != appsWaitStop_.end())   // a stop command received during initializing
            {
                grantedService_.erase(appId);
                appsWaitStop_.erase(appId);
            }
            else
            {
                EV << "Server::handleMessage - service initialization complete for application " << appId << endl;
                // send the grant message to the vehicle
                grantedService_[appId].initComplete = true;
                sendGrant2Vehicle(appId, false);
                appsWaitMacInitFb_.insert(appId);
            }
        }
    }
    else if(!strcmp(msg->getName(), "RsuFD")){
        updateRsuFeedback(msg);
    }
    else if(!strcmp(msg->getName(), "SrvGrant")){
        auto pkt = check_and_cast<Packet *>(msg);
        auto pktGrant = pkt->popAtFront<Grant2Rsu>();
        AppId appId = pktGrant->getAppId();

        if (pktGrant->getStart() && (grantedService_.find(appId) == grantedService_.end()))
            initializeService(pktGrant);
        else if (pktGrant->getStop() && (grantedService_.find(appId) != grantedService_.end()))
            stopService(appId);
        
        delete msg;
    }
    else if(!strcmp(msg->getName(), "SrvFD")){
        updateServiceStatus(msg);
    }
    else
    {
        auto pkt = check_and_cast<Packet *>(msg);
        auto appPkt = pkt->peekAtFront<JobPacket>();

        int frameId = appPkt->getIDframe();
        simtime_t reqDeadline = appPkt->getAbsDeadline();
        AppId appId = appPkt->getAppId();
        
        EV << "Server::handleMessage - received application packet " << pkt->getName() << endl;
        EV << "Server::handleMessage - app " << appId << " frame " << frameId << " required deadline " << reqDeadline << endl;

        // check if the app service is running on this RSU
        if (grantedService_.find(appId) == grantedService_.end())
        {
            EV << "Server::handleMessage - service for app " << appId << " is not running on this RSU" << endl;
            emit(failedSrvDownSignal_, 1);
        }
        else    // check if the deadline is met
        {
            if (simTime() + grantedService_[appId].exeTime > reqDeadline)  // the time when the job is processed is more than the deadline
            {
                EV << "Server::handleMessage - app " << appId << " frame " << frameId << " - app cannot be completed within its deadline, drop it" << endl;
                emit(missDlPktSignal_, 1);
            }
            else
            {
                EV << "Server::handleMessage - app " << appId << " frame " << frameId << " - application deadline is met" << endl;
                emit(meetDlPktSignal_, 1);
            }
        }

        delete msg;
    }
}

void Server::updateRsuFeedback(omnetpp::cMessage *msg)
{
    EV << "Server::updateRsuFeedback - update RSU status feedback and send it to the scheduler " << endl;

    auto pkt = check_and_cast<Packet *>(msg);
    pkt->trim();
    pkt->clearTags();
    auto rsuFd = pkt->removeAtFront<RsuFeedback>();

    rsuFd->setFreeCmpUnits(cmpUnitFree_);
    rsuFd->setDeviceType(deviceType_);
    rsuFd->setResourceType(resourceType_);
    rsuFd->setTotalCmpUnits(cmpUnitTotal_);
    rsuFd->setCmpUnitUpdateTime(simTime());
    pkt->insertAtFront(rsuFd);

    auto vehAddr = binder_->getIPv4Address(rsuFd->getVehId());

    // addHeaders(pkt, schedulerAddr_, schedulerPort_, vehAddr);
    // socket.sendTo(pkt, gwAddress_, tunnelPeerPort_);
    // pkt->addTagIfAbsent<InterfaceReq>()->setInterfaceId(pppIfInterfaceId_);
    socket.sendTo(pkt, nodeInfo_->getGlobalSchedulerAddr(), MEC_NPC_PORT);
}

void Server::updateServiceStatus(omnetpp::cMessage *msg)
{
    auto pkt = check_and_cast<Packet *>(msg);
    pkt->trim();
    pkt->clearTags();
    auto srvStatus = pkt->removeAtFront<ServiceStatus>();

    AppId appId = srvStatus->getAppId();
    EV << "Server::updateServiceStatus - service for app " << appId << " is " 
        << (srvStatus->getSuccess()? "alive" : "stopped") << ", inform the scheduler." << endl;
    
    if (srvStatus->getSuccess())    // service initialization success
    {
        grantedService_[appId].bands = srvStatus->getUsedBand();
        // if the service initialization success (not band adjustment), update computing units
        if (appsWaitMacInitFb_.find(appId) != appsWaitMacInitFb_.end())
        {
            cmpUnitFree_ -= grantedService_[appId].cmpUnits;
            appsWaitMacInitFb_.erase(appId);
        }
    }
    else    
    {
        // only restore computing units when the service is stopped (not initialization failed)
        if (appsWaitMacInitFb_.find(appId) == appsWaitMacInitFb_.end())
            cmpUnitFree_ += grantedService_[appId].cmpUnits;
        else
            appsWaitMacInitFb_.erase(appId);    // initialization failed
        
        grantedService_.erase(appId);
    }

    EV << "Server::updateServiceStatus - processing RSU " << gnbId_ << " has " 
       << cmpUnitFree_ << " free computing units, and offloading RSU " << srvStatus->getOffloadGnbId() << " has " 
       << srvStatus->getAvailBand() << " free bandwidth." << endl;

    srvStatus->setAvailCmpUnit(cmpUnitFree_);
    srvStatus->setProcessGnbCuUpdateTime(simTime());
    pkt->insertAtFront(srvStatus);

    // addHeaders(pkt, schedulerAddr_, schedulerPort_, serverAddr_.toIpv4());
    // socket.sendTo(pkt, gwAddress_, tunnelPeerPort_);
    // pkt->addTagIfAbsent<InterfaceReq>()->setInterfaceId(pppIfInterfaceId_);
    socket.sendTo(pkt, nodeInfo_->getGlobalSchedulerAddr(), MEC_NPC_PORT);
}

void Server::sendGrant2Vehicle(AppId appId, bool isStop)
{
    EV << "Server::sendGrant2Vehicle - send grant to the vehicle " << endl;
    Service& srv = grantedService_[appId];

    Packet* packet = new Packet("VehGrant");
    auto grant = makeShared<Grant2Veh>();
    grant->setAppId(appId);
    grant->setMaxOffloadTime(srv.maxOffloadTime);
    grant->setBands(srv.bands);
    grant->setProcessGnbId(srv.processGnbId);
    grant->setOffloadGnbId(srv.offloadGnbId);
    grant->setProcessGnbPort(localPort_);
    grant->setInputSize(srv.inputSize);
    grant->setOutputSize(srv.outputSize);
    grant->setGrantStop(isStop);
    packet->insertAtFront(grant);

    /***
     * if the processGnbId and offloadGnbId are the same, then the packet is sent to the NIC interface of the gNodeB
     * otherwise, the packet is forwarded to the NIC interface of the gNodeB
     */
    MacNodeId vehId = MacCidToNodeId(appId);
    auto vehAddr = binder_->getIPv4Address(vehId);
    int appPort = MacCidToLcid(appId);
    if (srv.processGnbId == srv.offloadGnbId)
    {
        int nicInterfaceId = nodeInfo_->getNicInterfaceId();
        EV << "Server::sendGrant2Vehicle - offloading gNodeB " << srv.offloadGnbId 
           << " is the same as processing gNodeB " << srv.processGnbId << ", send grant to NIC interface " << nicInterfaceId << endl;
        // find the NIC interface id of the gNodeB
        packet->addTagIfAbsent<InterfaceReq>()->setInterfaceId(nicInterfaceId);
        socket.sendTo(packet, vehAddr, appPort);
    }
    else
    {
        EV << "Server::sendGrant2Vehicle - offloading gNodeB " << srv.offloadGnbId 
           << " is different from processing gNodeB " << srv.processGnbId << ", forward grant to offloading gNodeB" << endl;

        // get the offloading gNodeB module by its MacNodeId
        cModule *offloadGnb = binder_->getModuleByMacNodeId(srv.offloadGnbId);
        if (offloadGnb == nullptr)
        {
            std::stringstream errorMsg;
            errorMsg << "Time: " << NOW << " Server::sendGrant2Vehicle - cannot find the offloading gNodeB module by its MacNodeId " << srv.offloadGnbId;
            throw cRuntimeError("%s", errorMsg.str().c_str());
        }
        // get the offloading gNodeB address and its server port
        L3Address offloadGnbAddr = L3AddressResolver().resolve(offloadGnb->getFullName()).toIpv4();
        EV << "Server::sendGrant2Vehicle - forwarding to the NPC module of the offloading gNodeB" << endl;
        // packet->addTagIfAbsent<InterfaceReq>()->setInterfaceId(pppIfInterfaceId_);
        socket.sendTo(packet, offloadGnbAddr, MEC_NPC_PORT);
    }
}


void Server::addHeaders(Packet* packet, const inet::L3Address& destAddr, int destPort, const inet::Ipv4Address& srcAddr)
{
    inet::Ptr<UdpHeader> udpHeader = makeShared<UdpHeader>();
    udpHeader->setDestinationPort(destPort);
    udpHeader->setTotalLengthField(udpHeader->getChunkLength() + packet->getTotalLength());
    udpHeader->setCrcMode(CRC_DECLARED_CORRECT);
    udpHeader->setCrc(0xC00D);
    packet->insertAtFront(udpHeader);

    inet::Ptr<Ipv4Header> ipv4Header = makeShared<Ipv4Header>();
    ipv4Header->setProtocolId(IP_PROT_UDP);
    ipv4Header->setDestAddress(destAddr.toIpv4());
    ipv4Header->setSrcAddress(srcAddr);   // vehicle address
    ipv4Header->addChunkLength(B(20));
    ipv4Header->setHeaderLength(B(20));
    ipv4Header->setTotalLengthField(ipv4Header->getChunkLength() + packet->getDataLength());
    packet->insertAtFront(ipv4Header);
}

void Server::initializeService(inet::Ptr<const Grant2Rsu> pkt)
{
    AppId appId = pkt->getAppId();
    MacNodeId processGnbId = pkt->getProcessGnbId();
    MacNodeId offloadGnbId = pkt->getOffloadGnbId();

    if (processGnbId != gnbId_)
    {
        EV << "Server::initializeService - processGnbId " << processGnbId 
           << " does not match RSU gnbId " << gnbId_ << ", service initialization failed for app " << appId << endl;
        Packet* packet = new Packet("SrvFD");
        auto srvStatus = makeShared<ServiceStatus>();
        srvStatus->setSuccess(false);
        srvStatus->setAppId(appId);
        srvStatus->setProcessGnbId(processGnbId);
        srvStatus->setOffloadGnbId(pkt->getOffloadGnbId());
        srvStatus->setGrantedBand(pkt->getBands());
        srvStatus->setAvailCmpUnit(cmpUnitFree_);
        srvStatus->addTag<CreationTimeTag>()->setCreationTime(simTime());
        packet->insertAtFront(srvStatus);
        // addHeaders(packet, schedulerAddr_, schedulerPort_, serverAddr_.toIpv4());
        // packet->addTagIfAbsent<InterfaceReq>()->setInterfaceId(pppIfInterfaceId_);
        socket.sendTo(packet, nodeInfo_->getGlobalSchedulerAddr(), MEC_NPC_PORT);

        grantedService_.erase(appId);
        return;
    }

    EV << "Server::initializeService - initialize the service for app " << appId << " in the RSU " << gnbId_ << endl;
    if (cmpUnitFree_ < pkt->getCmpUnits())
    {
        EV << "\t RSU does not have enough computing units to grant the service" << endl;
        Packet* packet = new Packet("SrvFD");
        auto srvStatus = makeShared<ServiceStatus>();
        srvStatus->setSuccess(false);
        srvStatus->setAppId(appId);
        srvStatus->setProcessGnbId(processGnbId);
        srvStatus->setOffloadGnbId(offloadGnbId);
        srvStatus->setGrantedBand(pkt->getBands());
        srvStatus->setAvailCmpUnit(cmpUnitFree_);
        srvStatus->addTag<CreationTimeTag>()->setCreationTime(simTime());
        packet->insertAtFront(srvStatus);
        // addHeaders(packet, schedulerAddr_, schedulerPort_, serverAddr_.toIpv4());
        // packet->addTagIfAbsent<InterfaceReq>()->setInterfaceId(pppIfInterfaceId_);
        socket.sendTo(packet, nodeInfo_->getGlobalSchedulerAddr(), MEC_NPC_PORT);

        grantedService_.erase(appId);
        return;
    }

    Service srv;
    srv.processGnbId = processGnbId;
    srv.offloadGnbId = offloadGnbId;
    srv.resourceType = static_cast<VecResourceType>(pkt->getResourceType());
    srv.service = static_cast<VecServiceType>(pkt->getService());
    srv.cmpUnits = pkt->getCmpUnits();
    srv.bands = pkt->getBands();
    srv.deadline = pkt->getDeadline();
    srv.outputSize = pkt->getOutputSize();
    srv.inputSize = pkt->getInputSize();
    srv.appId = appId;
    srv.exeTime = pkt->getExeTime();
    srv.initComplete = false;
    double time = pkt->getMaxOffloadTime().dbl();
    time = floor(time * 1000) / 1000;
    srv.maxOffloadTime = time;
    grantedService_[appId] = srv;

    EV << "\t Service Resource Demand: " << srv.cmpUnits <<  " computing units, " << srv.bands 
        << " bands, max offloading time " << srv.deadline - srv.exeTime << ", initialize time " << serviceInitTime_[srv.service] << endl;

    // insert the service into the waiting list
    srvInitCompleteTime_[appId] = simTime() + serviceInitTime_[srv.service];

    auto it = srvInInitVector_.begin();
    while (it != srvInInitVector_.end() && srvInitCompleteTime_[*it] <= srvInitCompleteTime_[appId]) {
        ++it;  // Find the first position where the current value is greater
    }
    srvInInitVector_.insert(it, appId);  // Insert key at the found position

    // if already scheduled, cancel the scheduled event. 
    if (srvInitComplete_->isScheduled())
        cancelEvent(srvInitComplete_);
    
    // Then, schedule the first event (initialize complete earliest) in the waiting list
    scheduleAt(srvInitCompleteTime_[srvInInitVector_[0]], srvInitComplete_);
}

void Server::stopService(AppId appId)
{
    Service& srv = grantedService_[appId];

    if (srv.initComplete)   // the service is running
    {
        EV << "Server::stopService - stop the running service for application " << appId << endl;
        sendGrant2Vehicle(appId, true); // send the stop grant to the vehicle
    }
    else    // the service is still in initializing status
    {
        EV << "Server::stopService - service stop command received during initializing for application " << appId << endl;
        // send the stop feedback to the scheduler
        Packet* packet = new Packet("SrvFD");
        auto srvStatus = makeShared<ServiceStatus>();
        srvStatus->setSuccess(false);
        srvStatus->setAppId(appId);
        srvStatus->setProcessGnbId(srv.processGnbId);
        srvStatus->setOffloadGnbId(srv.offloadGnbId);
        srvStatus->setGrantedBand(srv.bands);
        srvStatus->setAvailCmpUnit(cmpUnitFree_);
        srvStatus->addTag<CreationTimeTag>()->setCreationTime(simTime());
        packet->insertAtFront(srvStatus);
        // addHeaders(packet, schedulerAddr_, schedulerPort_, serverAddr_.toIpv4());
        // packet->addTagIfAbsent<InterfaceReq>()->setInterfaceId(pppIfInterfaceId_);
        socket.sendTo(packet, nodeInfo_->getGlobalSchedulerAddr(), MEC_NPC_PORT);

        appsWaitStop_.insert(appId);
    }
}

void Server::finish()
{

}
