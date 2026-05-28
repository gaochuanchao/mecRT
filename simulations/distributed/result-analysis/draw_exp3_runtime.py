#!/usr/bin/env python
# !/usr/bin/python3
# -*- coding: utf-8 -*-
# @Time    : 7/4/2024 4:33 PM
# @Author  : Gao Chuanchao
# @Email   : jerrygao53@gmail.com
# @File    : drawing_result.py

import os
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.ticker import FormatStrFormatter
from matplotlib import rc
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib as mpl
import scienceplots
import sys


# === Configurable Parameters ===
# configure path 
WORKING_DIR = os.path.dirname(os.path.abspath(__file__))

# === Plotting Parameters ===
# === Plotting Parameters ===
LEGEND_NAME = "Map Scale"
SCALE_MAP = {
    "2": "MAP2",
    "3": "MAP3",
    "4": "MAP4"
}
MAP_ORDER = ["MAP2", "MAP3", "MAP4"]
X_ORDER = ["3", "5", "7", "N"]
X_MAP = {
    "3": "3",
    "5": "5",
    "7": "7",
    "0": "N"
}
COLOR_PALETTE  = "pastel"
Y_MAX = 15000


def draw_exe_time_avg_tokenTime():
    # read txt file from folder results, format: intervals scheme expected_utility
    analyziz = {"mapScale": [], "esLimit": [], "schemeTime": [], "tokenTime": []}

    file_name1 = os.path.join(WORKING_DIR, "EXP1/distributed_app_count_summary.csv") 
    # mapScale,appCount,time,algorithm,pendingAppCount:vector,schemeUtility:vector,schedulingTime:vector,measured_utility,tokenTransferTimeMax,tokenTransferTimeAvg
    #    3,      3,       0,   DistIS,       1014.0,               721.488,              0.010097,            546.992,         0.009564,             0.00776526
    with open(file_name1, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            # only consider app count 3
            if (int(line[1]) != 3):
                continue

            analyziz["mapScale"].append(SCALE_MAP[line[0]])
            analyziz["esLimit"].append("5")
            analyziz["schemeTime"].append(float(line[6])*1000)   # schedulingTime
            analyziz["tokenTime"].append(float(line[9])*1000)   # tokenTransferTimeAvg

    file_name2 = os.path.join(WORKING_DIR, "EXP3/distributed_app_count_summary.csv")
    # mapScale,capLimit,time,algorithm,pendingAppCount:vector,schemeUtility:vector,schedulingTime:vector,measured_utility,tokenTransferTimeMax,tokenTransferTimeAvg
    #   2,        3,      0,   DistIS,      462.0,                  401.063,            0.005424,           353.9067,           0.0053,             0.00400222
    with open(file_name2, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            analyziz["mapScale"].append(SCALE_MAP[line[0]])
            analyziz["esLimit"].append(X_MAP[line[1]])
            analyziz["schemeTime"].append(float(line[6])*1000)   # schedulingTime
            analyziz["tokenTime"].append(float(line[9])*1000)   # tokenTransferTimeAvg

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    print("=== Average Scheme Time ===")
    print(df.groupby("mapScale")["schemeTime"].mean())
    print("=== Average Token Transfer Time ===")
    print(df.groupby("mapScale")["tokenTime"].mean())

    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    # Compute mean per interval if needed
    df_avg = df.groupby(["mapScale", "esLimit"], as_index=False)[["schemeTime", "tokenTime"]].mean()
    # update schemeTime by reducing the tokenTime in df_avg
    df_avg["schemeTime"] = df_avg["schemeTime"] - df_avg["tokenTime"]

    # Set up
    fig, ax = plt.subplots(figsize=(8, 4))
    bar_width = 0.28     # adjust based on the number of schemes and the gap between bars
    x = range(len(df_avg["esLimit"].unique()))
    # schemes = df_avg["scheme"].unique()
    # sort df_avg, for each scheme, by X_NAME order
    df_avg["esLimit"] = pd.Categorical(df_avg["esLimit"], categories=X_ORDER, ordered=True)
    df_avg = df_avg.sort_values([ "mapScale", "esLimit"])

    # use one color for each scheme, and use unfilled and filled bars for overhead and network loss
    dense_colors = ["#4387D0", "#FF9800", "#41B45C", "#E13D37", "#8156E6", "#AC7A4D"]
    light_colors = ["#B6D0ED", "#FFD08A", "#9EDCAC", "#F2ADAB", "#D7CAF7", "#D7BEA7"]
    for i, scale in enumerate(MAP_ORDER):
        sub = df_avg[df_avg["mapScale"] == scale]
        x_pos = [xx + i * bar_width for xx in x]
        ax.bar(
            x_pos, sub["tokenTime"], width=bar_width,
            label=f"{scale}-T", 
            facecolor=light_colors[i],
            hatch="////",   # fill style: horizontal lines
            edgecolor="black",       # colored outline
            linewidth=0.5,
        )
        ax.bar(
            x_pos, sub["schemeTime"], width=bar_width,
            bottom=sub["tokenTime"],
            label=f"{scale}-P", 
            color=dense_colors[i], 
            edgecolor="black", 
            linewidth=0.5,
        )

    ax.margins(x=0.02)
    ax.set_xticks([xx + bar_width * (len(MAP_ORDER) - 1) / 2 for xx in x])
    ax.set_xticklabels(df_avg["esLimit"].unique())
    # plt.tight_layout()
    ax.set_xlabel("Accessible ES Limit", fontsize=10, fontweight='bold')
    ax.set_ylabel("$\mathtt{DistIS}$ Execution Time (ms)", fontsize=9, fontweight='bold', labelpad=0.5)
    ax.set_ylim(0, 15)
    plt.yticks(np.arange(0, 14, 2))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(fontsize=8, loc="upper left", ncol=3, columnspacing=0.2, handlelength=1, handletextpad=0.2, borderpad=0.2)
    # plt.tight_layout() # using tight layout to reduce the margin
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.12,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()



def draw_exe_time_max_tokenTime():
    # read txt file from folder results, format: intervals scheme expected_utility
    analyziz = {"mapScale": [], "esLimit": [], "schemeTime": [], "tokenTime": []}

    file_name1 = os.path.join(WORKING_DIR, "EXP1/distributed_app_count_summary.csv") 
    # mapScale,appCount,time,algorithm,pendingAppCount:vector,schemeUtility:vector,schedulingTime:vector,measured_utility,tokenTransferTimeMax,tokenTransferTimeAvg
    #    3,      3,       0,   DistIS,       1014.0,               721.488,              0.010097,            546.992,         0.009564,             0.00776526
    with open(file_name1, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            # only consider app count 3
            if (int(line[1]) != 3):
                continue

            analyziz["mapScale"].append(SCALE_MAP[line[0]])
            analyziz["esLimit"].append("5")
            analyziz["schemeTime"].append(float(line[6])*1000)   # schedulingTime
            analyziz["tokenTime"].append(float(line[9])*1000)   # tokenTransferTimeAvg

    file_name2 = os.path.join(WORKING_DIR, "EXP3/distributed_app_count_summary.csv")
    # mapScale,capLimit,time,algorithm,pendingAppCount:vector,schemeUtility:vector,schedulingTime:vector,measured_utility,tokenTransferTimeMax,tokenTransferTimeAvg
    #   2,        3,      0,   DistIS,      462.0,                  401.063,            0.005424,           353.9067,           0.0053,             0.00400222
    with open(file_name2, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            analyziz["mapScale"].append(SCALE_MAP[line[0]])
            analyziz["esLimit"].append(X_MAP[line[1]])
            analyziz["schemeTime"].append(float(line[6])*1000)   # schedulingTime
            analyziz["tokenTime"].append(float(line[9])*1000)   # tokenTransferTimeAvg

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    print("=== Average Scheme Time ===")
    print(df.groupby("mapScale")["schemeTime"].mean())
    print("=== Average Token Transfer Time ===")
    print(df.groupby("mapScale")["tokenTime"].mean())

    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    # Compute mean per interval if needed
    # determine the row with max tokenTime for each (mapScale, esLimit)
    # so `schemeTime` is taken from the same sample that produced the max `tokenTime`.
    idx = df.groupby(["mapScale", "esLimit"])['tokenTime'].idxmax()
    df_max = df.loc[idx, ["mapScale", "esLimit", "schemeTime", "tokenTime"]].reset_index(drop=True)
    # update schemeTime by reducing the tokenTime in df_max (schemeTime - tokenTime)
    df_max["schemeTime"] = df_max["schemeTime"] - df_max["tokenTime"]

    # Set up
    fig, ax = plt.subplots(figsize=(8, 4))
    bar_width = 0.28     # adjust based on the number of schemes and the gap between bars
    x = range(len(df_max["esLimit"].unique()))
    # schemes = df_avg["scheme"].unique()
    # sort df_avg, for each scheme, by X_NAME order
    df_max["esLimit"] = pd.Categorical(df_max["esLimit"], categories=X_ORDER, ordered=True)
    df_max = df_max.sort_values(["mapScale", "esLimit"])

    # use one color for each scheme, and use unfilled and filled bars for overhead and network loss
    dense_colors = ["#4387D0", "#FF9800", "#41B45C", "#E13D37", "#8156E6", "#AC7A4D"]
    light_colors = ["#B6D0ED", "#FFD08A", "#9EDCAC", "#F2ADAB", "#D7CAF7", "#D7BEA7"]
    for i, scale in enumerate(MAP_ORDER):
        sub = df_max[df_max["mapScale"] == scale]
        x_pos = [xx + i * bar_width for xx in x]
        ax.bar(
            x_pos, sub["tokenTime"], width=bar_width,
            label=f"{scale}-T", 
            facecolor=light_colors[i],
            hatch="////",   # fill style: horizontal lines
            edgecolor="black",       # colored outline
            linewidth=0.5,
        )
        ax.bar(
            x_pos, sub["schemeTime"], width=bar_width,
            bottom=sub["tokenTime"],
            label=f"{scale}-P", 
            color=dense_colors[i], 
            edgecolor="black", 
            linewidth=0.5,
        )

    ax.margins(x=0.02)
    ax.set_xticks([xx + bar_width * (len(MAP_ORDER) - 1) / 2 for xx in x])
    ax.set_xticklabels(df_max["esLimit"].unique())
    # plt.tight_layout()
    ax.set_xlabel("Accessible ES Limit", fontsize=10, fontweight='bold')
    ax.set_ylabel("$\mathtt{DistIS}$ Execution Time (ms)", fontsize=9, fontweight='bold', labelpad=0.5)
    ax.set_ylim(0, 15)
    plt.yticks(np.arange(0, 14, 2))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(fontsize=8, loc="upper left", ncol=3, columnspacing=0.2, handlelength=1, handletextpad=0.2, borderpad=0.2)
    # plt.tight_layout() # using tight layout to reduce the margin
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.12,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()



if __name__ == "__main__":
    draw_exe_time_avg_tokenTime()
    draw_exe_time_max_tokenTime()

    # if len(sys.argv) != 2:
    #     print("Usage: python3 draw_app_count_normal.py [5a|5b]")
    #     sys.exit(1)

    # arg = sys.argv[1]

    # if arg == "5a":
    #     draw_measured_utility()
    # elif arg == "5b":
    #     draw_exe_time()
    # else:
    #     print(f"Unknown argument: {arg}")
    #     print("Valid options: 5a, 5b")
    #     sys.exit(1)
