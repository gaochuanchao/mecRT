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
X_NAME = "network_quality"
X_LABEL = "Network Quality Level"
NS_MAP = {"MAX_CQI": "HIGH", "MEDIAN_CQI": "MEDIUM", "MIN_CQI": "LOW"}
NS_ORDER = ["HIGH", "MEDIUM", "LOW"]
COLOR_PALETTE  = "pastel"

# ===== file names =====
# expected_energy_sum.csv
# offload_energy_sum.csv
# actual_energy_sum.csv

# granted_job_mean.csv
# scheduling_time_mean.csv
# offload_count_sum.csv


def draw_expected_energy(sche_all, count_exe_time):
    # read txt file from folder results, format: intervals scheme expected_energy
    # csv file format: "algorithm", "pilot", "energy"
    analyziz = {X_NAME: [], "scheme": [], "energy": []}

    file_name = os.path.join(FOLDER, "scheAll_" + sche_all + "_countExeTime_" + count_exe_time + "_expected_energy_sum.csv")
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
            analyziz["scheme"].append(SCHEME_MAP[line[0]])
            # get the modulation mode
            analyziz[X_NAME].append(NS_MAP[line[1]])
            # get the expected energy
            energy = float(line[2]) / (900 * 1000)  # convert to J/s
            analyziz["energy"].append(energy)
    
    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average energy for each scheme of all intervals
    print(df.groupby("scheme")["energy"].mean())

    # normalize the energy with the average energy of SARound
    saround_energy = df[df["scheme"] == "SARound"]["energy"].mean()
    df["normalized_energy"] = df.apply(
        lambda row: row["energy"] / saround_energy, axis=1
    )
    # print(df.groupby("scheme")["normalized_energy"].mean())

    # compute the difference with scheme SARound and normalize with the average energy of SARound
    for scheme in SCHEME_ORDER:
        if scheme != "SARound":
            scheme_mean = df[df['scheme'] == scheme]['normalized_energy'].mean()
            print(f"{scheme}: {(1 - scheme_mean) * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="energy", order=NS_ORDER, hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, gap=0.1)
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 0:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=7, color='black', xytext=(0, -5),
                        textcoords='offset points')
    ax.set_xlabel(X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Predicted Energy Saving (J/s)", fontsize=9, fontweight='bold')
    # ax.set_ylim(0, 170)
    # plt.yticks(np.arange(0, 170, 20))
    ax.set_ylim(0, 240)
    plt.yticks(np.arange(0, 240, 30))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=8, fontsize=8, loc="upper right", ncol=5, columnspacing=0.4, handletextpad=0.2, borderpad=0.2)
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


