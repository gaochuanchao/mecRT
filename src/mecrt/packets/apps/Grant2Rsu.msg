//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    Grant2Rsu.msg
//
//  Description:
//    This file implements message Grant2Rsu, which is sent from the scheduler to server module
//	  to either initiate or terminate the service for a vehicle application.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;


class Grant2Rsu extends inet::FieldsChunk
{
	unsigned int appId;				// the application id of the vehicle, 4 bytes
	uint32 ueAddr;			// the IP address of the vehicle, 4 bytes
	unsigned short resourceType;	// the resource type of the server	(e.g., "GPU"), 2 bytes
	unsigned short service;			// the service name, 2 bytes
	unsigned short processGnbId;	// the gNB id of processing gNB, 2 bytes
	unsigned short offloadGnbId;	// the gNB id of offloading gNB, 2 bytes
	uint32 offloadGnbAddr;		// the IP address of offloading gNB, 4 bytes
	int cmpUnits;			// allocated computing units, 4 bytes
	int bands;		    // allocated number of bands, 4 bytes
	simtime_t exeTime;	// the time needed for app processing in seconds
	simtime_t maxOffloadTime;	// the max offload time result in positive saved energy
	simtime_t deadline;				// application deadline, 8 bytes
	int outputSize;		// result size in bytes, 4 bytes
	int inputSize;		// input size in bytes, 4 bytes
	bool start;
	bool stop;
	
	chunkLength = inet::B(48);
}
