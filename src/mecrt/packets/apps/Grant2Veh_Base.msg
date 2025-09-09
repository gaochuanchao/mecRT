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

class Grant2Veh_Base extends inet::FieldsChunk
{
	unsigned int appId;				// the application id of the vehicle, 4 bytes
	simtime_t maxOffloadTime;		// maximum offloading time in seconds, 8 bytes
	int bands;			// allocated number of bands, 4 bytes
	unsigned short processGnbId;	// the id of processing gNB, 2 bytes
	unsigned short offloadGnbId;	// the id of offloading gNB,
	int processGnbPort;		// rsu server port, 4 bytes
	int inputSize;		// application input size, 4 bytes
	int outputSize;		// application output size, 4 bytes
	int bytePerTTI;		// granted bytes can be transferred within each TTI
	
	chunkLength = inet::B(36);
}
