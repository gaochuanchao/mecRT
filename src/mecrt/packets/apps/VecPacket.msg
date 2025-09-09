// 
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
// 
// 

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

//cplusplus {{
//#include "NR/common/VecCommon.h"
//}}


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
	simtime_t period;			// the deadline of single job or period of periodic task, 8 bytes
	unsigned short resourceType;	// whether using GPU or CPU, 2 bytes
	unsigned short service;			// the service name, 2 bytes
	unsigned int appId;	// the unique id of this application, 4 bytes
	simtime_t stopTime;	// the time that this application stops
	double energy;	// the energy consumed by processing an application locally
	double offloadPower;	// the offloading power of the Gnb
	chunkLength = inet::B(40);
}



