//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    DistPV.msg
//
//  Description:
//    This file implements message DistPV, used in distributed scheduling to inform the preference value 
//      that each application has assigned to each accessible RSU.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;


class DistPV extends inet::FieldsChunk
{
    unsigned int appId; // the application id of the vehicle, 4 bytes
    unsigned short preferenceValue;	// the preference value of the application for scheduling, 2 bytes
    
    chunkLength = inet::B(6);
}
