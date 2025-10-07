//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    OspfLsa.msg
//
//  Description:
//    This file implements the message OspfLsa, which is sent between OSPF routers
//	  to exchange link-state information.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

class OspfLsa extends inet::FieldsChunk
{
    uint32 origin;  // the int format of the origin node's IP address
    int nodeId; // the origin node's ID
    uint32 seqNum;
    simtime_t installTime;
    uint32 neighbor[]; // the int format of the neighbor IP addresses
    double cost[];                // corresponding costs

    chunkLength = inet::B(28); // origin + nodeId + seqNum + installTime + neighbor + cost
}
