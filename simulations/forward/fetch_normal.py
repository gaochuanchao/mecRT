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


def extract_expected_utility():
    scalar_name = "schemeUtility:mean"
    output_csv = os.path.join(OUTPUT_DIR, f"expected_utility_mean.csv")

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

            algorithm = file_match.group(1)
            interval = file_match.group(2)

            # Read the file and extract scalar value line
            filepath = os.path.join(INPUT_DIR, filename)
            value = None
            with open(filepath, 'r') as f:
                for line in f:
                    # line format: scalar <module> <name> <value>
                    # e.g., scalar DeMEC.gnb[6].scheduler schemeUtility:mean 45.070333333333
                    if line.startswith("scalar") and scalar_name in line:
                        parts = line.strip().split()
                        if len(parts) >= 4 and parts[2] == scalar_name and parts[3] != "-nan":
                            value = parts[3]
                            break  # stop after first match

            csv_writer.writerow([algorithm, interval, value])

    print(f"\t expected energy extraction complete.\n\t saved to: {output_csv}")


def extract_expected_job_count():
    scalar_name = "expectedJobsToBeOffloaded:mean"
    output_csv = os.path.join(OUTPUT_DIR, f"expected_job_count_mean.csv")

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["algorithm", "interval", "job_count"])

        # Loop through .sca files in INPUT_DIR
        for filename in os.listdir(INPUT_DIR):
            if not filename.endswith(".sca"):
                continue

            # Parse filename parameters
            file_match = match_pattern.search(filename)

            if not file_match:
                continue

            algorithm = file_match.group(1)
            interval = file_match.group(2)

            # Read the file and extract scalar value line
            filepath = os.path.join(INPUT_DIR, filename)
            value = None
            with open(filepath, 'r') as f:
                for line in f:
                    # line format: scalar <module> <name> <value>
                    # e.g., scalar DeMEC.gnb[6].scheduler schemeUtility:mean 45.070333333333
                    if line.startswith("scalar") and scalar_name in line:
                        parts = line.strip().split()
                        if len(parts) >= 4 and parts[2] == scalar_name and parts[3] != "-nan":
                            value = parts[3]
                            break  # stop after first match

            csv_writer.writerow([algorithm, interval, value])

    print(f"\t expected energy extraction complete.\n\t saved to: {output_csv}")


def extract_improved_utility():
    params_list = ["utility:sum", "meetDlPkt:sum", "jobGeneratedSinceGranted:sum"]
    output_csv = os.path.join(OUTPUT_DIR, f"improved_utility_mean.csv")

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["algorithm", "interval", "utility:mean", "meetDlPkt:mean", "jobGeneratedSinceGranted:mean"])

        # Loop through .vec files in INPUT_DIR
        for filename in os.listdir(INPUT_DIR):
            if not filename.endswith(".sca"):
                continue

            # Parse filename parameters
            file_match = match_pattern.search(filename)

            if not file_match:
                continue

            algorithm = file_match.group(1)
            interval = file_match.group(2)

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

            csv_writer.writerow([algorithm, interval] + [values[param] for param in params_list])

    print(f"\t improved utility extraction complete.\n\t saved to: {output_csv}")


def check_results():
    filepath = os.path.join(INPUT_DIR, "FastSA-interval-10-appCount-3.vec")
    with open(filepath, 'r') as f:
        for line in f:
            if re.match(r'^\d+\s+\d+', line):  # numeric line
                parts = line.strip().split()
                vec_id = int(parts[0])
                if vec_id == 0:
                    print(line.strip())


if __name__ == "__main__":
    print(f"\n==============================================================")
    print(f">>> Starting extraction...\n")

    print(">>> Starting extracting expected utility...")
    extract_expected_utility()

    print(">>> Starting extracting expected job count...")
    extract_expected_job_count()

    print(">>> Starting extracting actual improved utility...")
    extract_improved_utility()

    print(">>> All tasks completed successfully.")

    # check_results()
