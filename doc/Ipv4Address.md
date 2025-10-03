## Special Address Types

1. Ipv4Address::UNSPECIFIED_ADDRESS

Value: 0.0.0.0

Meaning:

“No address” or “unspecified address.”

Used when a host does not yet have an IP assigned (e.g. during DHCP discovery).

Also used as the wildcard bind address: if a UDP/TCP app binds its socket to 0.0.0.0, it will accept connections/datagrams sent to any of the host’s IP addresses.

Routing tables can also use 0.0.0.0/0 to represent the “default route.”

2. Ipv4Address::LOOPBACK_ADDRESS("127.0.0.1")

Value: 127.0.0.1

Meaning:

Loopback address; traffic sent to this never leaves the host, it is delivered back to the same node internally.

Every IPv4 host must support loopback.

Useful for testing: apps can talk to each other via 127.0.0.1 without requiring a network card or IP stack setup.

3. Ipv4Address::LOOPBACK_NETMASK("255.0.0.0")

Value: 255.0.0.0

Meaning:

This is the netmask of the loopback network 127.0.0.0/8.

The entire block 127.x.x.x (with netmask 255.0.0.0) is reserved for loopback.

So not just 127.0.0.1, but all addresses 127.0.0.0 – 127.255.255.255 refer to the local host.

4. Ipv4Address::ALLONES_ADDRESS("255.255.255.255")

Value: 255.255.255.255

Meaning:

The limited broadcast address.

Sent by a host to reach all hosts on the local link.

It is not forwarded by routers.

Used for “I don’t know the specific address, but I want everyone nearby to hear this” — e.g., some bootstrapping protocols (DHCPDISCOVER).

Applications in INET can use this as the destination address to make sure packets are delivered to all neighbors on the same subnet.

✅ So in practice (INET/OMNeT++ simulation context):

UNSPECIFIED_ADDRESS (0.0.0.0) = wildcard/no address.

LOOPBACK_ADDRESS (127.0.0.1) = talk to yourself only.

LOOPBACK_NETMASK (255.0.0.0) = loopback network mask (127/8).

ALLONES_ADDRESS (255.255.255.255) = broadcast to everyone on the same link.


### How they works

🔹 Ipv4Address::UNSPECIFIED_ADDRESS (0.0.0.0)

Meaning in INET: “any” or “unspecified.”

If used as a local address:

A UDP/TCP sink app binding to 0.0.0.0 means “accept packets to any of my addresses.”

Very common: if your gNB app binds to localAddress = "0.0.0.0", it will receive packets sent to any of its interfaces.

If used as a destination address:

IPv4 module will drop it. It’s not routable.

So you cannot send UE→gNB traffic with dest = 0.0.0.0.

👉 Use this only for binding apps, not for sending.

🔹 Ipv4Address::LOOPBACK_ADDRESS (127.0.0.1)

Meaning in INET: “deliver only inside the same node.”

If used as a local address:

An app can bind to 127.0.0.1 and only receive loopback traffic.

If used as a destination:

The packet is looped back in the same node — it never leaves the UE’s IPv4 module.

So if your UE app sends to 127.0.0.1, it will be delivered back to the UE, not to the gNB.

👉 Useful only for intra-node communication / testing, not for UE→gNB.

🔹 Ipv4Address::LOOPBACK_NETMASK (255.0.0.0)

Meaning in INET: the netmask for the loopback block 127.0.0.0/8.

Not typically used directly in apps.

It just defines the loopback subnet; any 127.x.x.x address behaves like loopback in INET.

Like above, packets won’t leave the node.

👉 You usually won’t set this in .ini.

🔹 Ipv4Address::ALLONES_ADDRESS (255.255.255.255)

Meaning in INET: the limited broadcast.

If used as a destination address:

IPv4 delivers the packet out of the UE’s wireless NIC.

Any node in the same broadcast domain (like the gNB) will receive it, regardless of its unicast IP.

In your case: the UE can send to 255.255.255.255 and the gNB’s UDP sink (bound to port N, with localAddress = "0.0.0.0") will accept it.

If used as a local bind address:

Rare; normally you bind to 0.0.0.0 instead.

👉 This is your best option if you don’t want to hard-code the gNB’s IP.

✅ Summary for your UE→gNB case

0.0.0.0 → use for gNB UDP local bind (wildcard).

127.0.0.1 / 127/8 → stays inside the same node, not usable for UE→gNB.

255.255.255.255 → broadcast from UE, delivered to gNB’s UDP app without knowing gNB’s IP.
