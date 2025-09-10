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
X_NAME = "factor"
X_LABEL = "Fairness Factor"
COLOR_PALETTE  = "pastel"
FACTOR = ["0.17", "0.25", "0.5"]

# ===== file names =====
# expected_energy_sum.csv
# offload_energy_sum.csv
# actual_energy_sum.csv

# granted_job_mean.csv
# scheduling_time_mean.csv
# offload_count_sum.csv


def draw_expected_energy(channel_quality, sche_all):
    # read txt file from folder results, format: intervals scheme expected_energy
    # csv file format: "algorithm", "pilot", "energy"
    analyziz = {X_NAME: [], "scheme": [], "energy": []}

    for factor_value in FACTOR:
        file_name = os.path.join(FOLDER, "scheAll_" + sche_all + "_factor_" + factor_value + "_expected_energy_sum.csv")
        with open(file_name, 'r') as f:
            # discard the first line
            f.readline()
            while True:
                line = f.readline()
                if not line:
                    break
                # split the line by comma
                line = line.split(",")
                quality = line[1]
                if quality != channel_quality:
                    continue
                # get the scheme
                analyziz["scheme"].append(SCHEME_MAP[line[0]])
                # get the modulation mode
                analyziz[X_NAME].append(factor_value)
                # get the expected energy
                energy = float(line[2]) / (900 * 1000)  # convert to J/s
                analyziz["energy"].append(energy)
    
    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average energy for each scheme of all intervals
    print(df.groupby("scheme")["energy"].mean())

    # normalize the energy with the average energy of FwdQuickLR
    quicklr_energy = df[df["scheme"] == "QuickLR"]["energy"].mean()
    df["normalized_energy"] = df.apply(
        lambda row: row["energy"] / quicklr_energy, axis=1
    )
    # print(df.groupby("scheme")["normalized_energy"].mean())

    # compute the difference with scheme QuickLR and normalize with the average energy of QuickLR
    for scheme in SCHEME_ORDER:
        if scheme != "QuickLR":
            scheme_mean = df[df['scheme'] == scheme]['normalized_energy'].mean()
            print(f"{scheme}: {(1 - scheme_mean) * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="energy", order=FACTOR, hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, gap=0.1)
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 0:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=7, color='black', xytext=(0, -5),
                        textcoords='offset points')
    ax.set_xlabel(X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Predicted Energy Saving (J/s)", fontsize=10, fontweight='bold')
    ax.set_ylim(0, 350)
    plt.yticks(np.arange(0, 350, 50))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=9, fontsize=9, loc="upper right", ncol=4, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.155,
        left=0.105,
        right=0.99,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


def draw_actual_energy(channel_quality, sche_all):
    # the actual saved energy is the difference between the actual energy and the offload energy
    analyziz = {X_NAME: [], "scheme": [], "energy": []}

    energy_value = dict()
    offload_value = dict()
    for factor_value in FACTOR:
        # read the energy file, csv file format: "algorithm", "pilot", "energy"
        energy_file = os.path.join(FOLDER, "scheAll_" + sche_all + "_factor_" + factor_value + "_actual_energy_sum.csv")
        with open(energy_file, 'r') as f:
            # discard the first line
            f.readline()
            while True:
                line = f.readline()
                if not line:
                    break
                # split the line by space
                line = line.split(",")
                quality = line[1]
                if quality != channel_quality:
                    continue

                # get the scheme
                scheme = SCHEME_MAP[line[0]]
                # get the energy
                value = float(line[2])
                energy = energy_value.get((factor_value, scheme), 0.0)
                energy_value[(factor_value, scheme)] = energy + value

        # read the offload file, csv file format: "algorithm", "pilot", "energy"
        offload_file = os.path.join(FOLDER, "scheAll_" + sche_all + "_factor_" + factor_value + "_offload_energy_sum.csv")
        with open(offload_file, 'r') as f:
            # discard the first line
            f.readline()
            while True:
                line = f.readline()
                if not line:
                    break
                # split the line by space
                line = line.split(",")
                quality = line[1]
                if quality != channel_quality:
                    continue

                # get the scheme
                scheme = SCHEME_MAP[line[0]]
                # get the energy
                offload_value[(factor_value, scheme)] = float(line[2])

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

    # normalize the energy with the average energy of FwdQuickLR
    quicklr_energy = df[df["scheme"] == "QuickLR"]["energy"].mean()
    df["normalized_energy"] = df.apply(
        lambda row: row["energy"] / quicklr_energy, axis=1
    )
    # print(df.groupby("scheme")["normalized_energy"].mean())

    # compute the difference with scheme QuickLR and normalize with the average energy of QuickLR
    for scheme in SCHEME_ORDER:
        if scheme != "QuickLR":
            scheme_mean = df[df['scheme'] == scheme]['normalized_energy'].mean()
            print(f"{scheme}: {(1 - scheme_mean) * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="energy", order=FACTOR, hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, gap=0.1)
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 0:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=7, color='black', xytext=(0, -5),
                        textcoords='offset points')
    ax.set_xlabel(X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Measured Energy Saving (J/s)", fontsize=10, fontweight='bold')
    ax.set_ylim(0, 350)
    plt.yticks(np.arange(0, 350, 50))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=9, fontsize=9, loc="upper right", ncol=4, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.155,
        left=0.105,
        right=0.99,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


if __name__ == '__main__':
    channelQuality = "MIN_CQI"  # "MAX_CQI" or "MIN_CQI"
    scheAll = "false"  # scheAll = "true" or "false"
    
    print(f"=========== channelQuality: {channelQuality}, scheAll: {scheAll} ===========")
    print(">>> Drawing expected energy...")
    draw_expected_energy(channelQuality, scheAll)
    print(">>> Drawing actual energy...")
    draw_actual_energy(channelQuality, scheAll)

    print("\nDone!")
