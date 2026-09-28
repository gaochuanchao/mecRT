#!/usr/bin/env python
# !/usr/bin/python3
# -*- coding: utf-8 -*-
# @Time    : 7/4/2024 4:33 PM
# @Author  : Gao Chuanchao
# @Email   : jerrygao53@gmail.com
# @File    : drawing_result.py

import os
from turtle import distance
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
LEGEND_NAME = "Sorting Criteria"
CRITERIA_MAP = {
    "id.vec": "ID",
    "distance.vec": "Distance",
    "random.vec": "Random",
}
CRITERIA_ORDER = ["ID", "Distance", "Random"]
X_NAME = "mapScale"
X_LABEL = "MEC Map Scale"
SCALE_MAP = {
    "2": "MAP2",
    "3": "MAP3",
    "4": "MAP4"
}
X_ORDER = ["MAP2", "MAP3", "MAP4"]
COLOR_PALETTE  = "pastel"
Y_MAX = 15000


def draw_exe_time_avg_tokenTime():
    # read txt file from folder results, format: intervals scheme expected_utility
    analyziz = {"mapScale": [], "criteria": [], "schemeTime": []}

    file_name = os.path.join(WORKING_DIR, "EXP4/distributed_app_count_summary.csv") 
    # mapScale,ueSortingScheme,time,algorithm,pendingAppCount:vector,schemeUtility:vector,schedulingTime:vector,measured_utility,tokenTransferTimeMax,tokenTransferTimeAvg,ESexecTimeAvg
    # 2,distance.vec,0,DistIS,462.0,411.6641666666669,0.008808,348.43792999999914,0.008487,0.006926249999999999,0.008766075000000002
    with open(file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            analyziz["mapScale"].append(SCALE_MAP[line[0]])
            analyziz["criteria"].append(CRITERIA_MAP[line[1]])
            analyziz["schemeTime"].append(float(line[6])*1000)   # schedulingTime

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    print("=== Average Scheme Time ===")
    print(df.groupby("mapScale")["schemeTime"].mean())

    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    # Compute mean per interval if needed
    df_avg = df.groupby(["mapScale", "criteria"], as_index=False)[["schemeTime"]].mean()

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="schemeTime", order=X_ORDER, hue="criteria", 
                     hue_order=CRITERIA_ORDER, data=df, palette=COLOR_PALETTE, width=0.93, gap=0.02, errorbar="pi")

    # ax.margins(x=0.02)
    # ax.set_xticks([xx + bar_width * (len(CRITERIA_ORDER) - 1) / 2 for xx in x])
    # ax.set_xticklabels(df_avg["esLimit"].unique())
    # plt.tight_layout()
    ax.set_xlabel("Map Scale", fontsize=10, fontweight='bold')
    ax.set_ylabel("$\mathtt{DistIS}$ Completion Time (ms)", fontsize=9, fontweight='bold', labelpad=0.5)
    ax.set_ylim(0, 16)
    plt.yticks(np.arange(0, 14, 2))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=8, fontsize=8, loc="upper left", ncol=3, columnspacing=0.2, handlelength=1, handletextpad=0.2, borderpad=0.2)
    # plt.tight_layout() # using tight layout to reduce the margin
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.179,
        left=0.138,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


if __name__ == "__main__":
    draw_exe_time_avg_tokenTime()

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
