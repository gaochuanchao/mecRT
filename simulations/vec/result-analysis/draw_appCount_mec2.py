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
FOLDER = os.path.join(WORKING_DIR, "mec2")

# === Plotting Parameters ===
LEGEND_NAME = "Algorithm"
SCHEME_MAP = {
    "FwdQuickLR": "QuickLR",
    "FwdGraphMatch": "GraphMatch",
    "FwdGreedy": "Greedy",
    "FwdGameTheory": "GameTheory"
}
SCHEME_ORDER = ["QuickLR", "GraphMatch", "Greedy", "GameTheory"]
X_NAME = "count"
RANGE_ORDER = ["40-80", "80-120", "120-160", "160-200"]
RANGE_THRESHOLD = [80, 120, 160]
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
            if count <= RANGE_THRESHOLD[0]:
                analyziz[X_NAME].append(RANGE_ORDER[0])
            elif count <= RANGE_THRESHOLD[1]:
                analyziz[X_NAME].append(RANGE_ORDER[1])
            elif count <= RANGE_THRESHOLD[2]:
                analyziz[X_NAME].append(RANGE_ORDER[2])
            else:
                analyziz[X_NAME].append(RANGE_ORDER[3])
            # get the energy
            analyziz["energy"].append(float(line[3])/(10*1000))  # convert to J/s

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average energy for each scheme of all intervals
    # print(df.groupby("scheme")["energy"].mean())
    print(df.groupby("scheme")["energy"].mean())

    # compute the difference with scheme QuickLR and normalize with the average energy of QuickLR
    quicklr_energy = df[df["scheme"] == "QuickLR"]["energy"].mean()
    for scheme in SCHEME_ORDER:
        if scheme != "QuickLR":
            scheme_mean = df[df['scheme'] == scheme]['energy'].mean()
            print(f"{scheme}: {(quicklr_energy - scheme_mean) / quicklr_energy * 100:.2f}%")


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
    ax.set_ylabel("Predicted Energy (J/s)", fontsize=10, fontweight='bold')
    ax.set_ylim(0, 350)
    plt.yticks(np.arange(0, 350, 50))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=9, fontsize=9, loc="upper left", ncol=2, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.105,
        right=0.99,
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
            if count <= RANGE_THRESHOLD[0]:
                analyziz[X_NAME].append(RANGE_ORDER[0])
            elif count <= RANGE_THRESHOLD[1]:
                analyziz[X_NAME].append(RANGE_ORDER[1])
            elif count <= RANGE_THRESHOLD[2]:
                analyziz[X_NAME].append(RANGE_ORDER[2])
            else:
                analyziz[X_NAME].append(RANGE_ORDER[3])
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
    ax.set_ylabel("Scheduling Time (s)", fontsize=10, fontweight='bold')
    ax.set_yscale('log')
    ymin, ymax = ax.get_ylim()
    ax.set_ylim(ymin, ymax * 5)   # 20% extra space
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=9, fontsize=9, loc="upper left", ncol=4, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.14,
        left=0.115,
        right=0.99,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


if __name__ == '__main__':
    print("=========== AppCount Test Results ===========")
    print(">>> Drawing expected energy...")
    draw_expected_energy()
    print(">>> Drawing scheduling time...")
    draw_scheduling_time()

    print("\nDone!")




