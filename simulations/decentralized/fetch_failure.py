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
OUTPUT_DIR = os.path.join(WORKING_DIR, "result-analysis/failure")
# Ensure output directory exists
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Regex patterns to extract fields from filename
# file name:  FastSA-routeUpdate-false-errorProb-0.2.vec
# routeUpdate: false, errorProb: 0.1
match_pattern = re.compile(r'routeUpdate-([a-zA-Z]+)-errorProb-([0-9.]+)')


def collect_link_failure():
    output_csv = os.path.join(OUTPUT_DIR, f"link_failure_data.csv")
    params_list = ["utility:vector", "meetDlPkt:vector"]

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["time", "routeUpdate", "errorProb"] + params_list)

        # Loop through .vec files in input_dir
        input_dir = os.path.join(WORKING_DIR, "results/LinkFailure")
        for filename in os.listdir(input_dir):
            if not filename.endswith(".vec"):
                continue

            # Parse filename parameters
            prob_match = match_pattern.search(filename)
            if not prob_match:
                continue

            route_update = prob_match.group(1) if prob_match else "Unknown"
            error_prob = prob_match.group(2) if prob_match else "Unknown"
            # remove the last character if it is '.'
            if error_prob.endswith('.'):
                error_prob = error_prob[:-1]

            # === Step 1: Parse vector IDs ===
            # get the file name without .vec
            indexFile = os.path.join(input_dir, filename[:-4] + ".vci")
            # Read the file and extract scalar value line
            vector_id2params = {}  # name -> id
            with open(indexFile, 'r') as f:
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

            # === Step 2: Extract vector values ===
            # e.g., "pendingAppCount:vector": 1	1006866	70.058	29
            # where 1 is the vector id, 1006866 is the event id, 70.058 is the time, and 29 is the value
            filepath = os.path.join(input_dir, filename)
            data = {key: {} for key in params_list}
            timeSet = set()
            with open(filepath, 'r') as f:
                for line in f:
                    if re.match(r'^\d+\s+\d+', line):  # numeric line
                        parts = line.strip().split()
                        vec_id = parts[0]
                        if vec_id in vector_id2params:
                            t = int(float(parts[2]))    # time
                            v = float(parts[3]) # value
                            param = vector_id2params[vec_id]
                            value = data[param].get(t, 0) + v
                            data[param][t] = value
                            timeSet.add(t)
            
            # === Step 3: writing rows to CSV ===
            timeList = sorted(timeSet)
            for t in timeList:
                row = [t, route_update, error_prob]
                for param in params_list:
                    row.append(data[param].get(t, 0))
                csv_writer.writerow(row)

    print(f"\t App count summary extraction complete.\n\t saved to: {output_csv}")
    # print the first 5 lines of the output csv
    df = pd.read_csv(output_csv)
    print(df.head())


def collect_node_failure():
    output_csv = os.path.join(OUTPUT_DIR, f"node_failure_data.csv")
    params_list = ["utility:vector", "meetDlPkt:vector"]

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["time", "probability"] + params_list)

        # Loop through .vec files in input_dir
        input_dir = os.path.join(WORKING_DIR, "results/NodeFailure")
        for filename in os.listdir(input_dir):
            if not filename.endswith(".vec"):
                continue

            # Parse filename parameters
            prob_match = match_pattern.search(filename)
            if not prob_match:
                continue

            route_update = prob_match.group(1) if prob_match else "Unknown"
            error_prob = prob_match.group(2) if prob_match else "Unknown"
            # remove the last character if it is '.'
            if error_prob.endswith('.'):
                error_prob = error_prob[:-1]

            # === Step 1: Parse vector IDs ===
            # get the file name without .vec
            indexFile = os.path.join(input_dir, filename[:-4] + ".vci")
            # Read the file and extract scalar value line
            vector_id2params = {}  # name -> id
            with open(indexFile, 'r') as f:
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

            # === Step 2: Extract vector values ===
            # e.g., "pendingAppCount:vector": 1	1006866	70.058	29
            # where 1 is the vector id, 1006866 is the event id, 70.058 is the time, and 29 is the value
            filepath = os.path.join(input_dir, filename)
            data = {key: {} for key in params_list}
            timeSet = set()
            with open(filepath, 'r') as f:
                for line in f:
                    if re.match(r'^\d+\s+\d+', line):  # numeric line
                        parts = line.strip().split()
                        vec_id = parts[0]
                        if vec_id in vector_id2params:
                            t = int(float(parts[2]))    # time
                            v = float(parts[3]) # value
                            param = vector_id2params[vec_id]
                            value = data[param].get(t, 0) + v
                            data[param][t] = value
                            timeSet.add(t)
            
            # === Step 3: writing rows to CSV ===
            timeList = sorted(timeSet)
            for t in timeList:
                row = [t, route_update, error_prob]
                for param in params_list:
                    row.append(data[param].get(t, 0))
                csv_writer.writerow(row)

    print(f"\t App count summary extraction complete.\n\t saved to: {output_csv}")
    # print the first 5 lines of the output csv
    df = pd.read_csv(output_csv)
    print(df.head())


