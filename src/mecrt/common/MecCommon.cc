//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecCommon.h/MecCommon.cc
//
//  Description:
//    Define common data struct used in MEC system
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-16
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/common/MecCommon.h"
#include "inet/common/packet/dissector/ProtocolDissectorRegistry.h"
#include "inet/networklayer/ipv4/Ipv4ProtocolDissector.h"
#include "inet/common/ProtocolGroup.h"

using namespace inet;


inet::Protocol* MecProtocol::mecOspf = new inet::Protocol("mecOspf", "MEC OSPF");
Register_Protocol_Dissector(MecProtocol::mecOspf, Ipv4ProtocolDissector);

// Register mecOspf as a valid IPv4 payload protocol
// Ensure it runs before simulation starts
void registerMecOspfProtocol() {
    static bool registered = false;
    if (!registered) {
        ProtocolGroup::getIpProtocolGroup()->addProtocol(99, MecProtocol::mecOspf);
        registered = true;
    }
}

// const inet::Protocol MecProtocol::mecOspf("mecOspf", "MEC OSPF");
// Register_Protocol_Dissector(&MecProtocol::mecOspf, Ipv4ProtocolDissector);

// Register mecOspf as a valid IPv4 payload protocol
// Ensure it runs before simulation starts
// void registerMecOspfProtocol() {
//     static bool registered = false;
//     if (!registered) {
//         ProtocolGroup::getIpProtocolGroup()->addProtocol(99, &MecProtocol::mecOspf);
//         registered = true;
//     }
// }

