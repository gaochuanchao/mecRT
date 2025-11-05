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
INPUT_DIR = os.path.join(WORKING_DIR, "results/Normal")
OUTPUT_DIR = os.path.join(WORKING_DIR, "result-analysis/normal")
# Ensure output directory exists
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Regex patterns to extract fields from filename
# file name example:  Greedy-interval-10-appCount-3.sca, Greedy-interval-10-appCount-3.vec
match_pattern = re.compile(r'^([^-]+)-interval-([0-9]+)-appCount-([0-9]+)')


def extract_expected_utility(app_count):
    scalar_name = "schemeUtility:mean"
    output_csv = os.path.join(OUTPUT_DIR, f"appCount_{app_count}_expected_utility_mean.csv")

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["algorithm", "interval", "utility"])

        # Loop through .sca files in INPUT_DIR
        for filename in os.listdir(INPUT_DIR):
            if not filename.endswith(".sca"):
                continue

            # Parse filename parameters
            file_match = match_pattern.search(filename)

            if not file_match:
                continue

            algorithm = file_match.group(0)
            interval = file_match.group(1)
            appCount = file_match.group(2)

            # Skip files that don't match required appCount values
            if appCount != app_count:
                continue

            # Read the file and extract scalar value line
            filepath = os.path.join(INPUT_DIR, filename)
            value = None
            with open(filepath, 'r') as f:
                for line in f:
                    # line format: scalar <module> <name> <value>
                    # e.g., scalar DeMEC.gnb[6].scheduler schemeUtility:mean 45.070333333333
                    if line.startswith("scalar") and scalar_name in line:
                        parts = line.strip().split()
                        if len(parts) >= 4 and parts[2] == scalar_name:
                            value = parts[3]
                            break  # stop after first match

            csv_writer.writerow([algorithm, interval, value])

    print(f"\t expected energy extraction complete.\n\t saved to: {output_csv}")


def extract_improved_utility(app_count):
    params_list = ["utility:vector", "meetDlPkt:vector"]
    output_csv = os.path.join(OUTPUT_DIR, f"appCount_{app_count}_improved_utility.csv")

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["algorithm", "interval"] + params_list)

        # Loop through .vec files in INPUT_DIR
        for filename in os.listdir(INPUT_DIR):
            if not filename.endswith(".vec"):
                continue

            # Parse filename parameters
            file_match = match_pattern.search(filename)

            if not file_match:
                continue

            algorithm = file_match.group(1)
            interval = file_match.group(2)
            appCount = file_match.group(3)

            # Skip files that don't match required appCount values
            if appCount != app_count:
                continue

            # Read the file and extract scalar value lines
            filepath = os.path.join(INPUT_DIR, filename)
            values = {param: None for param in params_list}
            with open(filepath, 'r') as f:
                for line in f:
                    # line format: scalar <module> <name> <value>
                    # e.g., scalar DeMEC.gnb[6].scheduler utility:vector 45.070333333333
                    if line.startswith("scalar"):
                        parts = line.strip().split()
                        if len(parts) >= 4:
                            scalar_name = parts[2]
                            if scalar_name in values:
                                values[scalar_name] = parts[3]

            csv_writer.writerow([algorithm, interval] + [values[param] for param in params_list])

    print(f"\t improved utility extraction complete.\n\t saved to: {output_csv}")


if __name__ == "__main__":
    for appCount in [3, 4]:
        print(f"\n==============================================================")
        print(f">>> Starting extraction for appCount={appCount}...\n")

        print(">>> Starting extracting expected utility...")
        extract_expected_utility(appCount)

        print(">>> Starting extracting vehSavedEnergy...")
        extract_veh_saved_energy(appCount)

        print(">>> Starting extracting offload energy...")
        extract_offload_energy(appCount)

    print(">>> All tasks completed successfully.")
