I’m going to explain the assignment mechanics using your exact rules so you can see which subnet each interface ends up in and why the masks matter.Yes. The key idea is that the IPv4 configurator does two things for each matching interface:

1. It decides which rule applies, based on `hosts` and `names`.
2. It assigns an address from the subnet described by `address` plus `netmask`.

For your current demo.xml, the rules are:

- Cellular interfaces on `*.ue[*]` and `*.gnb[*]` with interface name `cellular`
- Backhaul interfaces on `*.gnb[*]` with interface name `ppp*`
- Loopback interfaces on everything with name `lo*`

## 1. Cellular rule

```xml
<interface hosts="*.ue[*] *.gnb[*]" names="cellular" address="10.x.x.x" netmask="255.255.224.0"/>
```

This means:
- Any interface named `cellular` inside a UE or gNB matches this rule.
- The `10.x.x.x` pattern means the configurator starts from the `10.0.0.0` block and fills in the variable octets as needed.
- `255.255.224.0` is `/19`, so that rule gives one subnet with 8190 usable addresses.

Practical result:
- All your UE cellular interfaces and gNB cellular interfaces are assigned unique IPs from the `10.0.0.0/19` range.
- Example addresses would look like `10.0.0.1`, `10.0.0.2`, `10.0.0.3`, and so on.
- The exact numbering is handled by the configurator, so you do not manually choose each IP.

Why this works for 5k:
- 5,000 interfaces fit inside a subnet with 8190 usable addresses.
- That is why `/19` is enough.

## 2. Backhaul rule

```xml
<interface hosts="*.gnb[*]" names="ppp*" address="11.x.x.x" netmask="255.255.255.252"/>
```

This matches:
- Any gNB interface whose name starts with `ppp`

This is used for point-to-point backhaul links between gNBs or between a gNB and another backhaul peer.

`255.255.255.252` is `/30`, which means:
- 4 total addresses
- 2 usable host addresses

That is exactly what you want for a point-to-point link:
- one IP for one end
- one IP for the other end

So each ppp link gets its own tiny subnet, such as:
- `11.0.0.0/30`
- `11.0.0.4/30`
- `11.0.0.8/30`

The configurator increments to the next /30 block for each new ppp link.

## 3. Loopback rule

```xml
<interface hosts="*" names="lo*" address="127.x.x.x" netmask="255.0.0.0"/>
```

This matches:
- Any interface starting with `lo`

This is the loopback interface on each node. The `127.0.0.0/8` range is reserved for loopback, so each node gets a loopback address in that block.

Typical result:
- Node loopbacks might become `127.0.0.1`, `127.0.0.2`, etc.
- The configurator keeps them inside the `127.*.*.*` range.

## How matching works in practice

For each interface, the configurator checks the rules and assigns the first matching rule that applies to that interface category.

For example, on a UE:
- `cellular` matches the cellular rule
- `lo` matches the loopback rule
- it does not match `ppp*`, because UEs do not have those backhaul interfaces

On a gNB:
- `cellular` matches the cellular rule
- `ppp0`, `ppp1`, etc. match the ppp rule
- `lo` matches the loopback rule

## Why your earlier error happened

Your earlier setup had a subnet that was too small for the total number of interfaces matched by the rule. The configurator ran out of space while trying to assign unique addresses.

The fix was:
- keep cellular in a larger pool, now `/19`
- keep ppp in `/30`
- keep loopback in `127/8`

## Simple mental model

Think of each rule as a bucket:

- Cellular bucket: all UE/gNB radio interfaces go here
- PPP bucket: all backhaul point-to-point interfaces go here
- Loopback bucket: all local loopback interfaces go here

Each bucket has:
- a starting address pattern
- a subnet size
- a set of matching interfaces

If you want, I can also draw the exact address flow for one UE and one gNB, step by step, so you can see which interfaces get which addresses.