def draw_actual_energy(sche_all, count_exe_time):
    # the actual saved energy is the difference between the actual energy and the offload energy
    analyziz = {X_NAME: [], "scheme": [], "energy": []}
    energy_file = os.path.join(FOLDER, "scheAll_" + sche_all + "_countExeTime_" + count_exe_time + "_actual_energy_sum.csv")
    offload_file = os.path.join(FOLDER, "scheAll_" + sche_all + "_countExeTime_" + count_exe_time + "_offload_energy_sum.csv")

    # read the energy file
    # csv file format: "algorithm", "pilot", "energy"
    energy_value = dict()
    with open(energy_file, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by space
            line = line.split(",")
            # get the scheme
            scheme = SCHEME_MAP[line[0]]
            # get the modulation
            modulation = NS_MAP[line[1]]
            # get the energy
            value = float(line[2])

            energy = energy_value.get((modulation, scheme), 0.0)
            energy_value[(modulation, scheme)] = energy + value

    # print energy_value
    # for key in energy_value.keys():
    #     print(key, energy_value[key])

    # read the offload file
    offload_value = dict()
    with open(offload_file, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by space
            line = line.split(",")
            # get the scheme
            scheme = SCHEME_MAP[line[0]]
            # get the modulation
            modulation = NS_MAP[line[1]]
            # get the energy
            offload_value[(modulation, scheme)] = float(line[2])

    # calculate the actual energy
    for key in energy_value.keys():
        analyziz[X_NAME].append(key[0])
        analyziz["scheme"].append(key[1])
        result = energy_value[key] - offload_value[key]
        result = result / (900 * 1000)  # convert to J/s
        analyziz["energy"].append(result)

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average energy for each scheme of all intervals
    print(df.groupby("scheme")["energy"].mean())

    # normalize the energy with the average energy of SARound
    saround_energy = df[df["scheme"] == "SARound"]["energy"].mean()
    df["normalized_energy"] = df.apply(
        lambda row: row["energy"] / saround_energy, axis=1
    )
    # print(df.groupby("scheme")["normalized_energy"].mean())

    # compute the difference with scheme SARound and normalize with the average energy of SARound
    for scheme in SCHEME_ORDER:
        if scheme != "SARound":
            scheme_mean = df[df['scheme'] == scheme]['normalized_energy'].mean()
            print(f"{scheme}: {(1 - scheme_mean) * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="energy", order=NS_ORDER, hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, gap=0.1)
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 0:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=7, color='black', xytext=(0, -5),
                        textcoords='offset points')
    ax.set_xlabel(X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Measured Energy Saving (J/s)", fontsize=9, fontweight='bold')
    # ax.set_ylim(0, 170)
    # plt.yticks(np.arange(0, 170, 20))
    ax.set_ylim(0, 240)
    plt.yticks(np.arange(0, 240, 30))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=8, fontsize=8, loc="upper right", ncol=5, columnspacing=0.4, handletextpad=0.2, borderpad=0.2)
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


def draw_offload_count(sche_all, count_exe_time):
    # read the csv file from folder results/interval, format: intervals scheme count
    analyziz = {X_NAME: [], "scheme": [], "count": []}
    offload_file = os.path.join(FOLDER, "scheAll_" + sche_all + "_countExeTime_" + count_exe_time + "_offload_count_sum.csv")

    # read the offload file
    offload_count = dict()
    with open(offload_file, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by space
            line = line.split(",")
            # get the scheme
            scheme = SCHEME_MAP[line[0]]
            # get the modulation
            modulation = NS_MAP[line[1]]
            # get the count
            value = float(line[2]) / 900    # convert to per second

            count = offload_count.get((modulation, scheme), 0.0)
            offload_count[(modulation, scheme)] = count + value

    # calculate the average count per second
    for key in offload_count.keys():
        analyziz[X_NAME].append(key[0])
        analyziz["scheme"].append(key[1])
        analyziz["count"].append(offload_count[key])

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average count for each scheme of all intervals
    print(df.groupby("scheme")["count"].mean())

    # normalize the energy with the average energy of SARound
    saround_energy = df[df["scheme"] == "SARound"]["count"].mean()
    df["normalized_count"] = df.apply(
        lambda row: row["count"] / saround_energy, axis=1
    )
    # compute the difference with scheme SARound and normalize with the average energy of SARound
    for scheme in SCHEME_ORDER:
        if scheme != "SARound":
            scheme_mean = df[df['scheme'] == scheme]['normalized_count'].mean()
            print(f"{scheme}: {(1 - scheme_mean) * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="count", order=NS_ORDER, hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, gap=0.1)
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 0:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=7, color='black', xytext=(0, -5),
                        textcoords='offset points')
    ax.set_xlabel(X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Offloaded Job per Second", fontsize=9, fontweight='bold')
    ax.set_ylim(0, 650)
    plt.yticks(np.arange(0, 650, 100))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=9, fontsize=9, loc="upper right", ncol=3, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
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


if __name__ == '__main__':
    scheAll = "true"    # scheAll = "true" or "false"
    print(f"=========== scheAll: {scheAll} ===========")
    print(">>> Drawing expected energy...")
    draw_expected_energy(scheAll, "true")
    print("\n>>> Drawing measured energy...")
    draw_actual_energy(scheAll, "true")

    print("\nDone!")
