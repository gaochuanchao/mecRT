//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    OspfHello.msg
//
//  Description:
//    This file implements message OspfHello, which is sent between OSPF routers
//	  to establish and maintain neighbor relationships.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

class OspfHello extends inet::FieldsChunk
{
    uint32 senderIp;
    uint32 neighborIp;

    chunkLength = inet::B(8);
}
