//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecCommon.h/MecCommon.cc
//
//  Description:
//    Define common data struct used in MEC system
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-16
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef _MECRT_MECCOMMON_H_
#define _MECRT_MECCOMMON_H_

#include "common/LteCommon.h"


/**
 * Mec specific protocols
 */
class MecProtocol {
    public:
        static const inet::Protocol mecOspf;  // IP protocol on the uU interface
};


/***
 * Define the data struct used in MEC
 */

// =================================
// ======== for scheduler ==========
// =================================

typedef unsigned int AppId;

struct RequestMeta {
    int inputSize;          // input data size of the job
    int outputSize;     // output data size
    AppId appId;
    MacNodeId vehId;
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
    omnetpp::simtime_t bandUpdateTime;  // the time of updating
    omnetpp::simtime_t cmpUpdateTime;   // the time of updating
};

struct RsuAddr {
    inet::Ipv4Address rsuAddress;    // the Ipv4 address of the RSU
    int serverPort;     // the port of the server
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

// =================================
// ======== for RSU Server =========
// =================================

struct Service
{
    AppId appId;    // the application id
    int resourceType;  // whether using GPU or CPU
    int service;        // the service name
    MacNodeId processGnbId;  // the id of processing gNB
    MacNodeId offloadGnbId;  // the id of offloading gNB
    omnetpp::simtime_t exeTime;    // service execution time
    int cmpUnits;        // the allocated computing units for the service in RSU
    int bands;           // the allocated bands for the service in RSU
    omnetpp::simtime_t deadline;  // the deadline of the service
    int inputSize;  // the input data size of the job, in bytes
    int outputSize;  // the output data size of the job, in bytes
    bool initComplete;
    omnetpp::simtime_t maxOffloadTime;  // the maximum offloading time results in positive energy saving
};

#endif // _MECRT_MECCOMMON_H_
