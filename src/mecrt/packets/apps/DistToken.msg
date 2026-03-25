//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    DistToken.msg
//
//  Description:
//    This file implements message DistToken, used in distributed scheduling to exchange information
//    among ESs for scheduling decisions.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;


class DistToken extends inet::FieldsChunk
{
    unsigned int appId; // the application id of the vehicle, 4 bytes
    double utilReduction;   // the accumulated utility reduction of the application, 8 bytes
    string targetCategory;	// the category of candidates to select, 4 bytes
    unsigned short preferenceValue;	// the preference value of the application for scheduling, 2 bytes
    bool isScheduled;		// whether the application has been scheduled, 1 byte
    
    chunkLength = inet::B(19);
}
