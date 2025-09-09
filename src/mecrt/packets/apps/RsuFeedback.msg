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
