//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecUdp.cc / MecUdp.h
//
//  Description:
//    This file implements the UDP protocol with MEC support.
//    It extends the INET Udp module to handle large data packets.
//    The original Udp module can only handle packets up to 65527 Bytes, and fragmentation will occur for larger packets.
//    The fragmentation process significantly slows down the simulation.
//    To accelerate the simulation, we make the packet size threshold configurable, 
//    allowing larger packets to be sent without fragmentation.
//
//    Note that in real networks, packets larger than the MTU will be fragmented.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/udp/MecUdp.h"

#include "inet/common/socket/SocketTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/networklayer/common/MulticastTag_m.h"
#include "inet/networklayer/common/HopLimitTag_m.h"
#include "inet/networklayer/common/DscpTag_m.h"
#include "inet/networklayer/common/TosTag_m.h"
#include "inet/networklayer/contract/IL3AddressType.h"
#include "inet/transportlayer/common/L4Tools.h"

Define_Module(MecUdp);

void MecUdp::initialize(int stage)
{
    Udp::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        mtu_ = par("mtu");  // maximum transmission unit
        WATCH(mtu_);
    }
}

void MecUdp::handleUpperPacket(Packet *packet)
{
    if (packet->getKind() != UDP_C_DATA)
        throw cRuntimeError("Unknown packet command code (message kind) %d received from app", packet->getKind());

    const auto& interfaceReq = packet->findTag<InterfaceReq>();
    ASSERT(interfaceReq == nullptr || interfaceReq->getInterfaceId() != -1);
    
    L3Address srcAddr, destAddr;
    auto addressReq = packet->addTagIfAbsent<L3AddressReq>();
    destAddr = addressReq->getDestAddress();

    if (interfaceReq == nullptr && destAddr.isMulticast())
    {
        Udp::handleUpperPacket(packet);
        return;
    }

    srcAddr = addressReq->getSrcAddress();
    
    int srcPort = -1, destPort = -1;

    auto& socketReq = packet->removeTag<SocketReq>();
    int socketId = socketReq->getSocketId();

    SockDesc *sd = getOrCreateSocket(socketId);

    if (srcAddr.isUnspecified())
        addressReq->setSrcAddress(srcAddr = sd->localAddr);

    if (destAddr.isUnspecified())
        addressReq->setDestAddress(destAddr = sd->remoteAddr);

    if (auto& portsReq = packet->removeTagIfPresent<L4PortReq>()) {
        srcPort = portsReq->getSrcPort();
        destPort = portsReq->getDestPort();
    }

    if (srcPort == -1)
        srcPort = sd->localPort;

    if (destPort == -1)
        destPort = sd->remotePort;

    if (addressReq->getDestAddress().isUnspecified())
        throw cRuntimeError("send: unspecified destination address");

    if (destPort <= 0 || destPort > 65535)
        throw cRuntimeError("send: invalid remote port number %d", destPort);

    if (packet->findTag<MulticastReq>() == nullptr)
        packet->addTag<MulticastReq>()->setMulticastLoop(sd->multicastLoop);

    if (sd->ttl != -1 && packet->findTag<HopLimitReq>() == nullptr)
        packet->addTag<HopLimitReq>()->setHopLimit(sd->ttl);

    if (sd->dscp != -1 && packet->findTag<DscpReq>() == nullptr)
        packet->addTag<DscpReq>()->setDifferentiatedServicesCodePoint(sd->dscp);

    if (sd->tos != -1 && packet->findTag<TosReq>() == nullptr) {
        packet->addTag<TosReq>()->setTos(sd->tos);
        if (packet->findTag<DscpReq>())
            throw cRuntimeError("setting error: TOS and DSCP found together");
    }

    // TODO apps use ModuleIdAddress if the network interface doesn't have an IP address configured, and UDP uses NextHopForwarding which results in a weird error in MessageDispatcher
    const Protocol *l3Protocol = destAddr.getAddressType()->getNetworkProtocol();

    auto udpHeader = makeShared<UdpHeader>();
    // set source and destination port
    udpHeader->setSourcePort(srcPort);
    udpHeader->setDestinationPort(destPort);

    B totalLength = udpHeader->getChunkLength() + packet->getTotalLength();
    if (totalLength.get() > mtu_)
        throw cRuntimeError("send: total UDP message size exceeds %u", mtu_);

    udpHeader->setTotalLengthField(totalLength);
    if (crcMode == CRC_COMPUTED) {
        udpHeader->setCrcMode(CRC_COMPUTED);
        udpHeader->setCrc(0x0000); // crcMode == CRC_COMPUTED is done in an INetfilter hook
    }
    else {
        udpHeader->setCrcMode(crcMode);
        insertCrc(l3Protocol, srcAddr, destAddr, udpHeader, packet);
    }

    insertTransportProtocolHeader(packet, Protocol::udp, udpHeader);
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(l3Protocol);
    packet->setKind(0);

    EV_INFO << "Sending app packet " << packet->getName() << " over " << l3Protocol->getName() << ".\n";
    emit(packetSentSignal, packet);
    emit(packetSentToLowerSignal, packet);
    send(packet, "ipOut");
    numSent++;
}
