//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    ServiceStatus.msg
//
//  Description:
//    This file implements message ServiceStatus, which is sent from the ES (RSU) to scheduler.
//	  When a service initialize/terminate command is sent from the scheduler to the ES (RSU),
//    The server module first initialize/terminate the service, and inform the 5G NIC module to
//    update bandwidth allocation. After the 5G NIC module updates the bandwidth allocation,
//    the server module sends the ServiceStatus message to the scheduler to update its record.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;


class ServiceStatus extends inet::FieldsChunk
{
	unsigned int appId;				// the application id of the vehicle, 4 bytes
	unsigned short processGnbId;	// the id of processing gNB, 2 bytes
	unsigned short offloadGnbId;	// the id of offloading gNB, 2 bytes
	int processGnbPort;	// the port of processing gNB, 4 bytes
	bool success;					// if the initialization successd, 1 bytes
	int grantedBand;	// the bands granted by the scheduler, 4 bytes
	int usedBand;		// the actual band used by the AP, 4 bytes
	int availBand;		// the remaining available bands in the AP, 4 bytes
	int availCmpUnit;	// the remaining available bands in the RSU server, 4 bytes
	simtime_t offloadGnbRbUpdateTime; // the time when the offloading gNB updated its RBs, 8 bytes
	simtime_t processGnbCuUpdateTime; // the time when the processing gNB updated its computing units, 8 bytes

	chunkLength = inet::B(34);
}
