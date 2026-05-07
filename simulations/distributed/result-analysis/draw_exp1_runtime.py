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
LEGEND_NAME = "Algorithm"
SCHEME_MAP = {
    "FastIS": "FastIS",
    "IDAssign": "IDAssign",
    "Iterative": "Iterative",
    "DistIS": "DistIS",
    "GameTheory": "Game",
    "SARound": "SARound",
}
SCHEME_ORDER = ["DistIS", "FastIS", "SARound", "IDAssign", "Iterative", "Game"]
X_NAME = "count"
COLOR_PALETTE  = "pastel"
Y_MAX = 15000


# ===== file names =====
# centralized_app_count_summary.csv
# mapScale,appCount,time,algorithm,pendingAppCount:vector,schemeUtility:vector,schedulingTime:vector,measured_utility
#   4,          3,   0,   SARound,      1593.0,                 1305.320,           0.59526,            698.3802

# distributed_app_count_summary.csv
# mapScale,appCount,time,algorithm,pendingAppCount:vector,schemeUtility:vector,schedulingTime:vector,measured_utility,tokenTransferTimeMax,tokenTransferTimeAvg
#    3,      3,       0,   DistIS,       1014.0,               721.488,              0.010097,            546.992,         0.009564,             0.00776526


def draw_exe_time_map2():
    # read txt file from folder results, format: intervals scheme expected_utility
    analyziz = {"scheme": [], X_NAME: [], "time": []}

    centralized_file_name = os.path.join(WORKING_DIR, "EXP1/centralized_app_count_summary.csv")
    with open(centralized_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            # only consider map size 2
            if (int(line[0]) != 2):
                continue

            # get the scheme
            analyziz["scheme"].append(SCHEME_MAP[line[3]])
            # get the count
            count = float(line[4])
            if count < 400:
                analyziz[X_NAME].append("300-400")
            elif count < 500:
                analyziz[X_NAME].append("400-500")
            elif count < 600:
                analyziz[X_NAME].append("500-600")
            else:
                analyziz[X_NAME].append("600-700")
            # get the time
            analyziz["time"].append(float(line[6])*1000)

    distributed_file_name = os.path.join(WORKING_DIR, "EXP1/distributed_app_count_summary.csv")
    with open(distributed_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            # only consider map size 2
            if (int(line[0]) != 2):
                continue

            # get the scheme
            analyziz["scheme"].append(SCHEME_MAP[line[3]])
            # get the count
            count = float(line[4])
            if count < 400:
                analyziz[X_NAME].append("300-400")
            elif count < 500:
                analyziz[X_NAME].append("400-500")
            elif count < 600:
                analyziz[X_NAME].append("500-600")
            else:
                analyziz[X_NAME].append("600-700")
            # get the time
            analyziz["time"].append(float(line[6])*1000)

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average time for each scheme of all intervals
    # print(df.groupby("scheme")["time"].mean())
    print("=== Execution Time for Different Request Counts ===")
    print(df.groupby("scheme")["time"].mean())

    # compute the difference with scheme DistIS and normalize with the average time of DistIS
    fastsa_time = df[df["scheme"] == "DistIS"]["time"].mean()
    for scheme in SCHEME_ORDER:
        if scheme != "DistIS":
            scheme_mean = df[df['scheme'] == scheme]['time'].mean()
            print(f"{scheme}: {(fastsa_time - scheme_mean) / fastsa_time * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="time", order=["300-400", "400-500", "500-600", "600-700"], hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, width=0.93, gap=0.02, errorbar=None)
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 20:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=6, color='black', xytext=(0, -6),
                        textcoords='offset points')
        elif p.get_height() > 0:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=6, color='black', xytext=(0, 3),
                        textcoords='offset points')
    ax.set_xlabel("MAP2 - Pending Request Count", fontsize=10, fontweight='bold')
    ax.set_ylabel("Execution Time (ms)", fontsize=9, fontweight='bold', labelpad=0.5)
    # ax.set_ylim(0, 3000)
    # plt.yticks(np.arange(0, 260, 30))
    ax.set_yscale('log')
    ymin, ymax = ax.get_ylim()
    ax.set_ylim(ymin, Y_MAX)   # 20% extra space
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=8, fontsize=8, loc="upper left", ncol=3, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.13,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


def draw_exe_time_map3():
    # read txt file from folder results, format: intervals scheme expected_utility
    analyziz = {"scheme": [], X_NAME: [], "time": []}

    centralized_file_name = os.path.join(WORKING_DIR, "EXP1/centralized_app_count_summary.csv")
    with open(centralized_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            # only consider map size 3
            if (int(line[0]) != 3):
                continue

            # get the scheme
            analyziz["scheme"].append(SCHEME_MAP[line[3]])
            # get the count
            count = float(line[4])
            if count < 850:
                analyziz[X_NAME].append("650-850")
            elif count < 1000:
                analyziz[X_NAME].append("850-1000")
            elif count < 1150:
                analyziz[X_NAME].append("1000-1150")
            else:
                analyziz[X_NAME].append("1150-1300")
            # get the time
            analyziz["time"].append(float(line[6])*1000)

    distributed_file_name = os.path.join(WORKING_DIR, "EXP1/distributed_app_count_summary.csv")
    with open(distributed_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            # only consider map size 3
            if (int(line[0]) != 3):
                continue

            # get the scheme
            analyziz["scheme"].append(SCHEME_MAP[line[3]])
            # get the count
            count = float(line[4])
            if count < 850:
                analyziz[X_NAME].append("650-850")
            elif count < 1000:
                analyziz[X_NAME].append("850-1000")
            elif count < 1150:
                analyziz[X_NAME].append("1000-1150")
            else:
                analyziz[X_NAME].append("1150-1300")
            # get the time
            analyziz["time"].append(float(line[6])*1000)

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average time for each scheme of all intervals
    # print(df.groupby("scheme")["time"].mean())
    print("=== Execution Time for Different Request Counts ===")
    print(df.groupby("scheme")["time"].mean())

    # compute the difference with scheme DistIS and normalize with the average time of DistIS
    fastsa_time = df[df["scheme"] == "DistIS"]["time"].mean()
    for scheme in SCHEME_ORDER:
        if scheme != "DistIS":
            scheme_mean = df[df['scheme'] == scheme]['time'].mean()
            print(f"{scheme}: {(fastsa_time - scheme_mean) / fastsa_time * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="time", order=["650-850", "850-1000", "1000-1150", "1150-1300"], hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, width=0.93, gap=0.02, errorbar=None)
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 20:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=6, color='black', xytext=(0, -6),
                        textcoords='offset points')
        elif p.get_height() > 0:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=6, color='black', xytext=(0, 3),
                        textcoords='offset points')
    ax.set_xlabel("MAP3 - Pending Request Count", fontsize=10, fontweight='bold')
    ax.set_ylabel("Execution Time (ms)", fontsize=9, fontweight='bold', labelpad=0.5)
    # ax.set_ylim(0, 3000)
    # plt.yticks(np.arange(0, 260, 30))
    ax.set_yscale('log')
    ymin, ymax = ax.get_ylim()
    ax.set_ylim(ymin, Y_MAX)   # upper bound with fixed value
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=8, fontsize=8, loc="upper left", ncol=3, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.13,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


def draw_exe_time_map4():
    # read txt file from folder results, format: intervals scheme expected_utility
    analyziz = {"scheme": [], X_NAME: [], "time": []}

    centralized_file_name = os.path.join(WORKING_DIR, "EXP1/centralized_app_count_summary.csv")
    with open(centralized_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            # only consider map size 4
            if (int(line[0]) != 4):
                continue

            # get the scheme
            analyziz["scheme"].append(SCHEME_MAP[line[3]])
            # get the count
            count = float(line[4])
            if count < 1300:
                analyziz[X_NAME].append("1050-1300")
            elif count < 1500:
                analyziz[X_NAME].append("1300-1500")
            elif count < 1700:
                analyziz[X_NAME].append("1500-1700")
            else:
                analyziz[X_NAME].append("1700-1950")
            # get the time
            analyziz["time"].append(float(line[6])*1000)

    distributed_file_name = os.path.join(WORKING_DIR, "EXP1/distributed_app_count_summary.csv")
    with open(distributed_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            # only consider map size 4
            if (int(line[0]) != 4):
                continue

            # get the scheme
            analyziz["scheme"].append(SCHEME_MAP[line[3]])
            # get the count
            count = float(line[4])
            if count < 1300:
                analyziz[X_NAME].append("1050-1300")
            elif count < 1500:
                analyziz[X_NAME].append("1300-1500")
            elif count < 1700:
                analyziz[X_NAME].append("1500-1700")
            else:
                analyziz[X_NAME].append("1700-1950")
            # get the time
            analyziz["time"].append(float(line[6])*1000)

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average time for each scheme of all intervals
    # print(df.groupby("scheme")["time"].mean())
    print("=== Execution Time for Different Request Counts ===")
    print(df.groupby("scheme")["time"].mean())

    # compute the difference with scheme DistIS and normalize with the average time of DistIS
    fastsa_time = df[df["scheme"] == "DistIS"]["time"].mean()
    for scheme in SCHEME_ORDER:
        if scheme != "DistIS":
            scheme_mean = df[df['scheme'] == scheme]['time'].mean()
            print(f"{scheme}: {(fastsa_time - scheme_mean) / fastsa_time * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="time", order=["1050-1300", "1300-1500", "1500-1700", "1700-1950"], hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, width=0.93, gap=0.02, errorbar=None)
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 20:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=6, color='black', xytext=(0, -6),
                        textcoords='offset points')
        elif p.get_height() > 0:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=6, color='black', xytext=(0, 3),
                        textcoords='offset points')
    ax.set_xlabel("MAP4 - Pending Request Count", fontsize=10, fontweight='bold')
    ax.set_ylabel("Execution Time (ms)", fontsize=9, fontweight='bold', labelpad=0.5)
    # ax.set_ylim(0, 3000)
    # plt.yticks(np.arange(0, 260, 30))
    ax.set_yscale('log')
    ymin, ymax = ax.get_ylim()
    ax.set_ylim(ymin, Y_MAX)   # upper bound with fixed value
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=8, fontsize=8, loc="upper left", ncol=3, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.13,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


if __name__ == "__main__":
    draw_exe_time_map2()
    # draw_exe_time_map3()
    # draw_exe_time_map4()

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
