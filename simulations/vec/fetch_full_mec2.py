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
INPUT_DIR = os.path.join(WORKING_DIR, "results/Full-Test-MEC-2")
OUTPUT_DIR = os.path.join(WORKING_DIR, "result-analysis/mec2")
# Ensure output directory exists
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Regex patterns to extract fields from filename
scheAll_pattern = re.compile(r"scheAll-([^-\s]+)")
factor_pattern = re.compile(r"factor-([^-]+)")
algorithm_pattern = re.compile(r"factor-[^-]+-([^-\.]+)")
pilot_pattern = re.compile(r"pilot-([^-\.\s]+)")


def extract_expected_energy(sche_all, factor_value):
    scalar_name = "savedEnergy:sum"
    output_csv = os.path.join(OUTPUT_DIR, f"scheAll_{sche_all}_factor_{factor_value}_expected_energy_sum.csv")

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["algorithm", "pilot", "energy"])

        # Loop through .sca files in INPUT_DIR
        for filename in os.listdir(INPUT_DIR):
            if not filename.endswith(".sca"):
                continue

            # Parse filename parameters
            scheAll_match = scheAll_pattern.search(filename)
            factor_match = factor_pattern.search(filename)

            if not scheAll_match or not factor_match:
                continue

            scheAll = scheAll_match.group(1)
            factor = factor_match.group(1)

            # Skip files that don't match required scheAll and factor values
            if scheAll != sche_all or factor != factor_value:
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


def extract_veh_saved_energy(sche_all, factor_value):
    scalar_name = "vehSavedEnergy:sum"
    output_csv = os.path.join(OUTPUT_DIR, f"scheAll_{sche_all}_factor_{factor_value}_actual_energy_sum.csv")

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["algorithm", "pilot", "energy"])

        # Loop through .sca files in INPUT_DIR
        for filename in os.listdir(INPUT_DIR):
            if not filename.endswith(".sca"):
                continue

            # Parse filename parameters
            scheAll_match = scheAll_pattern.search(filename)
            factor_match = factor_pattern.search(filename)

            if not scheAll_match or not factor_match:
                continue

            scheAll = scheAll_match.group(1)
            factor = factor_match.group(1)

            # Skip files that don't match required scheAll and factor values
            if scheAll != sche_all or factor != factor_value:
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


def extract_offload_energy(sche_all, factor_value):
    scalar_name = "offloadEnergyConsumed:sum"
    output_csv = os.path.join(OUTPUT_DIR, f"scheAll_{sche_all}_factor_{factor_value}_offload_energy_sum.csv")

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["algorithm", "pilot", "energy"])

        # Loop through .sca files in INPUT_DIR
        for filename in os.listdir(INPUT_DIR):
            if not filename.endswith(".sca"):
                continue

            # Parse filename parameters
            scheAll_match = scheAll_pattern.search(filename)
            factor_match = factor_pattern.search(filename)

            if not scheAll_match or not factor_match:
                continue

            scheAll = scheAll_match.group(1)
            factor = factor_match.group(1)

            # Skip files that don't match required scheAll and factor values
            if scheAll != sche_all or factor != factor_value:
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
        for factorVal in ["0.17", "0.25", "0.5"]:
            print(f"\n==============================================================")
            print(f">>> Starting extraction for scheAll={scheAll}, factorVal={factorVal}...\n")

            print(">>> Starting extracting expected energy...")
            extract_expected_energy(scheAll, factorVal)

            print(">>> Starting extracting vehSavedEnergy...")
            extract_veh_saved_energy(scheAll, factorVal)

            print(">>> Starting extracting offload energy...")
            extract_offload_energy(scheAll, factorVal)
    print(">>> All tasks completed successfully.")
