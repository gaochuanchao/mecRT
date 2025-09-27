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
import inet.networklayer.contract.ipv4.Ipv4Address;

class OspfLsa extends inet::FieldsChunk
{
    inet::Ipv4Address origin;
    uint32 seqNum;
    simtime_t installTime;
    inet::Ipv4Address neighbor[]; // neighbor IP addresses
    double cost[];                // corresponding costs

    chunkLength = inet::B(12); // base + origin + neighbor + cost
}
