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
import inet.networklayer.contract.ipv4.Ipv4Address;
import inet.routing.ospf_common.OspfPacketBase;
import inet.transportlayer.common.CrcMode;

namespace inet::ospfv2;

cplusplus {{
using namespace ospf;
}}

class OspfHello extends ospf::OspfPacketBase
{
    type = HELLO_PACKET;
    version = 2;
}
