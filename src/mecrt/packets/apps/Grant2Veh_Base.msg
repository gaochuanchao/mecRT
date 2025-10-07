//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    Grant2Veh_Base.msg
//
//  Description:
//    This file implements message Grant2Veh_Base, which is sent from the 5G NIC module of an ES (RSU)
//	  to users for task offloading related information:
//		- start/suspend/stop offloading
//		- bandwidth allocation
//		- data rate
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

class Grant2Veh_Base extends inet::FieldsChunk
{
	unsigned int appId;				// the application id of the vehicle, 4 bytes
	uint32 ueAddr;			// the IP address of the vehicle, 4 bytes
	simtime_t maxOffloadTime;		// maximum offloading time in seconds, 8 bytes
	int bands;			// allocated number of bands, 4 bytes
	unsigned short processGnbId;	// the id of processing gNB, 2 bytes
	unsigned short offloadGnbId;	// the id of offloading gNB,
	uint32 processGnbAddr;	// the ipv4 address of processing gNB, 4 bytes
	int processGnbPort;		// rsu server port, 4 bytes
	int inputSize;		// application input size, 4 bytes
	int outputSize;		// application output size, 4 bytes
	int bytePerTTI;		// granted bytes can be transferred within each TTI
	
	chunkLength = inet::B(40);
}
