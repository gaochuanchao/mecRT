#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# @Time    : 18/9/2025 10:00 AM
# @Author  : Gao Chuanchao
# @Email   : gaoc0008@e.ntu.edu.sg
# @File    : ipGenerator.py
#
# This generator handles:
#   - numGnb gNBs and numUe UEs per gNB
#   - gNB ↔ gnbUpf PPP links (/30)
#   - gnbUpf ↔ gnbUpf interconnections from an adjacency matrix (/30)
#   - gnbUpf ↔ central UPF (/30)
#   - UPF ↔ SchedulerHost (/30)
#   - Cellular subnet for gNBs and UEs (/24)
#   - Dynamic pppg vectors per gnbUpf
#   - Fully concrete IPs

# --------------------------
# Helper function: convert a sequential number to a valid 4-octet IPv4
# --------------------------
def subnet_ip(counter):
    third = counter // 256
    fourth = counter % 256
    return f"10.0.{third}.{fourth}"


if __name__ == "__main__":
    numGnb = 15           # number of gNBs
    numUe = 80            # number of UEs per gNB
    numGnbUpf = 15       # number of gnbUpf nodes

    # read the adjacency matrix from a file
    filePath = "./traffic/RSU15_graph.txt"

    # Example adjacency matrix (15x15)
    adj_matrix = []
    with open(filePath, 'r') as f:
        for line in f:
            row = list(map(int, line.strip().split()))
            adj_matrix.append(row)

    xml = ['<?xml version="1.0"?>', '<config>']

    # --------------------------
    # Cellular subnet for gNBs and UEs (/24)
    # --------------------------
    cellular_base = 0
    for gnb_idx in range(numGnb):
        gnb_ip = f"10.0.{cellular_base + gnb_idx}.1"
        xml.append(f'    <interface hosts="gnb[{gnb_idx}]" names="cellular" address="{gnb_ip}" netmask="255.255.255.0"/>')
    for ue_idx in range(numUe):
        ue_ip = f"10.0.{cellular_base + ue_idx}.2"
        xml.append(f'    <interface hosts="ue[{ue_idx}]" names="cellular" address="{ue_ip}" netmask="255.255.255.0"/>')


    # --------------------------
    # gNB[i] <-> gnbUpf[i] PPP links (/30)
    # --------------------------
    ppp_counter = 10
    pppg_index = [0]*numGnbUpf  # track interface vector index

    for gnb_idx in range(numGnb):
        gnb_ppp_ip = subnet_ip(ppp_counter)
        upf_ppp_ip = subnet_ip(ppp_counter + 1)
        xml.append(f'    <interface hosts="gnb[{gnb_idx}]" names="ppp" address="{gnb_ppp_ip}" netmask="255.255.255.252"/>')
        xml.append(f'    <interface hosts="gnbUpf[{gnb_idx}]" names="pppg[{pppg_index[gnb_idx]}]" address="{upf_ppp_ip}" netmask="255.255.255.252"/>')
        pppg_index[gnb_idx] += 1
        ppp_counter += 4

    # --------------------------
    # gnbUpf[i] <-> gnbUpf[j] PPP links from adjacency matrix (/30)
    # --------------------------
    ppp_counter = 100  # start subnet counter for inter-gnbUpf links

    for i in range(numGnbUpf):
        for j in range(i+1, numGnbUpf):
            if adj_matrix[i][j] == 1:
                ip1 = subnet_ip(ppp_counter)
                ip2 = subnet_ip(ppp_counter + 1)
                xml.append(f'    <interface hosts="gnbUpf[{i}]" names="pppg[{pppg_index[i]}]" address="{ip1}" netmask="255.255.255.252"/>')
                xml.append(f'    <interface hosts="gnbUpf[{j}]" names="pppg[{pppg_index[j]}]" address="{ip2}" netmask="255.255.255.252"/>')
                pppg_index[i] += 1
                pppg_index[j] += 1
                ppp_counter += 4

    # --------------------------
    # gnbUpf[0] <-> central UPF (/30)
    # --------------------------
    xml.append(f'    <interface hosts="gnbUpf[0]" names="pppg[{pppg_index[0]}]" address="{subnet_ip(200)}" netmask="255.255.255.252"/>')
    xml.append(f'    <interface hosts="upf" names="pppg[0]" address="{subnet_ip(201)}" netmask="255.255.255.252"/>')

    # --------------------------
    # UPF <-> SchedulerHost (/30)
    # --------------------------
    xml.append('    <interface hosts="upf" names="filterGate" address="10.0.250.1" netmask="255.255.255.252"/>')
    xml.append('    <interface hosts="schedulerHost" names="ppp" address="10.0.250.2" netmask="255.255.255.252"/>')

    xml.append('</config>')

    # Save to file
    with open("ipConfig.xml", "w") as f:
        f.write("\n".join(xml))

    print("ipConfig.xml generated successfully!")
