//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    VecPacket.msg
//
//  Description:
//    This file implements message JobPacket and VecRequest.
//	  The JobPacket is used to carry the offloading data from UE to gNB, and then from gNB to the server app.
//    The VecRequest is used to carry the offloading request from UE to the scheduler.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;


class JobPacket extends inet::FieldsChunk {
    int nframes;
    int IDframe;
    simtime_t absDeadline;
    simtime_t jobInitTimestamp;
    int inputSize;
    int outputSize;
    unsigned int appId;	// the unique id of this application, 4 bytes
}

class VecRequest extends inet::FieldsChunk {
	int inputSize;			// input data size of the job, 4 bytes
	int outputSize = 0;		// output data size, 4 bytes
	uint32_t ueIpAddress;	// the IP address of the UE, 4 bytes
	simtime_t period;			// the deadline of single job or period of periodic task, 8 bytes
	string resourceType;	// whether using GPU or CPU, 4 bytes
	string service;			// the service name, 4 bytes
	unsigned int appId;	// the unique id of this application, 4 bytes
	simtime_t stopTime;	// the time that this application stops, 8 bytes
	double accuracy;		// the accuracy requirement of the application, 4 bytes
	double energy;	// the energy consumed by processing an application locally
	double offloadPower;	// the offloading power of the Gnb
	chunkLength = inet::B(52); // total size: 52 bytes
}