def collect_link_recovery_time():
    output_csv = os.path.join(OUTPUT_DIR, f"link_recovery_time.csv")
    target_param = "globalSchedulerReady:vector"

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["error_prob", "recovery_time"])

        # Loop through .sca files in input_dir
        input_dir = os.path.join(WORKING_DIR, "results/LinkFailure")
        for filename in os.listdir(input_dir):
            if not filename.endswith(".vec"):
                continue

            # Parse filename parameters
            prob_match = match_pattern.search(filename)
            if not prob_match:
                continue

            route_update = prob_match.group(1) if prob_match else "Unknown"
            if route_update != "true":
                continue

            error_prob = prob_match.group(2) if prob_match else "Unknown"
            # remove the last character if it is '.'
            if error_prob.endswith('.'):
                error_prob = error_prob[:-1]

            # === Step 1: Parse vector IDs ===
            # get the file name without .vec
            indexFile = os.path.join(input_dir, filename[:-4] + ".vci")
            # Read the file and extract scalar value line
            vector_ids = set()
            with open(indexFile, 'r') as f:
                # read line sequentially
                # find the line that starts with "vector"
                # e.g., vector 0 VEC.scheduler.vecScheduler pendingAppCount:vector ETV
                # where 0 is the vector id, and pendingAppCount:vector is the vector name
                for line in f:
                    if line.startswith("vector"):
                        parts = line.strip().split()
                        if len(parts) >= 4:
                            vec_id, vec_name = parts[1], parts[3]
                            if vec_name == target_param:
                                vector_ids.add(vec_id)

            # === Step 2: Extract vector values ===
            # e.g., "pendingAppCount:vector": 1	1006866	70.058	29
            # where 1 is the vector id, 1006866 is the event id, 70.058 is the time, and 29 is the value
            filepath = os.path.join(input_dir, filename)
            timeSet = set()
            with open(filepath, 'r') as f:
                for line in f:
                    if re.match(r'^\d+\s+\d+', line):  # numeric line
                        parts = line.strip().split()
                        vec_id = parts[0]
                        if vec_id in vector_ids:
                            v = float(parts[3]) # value
                            timeSet.add(v)
            
            # === Step 3: writing rows to CSV ===
            timeList = sorted(timeSet)
            for t in timeList:
                row = [error_prob, t]
                csv_writer.writerow(row)


def collect_node_recovery_time():
    output_csv = os.path.join(OUTPUT_DIR, f"node_recovery_time.csv")
    target_param = "globalSchedulerReady:vector"

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["error_prob", "recovery_time"])

        # Loop through .sca files in input_dir
        input_dir = os.path.join(WORKING_DIR, "results/NodeFailure")
        for filename in os.listdir(input_dir):
            if not filename.endswith(".vec"):
                continue

            # Parse filename parameters
            prob_match = match_pattern.search(filename)
            if not prob_match:
                continue

            route_update = prob_match.group(1) if prob_match else "Unknown"
            if route_update != "true":
                continue

            error_prob = prob_match.group(2) if prob_match else "Unknown"
            # remove the last character if it is '.'
            if error_prob.endswith('.'):
                error_prob = error_prob[:-1]

            # === Step 1: Parse vector IDs ===
            # get the file name without .vec
            indexFile = os.path.join(input_dir, filename[:-4] + ".vci")
            # Read the file and extract scalar value line
            vector_ids = set()
            with open(indexFile, 'r') as f:
                # read line sequentially
                # find the line that starts with "vector"
                # e.g., vector 0 VEC.scheduler.vecScheduler pendingAppCount:vector ETV
                # where 0 is the vector id, and pendingAppCount:vector is the vector name
                for line in f:
                    if line.startswith("vector"):
                        parts = line.strip().split()
                        if len(parts) >= 4:
                            vec_id, vec_name = parts[1], parts[3]
                            if vec_name == target_param:
                                vector_ids.add(vec_id)

            # === Step 2: Extract vector values ===
            # e.g., "pendingAppCount:vector": 1	1006866	70.058	29
            # where 1 is the vector id, 1006866 is the event id, 70.058 is the time, and 29 is the value
            filepath = os.path.join(input_dir, filename)
            timeSet = set()
            with open(filepath, 'r') as f:
                for line in f:
                    if re.match(r'^\d+\s+\d+', line):  # numeric line
                        parts = line.strip().split()
                        vec_id = parts[0]
                        if vec_id in vector_ids:
                            v = float(parts[3]) # value
                            timeSet.add(v)
            
            # === Step 3: writing rows to CSV ===
            timeList = sorted(timeSet)
            for t in timeList:
                row = [error_prob, t]
                csv_writer.writerow(row)


if __name__ == "__main__":
    print("Collecting link failure data...")
    collect_link_failure()
    print("Collecting node failure data...")
    collect_node_failure()
    print("Collecting link recovery time data...")
    collect_link_recovery_time()
    print("Collecting node recovery time data...")
    collect_node_recovery_time()
