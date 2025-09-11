//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    RsuFeedback.msg
//
//  Description:
//    This file implements message RsuFeedback, which is sent from the ES (RSU) to scheduler
//	  Before each scheduling cycle starts, UEs broadcast the SRS to nearby RSUs, and RSUs collects
//    the channel status together with its bandwidth information and computing resource information,
//    and then send the RsuFeedback message to the scheduler.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//
import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

class RsuFeedback extends inet::FieldsChunk {
    // ===== specified in the gNB MAC stack ===========
	int gnbId;	// MacNodeId of gNB, 2 bytes
    int vehId;	// MacNodeId of UE, 2 bytes
    int serverPort;			// the port id of the server app within the gNB, 2 bytes
    double frequency; 		// carrier frequency, 4 bytes
    int bytePerBand;	// bytes that can be delived within 1 TTI for each band, 4 bytes
    int availBands;	// number of bands, 2 bytes
    int totalBands;	// total bands can be used for scheduling, 2 bytes
	
	// ===== specified in the server app of the gNB ===========
	int freeCmpUnits;	// the available computing units in the RSU server, 2 bytes
	int totalCmpUnits;	// total computing units in the RSU server, 2 bytes
	unsigned short	deviceType;			// the device type of the server (e.g., "RTX3090"), 2 bytes
	unsigned short resourceType;		// the resource type of the server	(e.g., "GPU"), 2 bytes

    simtime_t bandUpdateTime; // the time when the bands were updated, 8 bytes
    simtime_t cmpUnitUpdateTime; // the time when the computing units were updated, 8 bytes
	
	chunkLength = inet::B(44);
}
