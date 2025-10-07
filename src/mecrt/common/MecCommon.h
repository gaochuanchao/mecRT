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
        // static const inet::Protocol mecOspf;  // IP protocol on the uU interface
        static inet::Protocol* mecOspf;  // IP protocol on the uU interface
};

void registerMecOspfProtocol();

static const inet::Ipv4Address MEC_UE_OFFLOAD_ADDR("192.168.0.0"); // the IPv4 address used by UE to offload data to MEC server
static const int MEC_NPC_PORT = 37; // the port number used by Node Packet Controller (NPC) module
static const int MEC_OSPF_PORT = 38; // the port number used by MecOspf module

/***
 * the next time when the scheduler should run
 * updated by the MecOspf module when a new global scheduler is elected
 * update by the global scheduler when it finishes a scheduling round
 * referenced by the UePhy module when deciding whether to broadcast feedback
 */
extern double NEXT_SCHEDULING_TIME; 

/***
 * Define the data struct used in MEC
 */

// =================================
// ======== for scheduler ==========
// =================================

typedef unsigned int AppId;

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



#endif // _MECRT_MECCOMMON_H_
