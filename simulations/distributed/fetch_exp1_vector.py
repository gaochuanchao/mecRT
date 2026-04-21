#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# @Time    : 7/4/2024 4:33 PM
# @Author  : Gao Chuanchao
# @Email   : gaoc0008@e.ntu.edu.sg
# @File    : fetch_centralized_app_count.py
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
INPUT_DIR = os.path.join(WORKING_DIR, "results/EXP1")
OUTPUT_DIR = os.path.join(WORKING_DIR, "result-analysis/EXP1")
# Ensure output directory exists
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Regex patterns to extract fields from filename
# file name example:  Greedy-mapScale-2-appCount-3.sca, Greedy-mapScale-2-appCount-3.vec
MATCH_PATTERN = re.compile(r'^([^-]+)-mapScale-([0-9]+)-appCount-([0-9]+)')
CENTRALIZED_ALG = ["GameTheory", "FastIS", "SARound", "Iterative", "IDAssign"]
INTERVAL = 10 # seconds, to average measured utility over this interval
ST_OFFSET = 0.05 # to align the utility time with scheduling time


def centralized_summary_app_count():
    output_csv = os.path.join(OUTPUT_DIR, f"centralized_app_count_summary.csv")

    params_list = ["pendingAppCount:vector", "schemeUtility:vector", "schedulingTime:vector"]

    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["mapScale", "appCount", "time", "algorithm"] + params_list + ["measured_utility"])

        # Loop through .vec files in INPUT_DIR
        for filename in os.listdir(INPUT_DIR):
            if not filename.endswith(".vec"):
                continue

            # Parse filename parameters
            scheAll_match = MATCH_PATTERN.search(filename)
            if not scheAll_match:
                continue

            algorithm = scheAll_match.group(1) if scheAll_match else "Unknown"
            mapScale = scheAll_match.group(2) if scheAll_match else "Unknown"
            appCount = scheAll_match.group(3) if scheAll_match else "Unknown"
            if algorithm not in CENTRALIZED_ALG:
                continue

            print(f"\tProcessing file: {filename}")
            # === Step 1: Parse vector IDs ===
            # Read the file and extract scalar value line
            filepath = os.path.join(INPUT_DIR, filename)
            vector_id2params = {}  # name -> id
            set_id2measured = set()  # id -> measured value
            with open(filepath, 'r') as f:
                # read line sequentially
                # find the line that starts with "vector"
                # e.g., vector 8 DistMEC.gnb[1].scheduler globalSchedulerReady:vector ETV
                #       8 -> vector id, globalSchedulerReady:vector -> vector name
                #                   8	    4907	0.003	0.003
                #                   vector_id, event_id, time, value
                for line in f:
                    if line.startswith("vector"):
                        parts = line.strip().split()
                        if len(parts) >= 4:
                            vec_id, vec_name = parts[1], parts[3]
                            for param in params_list:
                                if vec_name == param:
                                    vector_id2params[vec_id] = param
                            
                            if vec_name == "utility:vector":    # this is collected from server side
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
                            t = int(t)
                            v = float(parts[3])
                            param = vector_id2params[vec_id]
                            data[param][t] = v
                            timeSet.add(t)
                        if vec_id in set_id2measured:
                            t = float(parts[2])
                            t = int(t - ST_OFFSET)  # align with scheduling time
                            v = float(parts[3])
                            value = measured_data.get(t, 0)
                            value += v
                            measured_data[t] = value

            # === Step 3: writing rows to CSV ===
            timeList = sorted(timeSet)
            for t in timeList:
                row = [mapScale, appCount, t, algorithm]
                for param in params_list:
                    row.append(data[param].get(t, 0))
                
                row.append(measured_data.get(t, 0)/INTERVAL)  # average over interval
                csv_writer.writerow(row)

    print(f"\t Centralized App count summary extraction complete.\n\t saved to: {output_csv}")


def extract_pending_app_count():
    input_csv = os.path.join(OUTPUT_DIR, f"centralized_app_count_summary.csv")
    output_csv = os.path.join(OUTPUT_DIR, f"pending_app_count_summary.csv")
    df = pd.read_csv(input_csv)
    # using FastIS to extract pending app count
    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["mapScale", "appCount", "time", "pendingAppCount"])

        for index, row in df.iterrows():
            if row["algorithm"] == "FastIS":
                csv_writer.writerow([row["mapScale"], row["appCount"], row["time"], int(row["pendingAppCount:vector"])])

    print(f"\t pending app count extraction complete.\n\t saved to: {output_csv}")


