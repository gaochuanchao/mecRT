//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecIpv4.cc / MecIpv4.h
//
//  Description:
//    Extends the INET Ipv4 module to support MEC backhaul network fault simulation.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef __MECRT_IPV4_H
#define __MECRT_IPV4_H

#include "inet/networklayer/ipv4/Ipv4.h"

using namespace inet;


class MecIpv4 : public Ipv4
{
    protected:
        /**
         * Performs unicast routing. Based on the routing decision, it sends the
         * datagram through the outgoing interface.
         */
        virtual void routeUnicastPacket(Packet *packet) override;
};

#endif // __MECRT_IPV4_H


