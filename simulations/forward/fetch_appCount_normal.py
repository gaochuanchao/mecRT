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
import pandas as pd

# === Configurable Parameters ===
WORKING_DIR = os.path.dirname(os.path.abspath(__file__))
INPUT_DIR = os.path.join(WORKING_DIR, "results/Normal")
OUTPUT_DIR = os.path.join(WORKING_DIR, "result-analysis/normal")
# Ensure output directory exists
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Regex patterns to extract fields from filename
# file name:  Greedy-interval-10-appCount-3.sca, Greedy-interval-10-appCount-3.vec
# algorithm: Greedy
algorithm_pattern = re.compile(r'^([^-]+)-interval-([0-9]+)-appCount-([0-9]+)')

def summary_app_count():
    output_csv = os.path.join(OUTPUT_DIR, f"app_count_summary.csv")

    params_list = ["pendingAppCount:vector", "schemeUtility:vector", "schemeTime:vector"]

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["time", "algorithm"] + params_list)

        # Loop through .vec files in INPUT_DIR
        for filename in os.listdir(INPUT_DIR):
            if not filename.endswith(".vec"):
                continue

            # Parse filename parameters
            scheAll_match = algorithm_pattern.search(filename)
            if not scheAll_match:
                continue

            algorithm = scheAll_match.group(1) if scheAll_match else "Unknown"

            # === Step 1: Parse vector IDs ===
            # Read the file and extract scalar value line
            filepath = os.path.join(INPUT_DIR, filename)
            vector_id2params = {}  # name -> id
            with open(filepath, 'r') as f:
                # read line sequentially
                # find the line that starts with "vector"
                # e.g., vector 0 VEC.scheduler.vecScheduler pendingAppCount:vector ETV
                # where 0 is the vector id, and pendingAppCount:vector is the vector name
                for line in f:
                    if line.startswith("vector"):
                        parts = line.strip().split()
                        if len(parts) >= 4:
                            vec_id, vec_name = parts[1], parts[3]
                            for param in params_list:
                                if vec_name == param:
                                    vector_id2params[vec_id] = param

                    # if line starts with 0, then break
                    if line.startswith("0"):
                        break

            # === Step 2: Extract vector values ===
            # e.g., "pendingAppCount:vector": 1	1006866	70.058	29
            # where 1 is the vector id, 1006866 is the event id, 70.058 is the time, and 29 is the value
            data = {key: {} for key in params_list}
            timeSet = set()  # to collect all unique time values
            with open(filepath, 'r') as f:
                for line in f:
                    if re.match(r'^\d+\s+\d+', line):  # numeric line
                        parts = line.strip().split()
                        vec_id = parts[0]
                        if vec_id in vector_id2params:
                            t = float(parts[2])
                            v = float(parts[3])
                            param = vector_id2params[vec_id]
                            data[param][t] = v
                            timeSet.add(t)
            
            # === Step 3: writing rows to CSV ===
            timeList = sorted(timeSet)

            for t in timeList:
                row = [t, algorithm]
                for param in params_list:
                    row.append(data[param].get(t, 0))
                csv_writer.writerow(row)

    print(f"\t App count summary extraction complete.\n\t saved to: {output_csv}")


def summary_app_count_append_measured():
    output_csv = os.path.join(OUTPUT_DIR, f"app_count_summary.csv")

    params_list = ["pendingAppCount:vector", "schemeUtility:vector", "schemeTime:vector"]

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["time", "algorithm"] + params_list + ["measured_utility"])

        # Loop through .vec files in INPUT_DIR
        for filename in os.listdir(INPUT_DIR):
            if not filename.endswith(".vec"):
                continue

            # Parse filename parameters
            scheAll_match = algorithm_pattern.search(filename)
            if not scheAll_match:
                continue

            algorithm = scheAll_match.group(1) if scheAll_match else "Unknown"
            interval = scheAll_match.group(2) if scheAll_match else "Unknown"
            interval = int(interval)

            # === Step 1: Parse vector IDs ===
            # Read the file and extract scalar value line
            filepath = os.path.join(INPUT_DIR, filename)
            vector_id2params = {}  # name -> id
            set_id2measured = set()  # id -> measured value
            with open(filepath, 'r') as f:
                # read line sequentially
                # find the line that starts with "vector"
                # e.g., vector 0 VEC.scheduler.vecScheduler pendingAppCount:vector ETV
                # where 0 is the vector id, and pendingAppCount:vector is the vector name
                for line in f:
                    if line.startswith("vector"):
                        parts = line.strip().split()
                        if len(parts) >= 4:
                            vec_id, vec_name = parts[1], parts[3]
                            for param in params_list:
                                if vec_name == param:
                                    vector_id2params[vec_id] = param
                            
                            if vec_name == "utility:vector":
                                set_id2measured.add(vec_id)

                    # if line starts with 0, then break
                    if line.startswith("0"):
                        break

            # === Step 2: Extract vector values ===
            # e.g., "pendingAppCount:vector": 1	1006866	70.058	29
            # where 1 is the vector id, 1006866 is the event id, 70.058 is the time, and 29 is the value
            data = {key: {} for key in params_list}
            measured_data = {}
            timeSet = set()  # to collect all unique time values
            with open(filepath, 'r') as f:
                for line in f:
                    if re.match(r'^\d+\s+\d+', line):  # numeric line
                        parts = line.strip().split()
                        vec_id = parts[0]
                        if vec_id in vector_id2params:
                            t = float(parts[2])
                            v = float(parts[3])
                            param = vector_id2params[vec_id]
                            data[param][t] = v
                            timeSet.add(t)
                        if vec_id in set_id2measured:
                            t = float(parts[2])
                            v = float(parts[3])
                            adjusted_t = (t//interval) * interval + 0.058
                            value = measured_data.get(adjusted_t, 0)
                            value += v
                            measured_data[adjusted_t] = value

            # === Step 3: writing rows to CSV ===
            timeList = sorted(timeSet)
            for t in timeList:
                row = [t, algorithm]
                for param in params_list:
                    row.append(data[param].get(t, 0))
                
                row.append(measured_data.get(t, 0)/interval)  # average over interval
                csv_writer.writerow(row)

    print(f"\t App count summary extraction complete.\n\t saved to: {output_csv}")


if __name__ == "__main__":
    # summary_app_count()
    summary_app_count_append_measured()
    print(">>> All tasks completed successfully.")