def distributed_summary_app_count():
    output_csv = os.path.join(OUTPUT_DIR, f"distributed_app_count_summary.csv")
    # read pending_app_count_summary.csv to get the pending app count at each time point
    pending_df = pd.read_csv(os.path.join(OUTPUT_DIR, f"pending_app_count_summary.csv"))
    pendingAppCountDict = {}
    for index, row in pending_df.iterrows():
        key = (int(row["mapScale"]), int(row["appCount"]), int(row["time"]))
        pendingAppCountDict[key] = row["pendingAppCount"]

    params_list = ["pendingAppCount:vector", "schemeUtility:vector", "schedulingTime:vector", "measured_utility", "tokenTransferTime"]
    vector_name_list = {"pendingAppCount:vector", "schemeUtility:vector", "schedulingTime:vector", "schemeTime:vector", "distSchemeExecTime:vector"}
    with open(output_csv, mode='w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        csv_writer.writerow(["mapScale", "appCount", "time", "algorithm"] + params_list)

        # Loop through .vec files in INPUT_DIR
        for filename in os.listdir(INPUT_DIR):
            if not filename.endswith(".vec"):
                continue

            # Parse filename parameters
            scheAll_match = MATCH_PATTERN.search(filename)
            if not scheAll_match:
                continue

            algorithm = scheAll_match.group(1) if scheAll_match else "Unknown"
            mapScale = scheAll_match.group(2) if scheAll_match else "Unknown"
            appCount = scheAll_match.group(3) if scheAll_match else "Unknown"
            if algorithm in CENTRALIZED_ALG:
                continue

            print(f"\tProcessing file: {filename}")
            # === Step 1: Parse vector IDs ===
            # Read the file and extract scalar value line
            filepath = os.path.join(INPUT_DIR, filename)
            id2params = {}  # scheduler: id -> params
            id2module = {}  # scheduler: id -> module name
            server_ids = set()  # ids for utility:vector collected from server side
            with open(filepath, 'r') as f:
                # find the line that starts with "vector"
                # e.g., vector 8 DistMEC.gnb[1].scheduler globalSchedulerReady:vector ETV
                #       8 -> vector id, globalSchedulerReady:vector -> vector name
                for line in f:
                    if line.startswith("vector"):
                        parts = line.strip().split()
                        if len(parts) >= 4:
                            vec_id, module_name, vec_name = parts[1], parts[2], parts[3]
                            if vec_name in vector_name_list:
                                id2params[vec_id] = vec_name
                                id2module[vec_id] = module_name

                            if vec_name == "utility:vector":    # this is collected from server side
                                server_ids.add(vec_id)

                    # if line starts with 0, then break
                    if line.startswith("0"):
                        break

            # === Step 2: Extract vector values ===
            #   8	    4907	0.003	0.003
            #   vector_id, event_id, time, value
            vector_data = {}
            server_data = {}
            timeSet = set()  # to collect all unique time values
            with open(filepath, 'r') as f:
                for line in f:
                    if re.match(r'^\d+\s+\d+', line):  # numeric line
                        parts = line.strip().split()
                        vec_id = parts[0]
                        if vec_id in id2params:
                            t = float(parts[2])
                            t = int(t)
                            v = float(parts[3])
                            param = id2params[vec_id]
                            module_name = id2module[vec_id]
                            # vector_data: {param: {t: {module_name: value}}}}
                            if param not in vector_data:
                                vector_data[param] = {}
                            if t not in vector_data[param]:
                                vector_data[param][t] = {}
                            
                            vector_data[param][t][module_name] = v
                            timeSet.add(t)
                        if vec_id in server_ids:
                            t = float(parts[2])
                            t = int(t - ST_OFFSET)  # align with scheduling time
                            v = float(parts[3])
                            value = server_data.get(t, 0)
                            value += v
                            server_data[t] = value

            # params_list = ["pendingAppCount:vector", "schemeUtility:vector", "schedulingTime:vector", "measured_utility", "tokenTransferTime"]
            # vector_name_list = {"pendingAppCount:vector", "schemeUtility:vector", "schedulingTime:vector", "schemeTime:vector", "distSchemeExecTime:vector"}
            data = {param: {} for param in params_list}
            for t in timeSet:
                data["pendingAppCount:vector"][t] = pendingAppCountDict.get((int(mapScale), int(appCount), int(t)), 0)
                data["schemeUtility:vector"][t] = sum(vector_data.get("schemeUtility:vector", {}).get(t, {}).values())
                data["schedulingTime:vector"][t] = max(vector_data.get("schedulingTime:vector", {}).get(t, {}).values())
                data["measured_utility"][t] = server_data.get(t, 0)/INTERVAL  # average over interval
                # tokenTransferTime = schemeTime - distSchemeExecTime
                # compute the per-module token transfer time and then get the max among all modules
                schemeTime = vector_data.get("schemeTime:vector", {}).get(t, {})
                distSchemeExecTime = vector_data.get("distSchemeExecTime:vector", {}).get(t, {})
                tokenTransferTime = {}
                for module_name in schemeTime.keys():
                    st = schemeTime.get(module_name, 0)
                    dset = distSchemeExecTime.get(module_name, 0)
                    tokenTransferTime[module_name] = st - dset
                data["tokenTransferTime"][t] = max(tokenTransferTime.values()) if tokenTransferTime else 0

            # === Step 3: writing rows to CSV ===
            timeList = sorted(timeSet)
            for t in timeList:
                row = [mapScale, appCount, t, algorithm]
                for param in params_list:
                    row.append(data[param].get(t, 0))
                csv_writer.writerow(row)

    print(f"\t DistributedApp count summary extraction complete.\n\t saved to: {output_csv}")


if __name__ == "__main__":
    # centralized_summary_app_count()
    # extract_pending_app_count()
    distributed_summary_app_count()
    print(">>> All tasks completed successfully.")
