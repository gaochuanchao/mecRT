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
