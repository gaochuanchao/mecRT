#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# @Time    : 18/9/2025 10:00 AM
# @Author  : Gao Chuanchao
# @Email   : gaoc0008@e.ntu.edu.sg
# @File    : topologyGen.py
#
# This script reads an adjacency matrix from a file and generates
# the corresponding gnbRouter to gnbRouter connection lines.


def generate_connections_from_file(filename, channel="Eth100G", prefix="gnb"):
    # Read adjacency matrix from file
    with open(filename, "r") as f:
        lines = f.readlines()
    
    matrix = []
    for line in lines:
        row = list(map(int, line.strip().split()))
        if row:  # skip empty lines
            matrix.append(row)

    n = len(matrix)
    connections = []

    # Generate connections
    for i in range(n):
        for j in range(i + 1, n):  # upper triangular to avoid duplicates
            if matrix[i][j] == 1:
                connections.append(
                    f"{prefix}[{i}].pppg++ <--> {channel} <--> {prefix}[{j}].pppg++;"
                )

    return connections


if __name__ == "__main__":
    map_size = 4    # 0 is default, 1 is 1km x 1km, 2 is 2km x 2km, 3 is 3km x 3km, 4 is 4km x 4km
    filename = f"./map_{map_size}/ES_graph.txt"
    output_filename = f"./map_{map_size}/connections.txt"
    result = generate_connections_from_file(filename)

    # save to output file
    with open(output_filename, "w") as f:
        f.write("\n".join(result) + "\n")

    print("Connections generated to file", output_filename)
