#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# @Time    : 7/4/2024 4:33 PM
# @Author  : Gao Chuanchao
# @Email   : gaoc0008@e.ntu.edu.sg
# @File    : fetch_offload_energy.py
#
# Searches for .sca files in a given directory
# Filters by values of scheAll and countExeTime (both adjustable)
# Extracts a specific scalar (e.g., vehSavedEnergy:sum)
# Parses file names to extract algorithm and pilot
# Writes output as a CSV: algorithm,pilot,energy
#
import os
import re
import csv

# === Configurable Parameters ===
WORKING_DIR = os.path.dirname(os.path.abspath(__file__))
INPUT_DIR = os.path.join(WORKING_DIR, "results/EXP2")
OUTPUT_DIR = os.path.join(WORKING_DIR, "result-analysis/EXP2")
# Ensure output directory exists
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Regex patterns to extract fields from filename
# file name example:  Greedy-mapScale-2-appCount-3.sca, Greedy-mapScale-2-appCount-3.vec
MATCH_PATTERN = re.compile(r'^([^-]+)-mapScale-([0-9]+)-appCount-([0-9]+)')


def extract_improved_utility():
    params_list = ["utility:sum", "meetDlPkt:sum", "jobGeneratedSinceGranted:sum"]
    output_csv = os.path.join(OUTPUT_DIR, f"improved_utility_mean.csv")

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["algorithm", "mapScale", "appCount", "utility:mean", "meetDlPkt:mean", "jobGeneratedSinceGranted:mean"])

        # Loop through .vec files in INPUT_DIR
        for filename in os.listdir(INPUT_DIR):
            if not filename.endswith(".sca"):
                continue

            # Parse filename parameters
            file_match = MATCH_PATTERN.search(filename)

            if not file_match:
                continue

            algorithm = file_match.group(1)
            mapScale = file_match.group(2)
            appCount = file_match.group(3)


            # Read the file and extract scalar value lines
            filepath = os.path.join(INPUT_DIR, filename)
            values = {param: 0 for param in params_list}
            total_sim_time = 900.0  # assuming sim-time-limit is 900s
            found_sim_time = False
            with open(filepath, 'r') as f:
                for line in f:
                    if line.startswith("config") and not found_sim_time:
                        # e.g., config sim-time-limit 100s
                        parts = line.strip().split()
                        if len(parts) >= 3 and parts[1] == "sim-time-limit":
                            time_str = parts[2]
                            if time_str.endswith('s'):
                                total_sim_time = float(time_str[:-1])
                                found_sim_time = True
                    elif line.startswith("scalar"):
                        # line format: scalar <module> <name> <value>
                        # e.g., scalar DeMEC.gnb[0].server utility:sum 267.78629999999
                        # e.g., scalar DeMEC.gnb[0].server meetDlPkt:sum 1829
                        parts = line.strip().split()
                        if len(parts) >= 4:
                            scalar_name = parts[2]
                            if scalar_name in values:
                                values[scalar_name] += float(parts[3])

            # Calculate mean values over total simulation time
            for param in params_list:
                values[param] /= total_sim_time

            csv_writer.writerow([algorithm, mapScale, appCount] + [values[param] for param in params_list])

    print(f"\t improved utility extraction complete.\n\t saved to: {output_csv}")


if __name__ == "__main__":
    print(f"\n==============================================================")
    print(f">>> Starting extraction...\n")

    print(">>> Starting extracting actual improved utility...")
    extract_improved_utility()

    print(">>> All tasks completed successfully.")

    # check_results()
