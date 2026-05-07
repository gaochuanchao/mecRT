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
from matplotlib.ticker import FuncFormatter
from matplotlib import rc
import numpy as np
import pandas as pd
import seaborn as sns
import seaborn.objects as so
import matplotlib as mpl
import scienceplots
import sys


# === Configurable Parameters ===
# configure path 
WORKING_DIR = os.path.dirname(os.path.abspath(__file__))

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
X_NAME = "mapScale"
X_LABEL = "MEC Map Scale"
SCALE_MAP = {
    "2": "MAP2",
    "3": "MAP3",
    "4": "MAP4"
}
X_ORDER = ["MAP2", "MAP3", "MAP4"]
COLOR_PALETTE  = "pastel"
Y_SCALE = 100


def draw_expected_utility():
    # read txt file from folder results, format: intervals scheme expected_utility
    # csv file format: "algorithm", "interval", "utility"
    analyziz = {"scheme": [], X_NAME: [], "utility": []}

    file_name = os.path.join(WORKING_DIR, "EXP1/expected_utility_mean.csv")
    with open(file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline() # algorithm,mapScale,appCount,utility
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            # get the scheme
            analyziz["scheme"].append(SCHEME_MAP[line[0]])
            # get the modulation mode
            analyziz[X_NAME].append(SCALE_MAP[line[1]])
            # get the expected utility
            analyziz["utility"].append(float(line[3]))
    
    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average utility for each scheme of all intervals
    print(f"=== Average Expected Utility ===")
    print(df.groupby("scheme")["utility"].mean())

    # normalize the utility with the average utility of DistIS
    distis_utility = df[df["scheme"] == "DistIS"]["utility"].mean()
    df["normalized_utility"] = df.apply(
        lambda row: row["utility"] / distis_utility, axis=1
    )
    # print(df.groupby("scheme")["normalized_utility"].mean())

    # compute the difference with scheme DistIS and normalize with the average utility of DistIS
    for scheme in SCHEME_ORDER:
        if scheme != "DistIS":
            scheme_mean = df[df['scheme'] == scheme]['normalized_utility'].mean()
            print(f"{scheme}: {(1 - scheme_mean) * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="utility", order=X_ORDER, hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, width=0.93, gap=0.02, errorbar=None)
    # add value on the top of the bar, only for the bar with height > 0
    for i, p in enumerate(ax.patches):
        if p.get_height() > 0:
            offset = -8 if i % 2 == 0 else -14
            ax.annotate(
                f'{p.get_height() / Y_SCALE:.1f}',
                (p.get_x() + p.get_width() / 2., p.get_height()),
                ha='center',
                va='bottom',
                fontsize=6,
                color='black',
                xytext=(0, offset),
                textcoords='offset points'
            )
    ax.set_xlabel(X_LABEL, fontsize=10, fontweight='bold')
    ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: f'{y/Y_SCALE:.0f}'))
    ax.set_ylabel("Predicted Utility (x100)", fontsize=9, fontweight='bold',labelpad=0.5)
    ax.set_ylim(0, 1250)
    plt.yticks(np.arange(0, 1250, 200))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=8, fontsize=8, loc="upper left", ncol=2, columnspacing=0.4, handletextpad=0.2, borderpad=0.2)
    # plt.tight_layout() # using tight layout to reduce the margin
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.112,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


def draw_measured_utility():
    # read txt file from folder results, format: intervals scheme expected_utility
    # csv file format: "algorithm", "interval", "utility"
    analyziz = {"scheme": [], X_NAME: [], "utility": []}

    file_name = os.path.join(WORKING_DIR, "EXP1/improved_utility_mean.csv")
    with open(file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline() # algorithm,mapScale,appCount,utility
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            # get the scheme
            analyziz["scheme"].append(SCHEME_MAP[line[0]])
            # get the modulation mode
            analyziz[X_NAME].append(SCALE_MAP[line[1]])
            # get the expected utility
            analyziz["utility"].append(float(line[3]))
    
    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average utility for each scheme of all intervals
    print(f"=== Average Measured Utility ===")
    print(df.groupby("scheme")["utility"].mean())

    # normalize the utility with the average utility of DistIS
    distis_utility = df[df["scheme"] == "DistIS"]["utility"].mean()
    df["normalized_utility"] = df.apply(
        lambda row: row["utility"] / distis_utility, axis=1
    )
    # print(df.groupby("scheme")["normalized_utility"].mean())

    # compute the difference with scheme DistIS and normalize with the average utility of DistIS
    for scheme in SCHEME_ORDER:
        if scheme != "DistIS":
            scheme_mean = df[df['scheme'] == scheme]['normalized_utility'].mean()
            print(f"{scheme}: {(1 - scheme_mean) * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="utility", order=X_ORDER, hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, width=0.93, gap=0.02, errorbar=None)
    # add value on the top of the bar, only for the bar with height > 0
    for i, p in enumerate(ax.patches):
        if p.get_height() > 0:
            offset = -8 if i % 2 == 0 else -14
            ax.annotate(
                f'{p.get_height() / Y_SCALE:.1f}',
                (p.get_x() + p.get_width() / 2., p.get_height()),
                ha='center',
                va='bottom',
                fontsize=6,
                color='black',
                xytext=(0, offset),
                textcoords='offset points'
            )
    ax.set_xlabel(X_LABEL, fontsize=10, fontweight='bold')
    ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: f'{y/Y_SCALE:.0f}'))
    ax.set_ylabel("Measured Utility (x100)", fontsize=9, fontweight='bold',labelpad=0.5)
    ax.set_ylim(0, 1250)
    plt.yticks(np.arange(0, 1250, 200))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=8, fontsize=8, loc="upper left", ncol=2, columnspacing=0.4, handletextpad=0.2, borderpad=0.2)
    # plt.tight_layout() # using tight layout to reduce the margin
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.112,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


