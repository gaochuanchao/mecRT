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
// notes regarding to different init stages
// INITSTAGE_LOCAL 0
// INITSTAGE_NETWORK_INTERFACE_CONFIGURATION 1
// INITSTAGE_PHYSICAL_ENVIRONMENT 4
// INITSTAGE_SINGLE_MOBILITY 6
// INITSTAGE_PHYSICAL_LAYER 8
// INITSTAGE_LINK_LAYER 9
// INITSTAGE_NETWORK_CONFIGURATION 12
// INITSTAGE_NETWORK_LAYER 17
// INITSTAGE_TRANSPORT_LAYER 19
// INITSTAGE_LAST 22
//

#ifndef _MECRT_UEAPP_H_
#define _MECRT_UEAPP_H_

#include <string.h>
#include <omnetpp.h>

#include <inet/common/INETDefs.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "mecrt/packets/apps/VecPacket_m.h"
#include "mecrt/mobility/MecMobility.h"
#include "mecrt/common/Database.h"
#include "mecrt/nic/mac/UeMac.h"
#include "mecrt/common/NodeInfo.h"

class UeApp : public omnetpp::cSimpleModule
{
  protected:
    bool enableInitDebug_;
    inet::UdpSocket socket;
    //has the sender been initialized?
    bool initialized_;

    omnetpp::cMessage *selfSender_;
    omnetpp::cMessage* initRequest_;

    //sender
    int nframes_;
    int iDframe_;
    int txBytes_;
    // ----------------------------

    int localPort_;

    int inputSize_;          // input data size of the job
    int outputSize_;     // output data size
    unsigned int appId_;
    omnetpp::simtime_t period_;          // in milliseconds, the deadline of single job or period of periodic task
    VecResourceType resourceType_;    // whether using GPU or CPU
    VecServiceType appType_;            // the service name
    double localExecTime_;          // the local execution time of the job
    double localExecPower_;          // the local execution power of the job
    double offloadPower_;          // the offloading power of the ue
    double dlScale_;          // the scale for the deadline of the app, default is 1.0. dl = dl / dlScale_
    
    double localConsumedEnergy_;    // the local consumed energy of the job
    double fullyLocalConsumedEnergy_;    // the local consumed energy of the job is all processed locally
    
    int imgIndex_;  // the random assigned index for the application image

    bool serviceGranted_;    // whether the service has been granted by the RSU server

    // int schedulerPort_;
    // inet::L3Address schedulerAddr_;

    Binder *binder_;
    Database *db_;
    NodeInfo *nodeInfo_;  // the node information of the vehicle

    MecMobility *mobility_;   // the mobility module of the vehicle
    simtime_t moveStartTime_;	// the start time of the provided file, start moving
	  simtime_t moveStoptime_;	// the last time of provided file, stop moving
    omnetpp::simtime_t startOffset_;  // the offset of the start time

    /***
     * a IPv4 segment cannot exceed 1500 bytes (including a IPv4 header 20 bytes).
     * a UDP segment cannot exceed 65535 bytes (including a UDP header 8 bytes).
     */
    int MAX_UDP_CHUNK;
    int MAX_IPv4_CHUNK;

    // ----------- Define Signals -----------------
    omnetpp::simsignal_t localProcessSignal_; // the local processing signal
    omnetpp::simsignal_t offloadSignal_;  // the offloading signal
    omnetpp::simsignal_t savedEnergySignal_;   // energy consumed by processing each job
    omnetpp::simsignal_t energyConsumedIfLocalSignal_; // energy consumed by processing each job fully locally

  public:
    ~UeApp();
    UeApp();

  protected:

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void finish() override;
    void handleMessage(omnetpp::cMessage *msg) override;

    virtual void initTraffic();
    virtual void sendJobPacket();
    virtual void sendServiceRequest();

    virtual int computeExtraBytes(int dataSize);
};

#endif

