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


# === Configurable Parameters ===
# configure path 
WORKING_DIR = os.path.dirname(os.path.abspath(__file__))
FOLDER = os.path.join(WORKING_DIR, "normal")

# === Plotting Parameters ===
LEGEND_NAME = "Algorithm"
SCHEME_MAP = {
    "FastSA": "FastSA",
    "Greedy": "Greedy",
    "GameTheory": "Game",
    "GraphMatch": "Graph"
}
SCHEME_ORDER = ["FastSA", "Greedy", "Game", "Graph"]
X_NAME = "count"
X_LABEL = "Pending Request Count"
X_ORDER = ["20-65", "65-110", "110-155", "155-200"]
COLOR_PALETTE  = "pastel"


# ===== file names =====
# app_count_summary.csv
# e.g., time,   algorithm,  pendingAppCount:vector, schemeUtility:vector, schemeTime:vector
#       0.058,  GraphMatch,         28.0,             49.082857142857,        0.287885

def draw_expected_utility():
    # read txt file from folder results, format: intervals scheme expected_utility
    analyziz = {"scheme": [], X_NAME: [], "utility": []}

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
            count = float(line[2])
            if count <= 65:
                analyziz[X_NAME].append("20-65")
            elif count <= 110:
                analyziz[X_NAME].append("65-110")
            elif count <= 155:
                analyziz[X_NAME].append("110-155")
            else:
                analyziz[X_NAME].append("155-200")
            # get the utility
            analyziz["utility"].append(float(line[3]))

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average utility for each scheme of all intervals
    # print(df.groupby("scheme")["utility"].mean())
    print("=== Obtained Utility for Different Request Counts ===")
    print(df.groupby("scheme")["utility"].mean())

    # compute the difference with scheme FastSA and normalize with the average utility of FastSA
    fastsa_utility = df[df["scheme"] == "FastSA"]["utility"].mean()
    for scheme in SCHEME_ORDER:
        if scheme != "FastSA":
            scheme_mean = df[df['scheme'] == scheme]['utility'].mean()
            print(f"{scheme}: {(fastsa_utility - scheme_mean) / fastsa_utility * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="utility", order=X_ORDER, hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, gap=0.1, errorbar=None)
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
    ax.set_ylabel("Predicted Improved Accuracy", fontsize=9, fontweight='bold')
    # ax.set_ylim(0, 260)
    # plt.yticks(np.arange(0, 260, 30))
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


def draw_measured_utility():
    # read txt file from folder results, format: intervals scheme expected_utility
    analyziz = {"scheme": [], X_NAME: [], "utility": []}

    file_name = os.path.join(FOLDER, "app_count_summary.csv")
    with open(file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # time,algorithm,pendingAppCount:vector,schemeUtility:vector,schemeTime:vector,measured_utility
            # 0.058,GraphMatch,28.0,49.064285714286,0.33472,45.49206000000013
            line = line.split(",")
            # get the scheme
            analyziz["scheme"].append(SCHEME_MAP[line[1]])
            # get the count
            count = float(line[2])
            if count <= 65:
                analyziz[X_NAME].append("20-65")
            elif count <= 110:
                analyziz[X_NAME].append("65-110")
            elif count <= 155:
                analyziz[X_NAME].append("110-155")
            else:
                analyziz[X_NAME].append("155-200")
            # get the utility
            analyziz["utility"].append(float(line[5]))

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average utility for each scheme of all intervals
    # print(df.groupby("scheme")["utility"].mean())
    print("=== Measured Utility for Different Request Counts ===")
    print(df.groupby("scheme")["utility"].mean())

    # compute the difference with scheme FastSA and normalize with the average utility of FastSA
    fastsa_utility = df[df["scheme"] == "FastSA"]["utility"].mean()
    for scheme in SCHEME_ORDER:
        if scheme != "FastSA":
            scheme_mean = df[df['scheme'] == scheme]['utility'].mean()
            print(f"{scheme}: {(fastsa_utility - scheme_mean) / fastsa_utility * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="utility", order=X_ORDER, hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, gap=0.1, errorbar=None)
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
    ax.set_ylabel("Measured Accuracy Improvement", fontsize=9, fontweight='bold')
    ax.set_ylim(0, 110)
    plt.yticks(np.arange(0, 110, 20))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=9, fontsize=9, loc="upper left", ncol=4, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
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


def draw_exe_time():
    # read txt file from folder results, format: intervals scheme expected_utility
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
            count = float(line[2])
            if count <= 65:
                analyziz[X_NAME].append("20-65")
            elif count <= 110:
                analyziz[X_NAME].append("65-110")
            elif count <= 155:
                analyziz[X_NAME].append("110-155")
            else:
                analyziz[X_NAME].append("155-200")
            # get the time
            analyziz["time"].append(float(line[4]))

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average time for each scheme of all intervals
    # print(df.groupby("scheme")["time"].mean())
    print("=== Execution Time for Different Request Counts ===")
    print(df.groupby("scheme")["time"].mean())

    # compute the difference with scheme FastSA and normalize with the average time of FastSA
    fastsa_time = df[df["scheme"] == "FastSA"]["time"].mean()
    for scheme in SCHEME_ORDER:
        if scheme != "FastSA":
            scheme_mean = df[df['scheme'] == scheme]['time'].mean()
            print(f"{scheme}: {(fastsa_time - scheme_mean) / fastsa_time * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="time", order=X_ORDER, hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, gap=0.1, errorbar=None)
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 0.01:
            ax.annotate(f'{p.get_height():.3f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=6, color='black', xytext=(0, -6),
                        textcoords='offset points')
        elif p.get_height() > 0:
            ax.annotate(f'{p.get_height():.3f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=6, color='black', xytext=(0, 3),
                        textcoords='offset points')
    ax.set_xlabel(X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Execution Time (s)", fontsize=9, fontweight='bold')
    # ax.set_ylim(0, 260)
    # plt.yticks(np.arange(0, 260, 30))
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
        bottom=0.15,
        left=0.11,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


if __name__ == "__main__":
    # draw_expected_utility()
    # draw_expected_utility()
    # draw_exe_time()
    draw_measured_utility()
