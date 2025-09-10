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
INPUT_DIR = os.path.join(WORKING_DIR, "results/Full-Test-MEC-1")
OUTPUT_DIR = os.path.join(WORKING_DIR, "result-analysis/mec1")
# Ensure output directory exists
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Regex patterns to extract fields from filename
scheAll_pattern = re.compile(r"scheAll-([^-\s]+)")
countExeTime_pattern = re.compile(r"countExeTime-([^-\s]+)")
algorithm_pattern = re.compile(r"countExeTime-[^-]+-([^-\.]+)")
pilot_pattern = re.compile(r"pilot-([^-\.\s]+)")


def extract_expected_energy(sche_all, count_exe_time):
    scalar_name = "savedEnergy:sum"
    output_csv = os.path.join(OUTPUT_DIR, f"scheAll_{sche_all}_countExeTime_{count_exe_time}_expected_energy_sum.csv")

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["algorithm", "pilot", "energy"])

        # Loop through .sca files in INPUT_DIR
        for filename in os.listdir(INPUT_DIR):
            if not filename.endswith(".sca"):
                continue

            # Parse filename parameters
            scheAll_match = scheAll_pattern.search(filename)
            countExeTime_match = countExeTime_pattern.search(filename)

            if not scheAll_match or not countExeTime_match:
                continue

            scheAll = scheAll_match.group(1)
            countExeTime = countExeTime_match.group(1)

            # Skip files that don't match required scheAll and countExeTime values
            if scheAll != sche_all or countExeTime != count_exe_time:
                continue

            algorithm_match = algorithm_pattern.search(filename)
            pilot_match = pilot_pattern.search(filename)
            if not algorithm_match or not pilot_match:
                continue

            algorithm = algorithm_match.group(1)
            pilot = pilot_match.group(1)

            # Read the file and extract scalar value line
            filepath = os.path.join(INPUT_DIR, filename)
            value = None
            with open(filepath, 'r') as f:
                for line in f:
                    # line format: scalar <module> <name> <value>
                    if line.startswith("scalar") and scalar_name in line:
                        parts = line.strip().split()
                        if len(parts) >= 4 and parts[2] == scalar_name:
                            value = parts[3]
                            break  # stop after first match

            csv_writer.writerow([algorithm, pilot, value])

    print(f"\t expected energy extraction complete.\n\t saved to: {output_csv}")


def extract_veh_saved_energy(sche_all, count_exe_time):
    scalar_name = "vehSavedEnergy:sum"
    output_csv = os.path.join(OUTPUT_DIR, f"scheAll_{sche_all}_countExeTime_{count_exe_time}_actual_energy_sum.csv")

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["algorithm", "pilot", "energy"])

        # Loop through .sca files in INPUT_DIR
        for filename in os.listdir(INPUT_DIR):
            if not filename.endswith(".sca"):
                continue

            # Parse filename parameters
            scheAll_match = scheAll_pattern.search(filename)
            countExeTime_match = countExeTime_pattern.search(filename)

            if not scheAll_match or not countExeTime_match:
                continue

            scheAll = scheAll_match.group(1)
            countExeTime = countExeTime_match.group(1)

            # Skip files that don't match required scheAll and countExeTime values
            if scheAll != sche_all or countExeTime != count_exe_time:
                continue

            algorithm_match = algorithm_pattern.search(filename)
            pilot_match = pilot_pattern.search(filename)
            if not algorithm_match or not pilot_match:
                continue

            algorithm = algorithm_match.group(1)
            pilot = pilot_match.group(1)

            # Read the file and extract scalar value line
            filepath = os.path.join(INPUT_DIR, filename)
            value = None
            with open(filepath, 'r') as f:
                for line in f:
                    # line format: scalar <module> <name> <value>
                    if line.startswith("scalar") and scalar_name in line:
                        parts = line.strip().split()
                        if len(parts) >= 4 and parts[2] == scalar_name:
                            value = parts[3]
                            csv_writer.writerow([algorithm, pilot, value])

    print(f"\t vehSavedEnergy extraction complete.\n\t saved to: {output_csv}")


def extract_offload_energy(sche_all, count_exe_time):
    scalar_name = "offloadEnergyConsumed:sum"
    output_csv = os.path.join(OUTPUT_DIR, f"scheAll_{sche_all}_countExeTime_{count_exe_time}_offload_energy_sum.csv")

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["algorithm", "pilot", "energy"])

        # Loop through .sca files in INPUT_DIR
        for filename in os.listdir(INPUT_DIR):
            if not filename.endswith(".sca"):
                continue

            # Parse filename parameters
            scheAll_match = scheAll_pattern.search(filename)
            countExeTime_match = countExeTime_pattern.search(filename)

            if not scheAll_match or not countExeTime_match:
                continue

            scheAll = scheAll_match.group(1)
            countExeTime = countExeTime_match.group(1)

            # Skip files that don't match required scheAll and countExeTime values
            if scheAll != sche_all or countExeTime != count_exe_time:
                continue

            algorithm_match = algorithm_pattern.search(filename)
            pilot_match = pilot_pattern.search(filename)
            if not algorithm_match or not pilot_match:
                continue

            algorithm = algorithm_match.group(1)
            pilot = pilot_match.group(1)

            # Read the file and extract scalar value line
            filepath = os.path.join(INPUT_DIR, filename)
            value = None
            with open(filepath, 'r') as f:
                for line in f:
                    # line format: scalar <module> <name> <value>
                    if line.startswith("scalar") and scalar_name in line:
                        parts = line.strip().split()
                        if len(parts) >= 4 and parts[2] == scalar_name:
                            value = parts[3]
                            break  # stop after first match
            
            csv_writer.writerow([algorithm, pilot, value])

    print(f"\t offload energy extraction complete.\n\t saved to: {output_csv}")


if __name__ == "__main__":
    for scheAll in ["true", "false"]:
        for countExeTime in ["true", "false"]:
            print(f"\n==============================================================")
            print(f">>> Starting extraction for scheAll={scheAll}, countExeTime={countExeTime}...\n")

            print(">>> Starting extracting expected energy...")
            extract_expected_energy(scheAll, countExeTime)

            print(">>> Starting extracting vehSavedEnergy...")
            extract_veh_saved_energy(scheAll, countExeTime)

            print(">>> Starting extracting offload energy...")
            extract_offload_energy(scheAll, countExeTime)

    print(">>> All tasks completed successfully.")
