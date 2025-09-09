//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;


class Grant2Rsu extends inet::FieldsChunk
{
	unsigned int appId;				// the application id of the vehicle, 4 bytes
	unsigned short resourceType;	// the resource type of the server	(e.g., "GPU"), 2 bytes
	unsigned short service;			// the service name, 2 bytes
	unsigned short processGnbId;	// the id of processing gNB, 2 bytes
	unsigned short offloadGnbId;	// the id of offloading gNB, 2 bytes
	int cmpUnits;			// allocated computing units, 4 bytes
	int bands;		    // allocated number of bands, 4 bytes
	simtime_t exeTime;	// the time needed for app processing in seconds
	simtime_t maxOffloadTime;	// the max offload time result in positive saved energy
	simtime_t deadline;				// application deadline, 8 bytes
	int outputSize;		// result size in bytes, 4 bytes
	int inputSize;		// input size in bytes, 4 bytes
	bool start;
	bool stop;
	
	chunkLength = inet::B(40);
}
