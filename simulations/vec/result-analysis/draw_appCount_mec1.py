#!/usr/bin/env python
# !/usr/bin/python3
# -*- coding: utf-8 -*-
# @Time    : 7/4/2024 4:33 PM
# @Author  : Gao Chuanchao
# @Email   : jerrygao53@gmail.com
# @File    : drawing_result.py

import os
import matplotlib.pyplot as plt
from matplotlib.ticker import FormatStrFormatter
from matplotlib import rc
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib as mpl
import scienceplots

# === Configurable Parameters ===
# configure path 
WORKING_DIR = os.path.dirname(os.path.abspath(__file__))
FOLDER = os.path.join(WORKING_DIR, "mec1")

# === Plotting Parameters ===
LEGEND_NAME = "Algorithm"
SCHEME_MAP = {
    "SARound": "SARound",
    "FastLR": "FastLR",
    "Greedy": "Greedy",
    "GameTheory": "Game",
    "Iterative": "Iterative"
}
SCHEME_ORDER = ["SARound", "FastLR", "Greedy", "Game", "Iterative"]
X_NAME = "count"
RANGE_ORDER = ["20-50", "50-80", "80-110", "110-150"]
X_LABEL = "Request Count Range"
COLOR_PALETTE  = "pastel"

# ===== file names =====
# app_count_summary.csv
# e.g., time   algorithm      pendingAppCount:vector savedEnergy:vector schemeTime:vector
#       10     FwdGameTheory           72.0            759172.84015873        0.033406


def draw_expected_energy():
    # read txt file from folder results, format: intervals scheme expected_energy
    analyziz = {"scheme": [], X_NAME: [], "energy": []}

    file_name = os.path.join(FOLDER, "app_count_summary.csv")
    with open(file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            # get the scheme
            analyziz["scheme"].append(SCHEME_MAP[line[1]])
            # get the count
            count = int(float(line[2]))
            if count <= 50:
                analyziz[X_NAME].append("20-50")
            elif count <= 80:
                analyziz[X_NAME].append("50-80")
            elif count <= 110:
                analyziz[X_NAME].append("80-110")
            else:
                analyziz[X_NAME].append("110-150")
            # get the energy
            analyziz["energy"].append(float(line[3])/(10*1000))  # convert to J/s

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average energy for each scheme of all intervals
    # print(df.groupby("scheme")["energy"].mean())
    print(df.groupby("scheme")["energy"].mean())

    # compute the difference with scheme SARound and normalize with the average energy of SARound
    saround_energy = df[df["scheme"] == "SARound"]["energy"].mean()
    for scheme in SCHEME_ORDER:
        if scheme != "SARound":
            scheme_mean = df[df['scheme'] == scheme]['energy'].mean()
            print(f"{scheme}: {(saround_energy - scheme_mean) / saround_energy * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="energy", order=RANGE_ORDER, hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, gap=0.1, errorbar="sd")
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 70:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=7, color='black', xytext=(0, -15),
                        textcoords='offset points')
        elif p.get_height() > 30:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=7, color='black', xytext=(0, -12),
                        textcoords='offset points')
        elif p.get_height() > 10:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=7, color='black', xytext=(0, -9),
                        textcoords='offset points')
    ax.set_xlabel(X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Predicted Energy Saving (J/s)", fontsize=9, fontweight='bold')
    ax.set_ylim(0, 260)
    plt.yticks(np.arange(0, 260, 30))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=9, fontsize=9, loc="upper left", ncol=2, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.103,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


def draw_scheduling_time():
    # app_count_summary.csv
    # e.g., time   algorithm      pendingAppCount:vector savedEnergy:vector schemeTime:vector
    #       10     FwdGameTheory           72.0            759172.84015873        0.033406
    analyziz = {"scheme": [], X_NAME: [], "time": []}

    file_name = os.path.join(FOLDER, "app_count_summary.csv")
    with open(file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            # get the scheme
            analyziz["scheme"].append(SCHEME_MAP[line[1]])
            # get the count
            count = int(float(line[2]))
            if count <= 50:
                analyziz[X_NAME].append("20-50")
            elif count <= 80:
                analyziz[X_NAME].append("50-80")
            elif count <= 110:
                analyziz[X_NAME].append("80-110")
            else:
                analyziz[X_NAME].append("110-150")
            # get the time
            analyziz["time"].append(float(line[4]))  # in seconds
    
    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average time for each scheme of all intervals
    print(df.groupby("scheme")["time"].mean())

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="time", order=RANGE_ORDER, hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, gap=0.1)
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 1:
            ax.annotate(f'{p.get_height():.3f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=6, color='black', xytext=(0, -12),
                        textcoords='offset points')
        elif p.get_height() > 0.3:
            ax.annotate(f'{p.get_height():.3f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=6, color='black', xytext=(0, -7),
                        textcoords='offset points')
        elif p.get_height() > 0:
            ax.annotate(f'{p.get_height():.3f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=6, color='black', xytext=(0, -5),
                        textcoords='offset points')
    ax.set_xlabel(X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Scheduling Time (s)", fontsize=9, fontweight='bold')
    # define two intervals for y-axis, first interval is 0-2.0, second interval is 5-8   
    ax.set_yscale('log')
    ymin, ymax = ax.get_ylim()
    ax.set_ylim(ymin, ymax * 5)   # 20% extra space
    # ax.set_ylim(0, 0.02)
    # plt.yticks(np.arange(0, 0.02, 0.005))
    # ax.set_ylim(-3, 3)
    # plt.yticks(np.arange(-3, 3, 1))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=9, fontsize=9, loc="upper left", ncol=5, columnspacing=0.4, handletextpad=0.2, borderpad=0.2)
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.113,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


if __name__ == '__main__':
    print("======= Drawing Results for App Count Summary =======")
    print(">>> Drawing expected energy...")
    draw_expected_energy()
    print(">>> Drawing scheduling time...")
    draw_scheduling_time()

    print("\nDone!")