def draw_offload_percentage():
    # read txt file from folder results, format: intervals scheme expected_utility
    # csv file format: "algorithm", "interval", "utility"
    expected_job_count = dict()
    file_name1 = os.path.join(WORKING_DIR, "EXP1/expected_job_count_mean.csv")
    with open(file_name1, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma, format: algorithm,mapScale,appCount,job_count
            line = line.split(",")      
            scheme = SCHEME_MAP[line[0]]
            if scheme not in expected_job_count:
                expected_job_count[scheme] = {}
            expected_job_count[scheme][SCALE_MAP[line[1]]] = float(line[3])

    job_count_since_grant = dict()
    actual_job_count = dict()
    file_name2 = os.path.join(WORKING_DIR, "EXP1/improved_utility_mean.csv")
    with open(file_name2, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # algorithm,mapScale,appCount,utility:mean,meetDlPkt:mean,jobGeneratedSinceGranted:mean
            # FastIS,       2,      2,      343.827,    2613.5777,          3068.9955555555557
            line = line.split(",")
            scheme = SCHEME_MAP[line[0]]
            if scheme not in job_count_since_grant:
                job_count_since_grant[scheme] = {}
                actual_job_count[scheme] = {}
            job_count_since_grant[scheme][SCALE_MAP[line[1]]] = float(line[5])
            actual_job_count[scheme][SCALE_MAP[line[1]]] = float(line[4])

    loss = {"scheme": [], X_NAME: [], "overhead_loss": [], "network_loss": []}
    for scheme in expected_job_count.keys():
        for mapScale in expected_job_count[scheme].keys():
            expected_count = expected_job_count[scheme][mapScale]
            actual_count = actual_job_count[scheme][mapScale]
            granted_count = job_count_since_grant[scheme][mapScale]
            overhead_loss = (expected_count - granted_count) / expected_count
            network_loss = (granted_count - actual_count) / expected_count
            loss["scheme"].append(scheme)
            loss[X_NAME].append(mapScale)
            loss["overhead_loss"].append(overhead_loss)
            loss["network_loss"].append(network_loss)
    
    # create a dataframe
    df = pd.DataFrame.from_dict(loss)

    # print the average overhead loss and network loss for each scheme of all intervals
    print(f"=== Average Overhead Loss ===")
    print(df.groupby("scheme")["overhead_loss"].mean())
    print(f"=== Average Network Loss ===")
    print(df.groupby("scheme")["network_loss"].mean())

    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    # Compute mean per interval if needed
    df_avg = df.groupby(["scheme", X_NAME], as_index=False)[["overhead_loss", "network_loss"]].mean()

    # Set up
    fig, ax = plt.subplots(figsize=(8, 4))
    bar_width = 0.15     # adjust based on the number of schemes and the gap between bars
    x = range(len(df_avg[X_NAME].unique()))
    # schemes = df_avg["scheme"].unique()
    # sort df_avg, for each scheme, by X_NAME order
    df_avg[X_NAME] = pd.Categorical(df_avg[X_NAME], categories=X_ORDER, ordered=True)
    df_avg = df_avg.sort_values([ "scheme", X_NAME])

    # use one color for each scheme, and use unfilled and filled bars for overhead and network loss
    dense_colors = ["#4387D0", "#FF9800", "#41B45C", "#E13D37", "#8156E6", "#AC7A4D"]
    light_colors = ["#B6D0ED", "#FFD08A", "#9EDCAC", "#F2ADAB", "#D7CAF7", "#D7BEA7"]
    for i, scheme in enumerate(SCHEME_ORDER):
        sub = df_avg[df_avg["scheme"] == scheme]
        x_pos = [xx + i * bar_width for xx in x]
        ax.bar(
            x_pos, sub["overhead_loss"], width=bar_width,
            label=f"{scheme}-O", 
            color=dense_colors[i], 
            edgecolor="black", 
            linewidth=0.5,
        )
        ax.bar(
            x_pos, sub["network_loss"], width=bar_width,
            bottom=sub["overhead_loss"], label=f"{scheme}-N", 
            facecolor=light_colors[i],
            hatch="////",   # fill style: horizontal lines
            edgecolor="black",       # colored outline
            linewidth=0.5,
        )

    ax.margins(x=0.02)
    ax.set_xticks([xx + bar_width * (len(SCHEME_ORDER) - 1) / 2 for xx in x])
    ax.set_xticklabels(df_avg[X_NAME].unique())
    # plt.tight_layout()
    ax.set_xlabel(X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Fallback Ratio", fontsize=9, fontweight='bold', labelpad=0.5)
    ax.set_ylim(0, 0.5)
    plt.yticks(np.arange(0, 0.5, 0.1))
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
    # draw_expected_utility()
    # draw_measured_utility()
    draw_offload_percentage()

    # if len(sys.argv) != 2:
    #     print("Usage: python3 draw_performance_normal.py [4a|4b|4c]")
    #     sys.exit(1)

    # arg = sys.argv[1]

    # if arg == "4a":
    #     draw_expected_utility()
    # elif arg == "4b":
    #     draw_improved_utility()
    # elif arg == "4c":
    #     draw_offload_percentage()
    # else:
    #     print(f"Unknown argument: {arg}")
    #     print("Valid options: 4a, 4b, 4c")
    #     sys.exit(1)
