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
import seaborn.objects as so
import matplotlib as mpl
import scienceplots


# === Configurable Parameters ===
# configure path 
WORKING_DIR = os.path.dirname(os.path.abspath(__file__))
FOLDER = os.path.join(WORKING_DIR, "ablation-fwd")

# === Plotting Parameters ===
LEGEND_NAME = "Algorithm"
SCHEME_MAP = {
    "FastSA": "FastSA",
    "FastSANF": "FastSA-NF"
}
SCHEME_ORDER = ["FastSA", "FastSA-NF"]
X_NAME = "distance"
X_LABEL = "Commucation Range (m)"
X_ORDER = ["300", "450", "600"]
COLOR_PALETTE  = "pastel"


def draw_expected_utility():
    # read txt file from folder results, format: intervals scheme expected_utility
    # csv file format: "algorithm", "interval", "utility"
    analyziz = {"scheme": [], X_NAME: [], "utility": []}

    file_name = os.path.join(FOLDER, "expected_utility_mean.csv")
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
            analyziz[X_NAME].append(line[1])
            # get the expected utility
            analyziz["utility"].append(float(line[2]))
    
    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average utility for each scheme of all intervals
    print(f"=== Average Expected Utility ===")
    print(df.groupby("scheme")["utility"].mean())

    # normalize the utility with the average utility of FastSA
    fastsa_utility = df[df["scheme"] == "FastSA"]["utility"].mean()
    df["normalized_utility"] = df.apply(
        lambda row: row["utility"] / fastsa_utility, axis=1
    )
    # print(df.groupby("scheme")["normalized_utility"].mean())

    # compute the difference with scheme SARound and normalize with the average utility of SARound
    for scheme in SCHEME_ORDER:
        if scheme != "SARound":
            scheme_mean = df[df['scheme'] == scheme]['normalized_utility'].mean()
            print(f"{scheme}: {(1 - scheme_mean) * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="utility", order=X_ORDER, hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, gap=0.1, errorbar=None)
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 0:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=7, color='black', xytext=(0, -5),
                        textcoords='offset points')
    ax.set_xlabel(X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Predicted Accuracy Improvement", fontsize=9, fontweight='bold')
    ax.set_ylim(0, 230)
    plt.yticks(np.arange(0, 230, 30))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=8, fontsize=8, loc="upper left", ncol=4, columnspacing=0.4, handletextpad=0.2, borderpad=0.2)
    # plt.tight_layout() # using tight layout to reduce the margin
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.142,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


def draw_improved_utility():
    # read txt file from folder results, format: intervals scheme expected_utility
    # csv file format: "algorithm", "interval", "utility"
    analyziz = {"scheme": [], X_NAME: [], "utility": []}

    file_name = os.path.join(FOLDER, "improved_utility_mean.csv")
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
            analyziz[X_NAME].append(line[1])
            # get the expected utility
            analyziz["utility"].append(float(line[2]))
    
    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average utility for each scheme of all intervals
    print(f"=== Average Actual Utility ===")
    print(df.groupby("scheme")["utility"].mean())

    # normalize the utility with the average utility of FastSA
    fastsa_utility = df[df["scheme"] == "FastSA"]["utility"].mean()
    df["normalized_utility"] = df.apply(
        lambda row: row["utility"] / fastsa_utility, axis=1
    )
    # print(df.groupby("scheme")["normalized_utility"].mean())

    # compute the difference with scheme SARound and normalize with the average utility of SARound
    for scheme in SCHEME_ORDER:
        if scheme != "SARound":
            scheme_mean = df[df['scheme'] == scheme]['normalized_utility'].mean()
            print(f"{scheme}: {(1 - scheme_mean) * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="utility", order=X_ORDER, hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, gap=0.1, errorbar=None)
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 10:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=7, color='black', xytext=(0, -5),
                        textcoords='offset points')
        elif p.get_height() > 0:
            ax.annotate(f'{p.get_height():.1f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=6, color='black', xytext=(0, 3),
                        textcoords='offset points')
    ax.set_xlabel(X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Measured Accuracy Improvement", fontsize=9, fontweight='bold')
    ax.set_ylim(0, 230)
    plt.yticks(np.arange(0, 230, 30))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=8, fontsize=8, loc="upper right", ncol=5, columnspacing=0.4, handletextpad=0.2, borderpad=0.2)
    # plt.tight_layout() # using tight layout to reduce the margin
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.142,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


def draw_offload_count():
    # read txt file from folder results, format: intervals scheme expected_utility
    # csv file format: "algorithm", "interval", "utility"
    analyziz = {"scheme": [], X_NAME: [], "count": []}

    file_name = os.path.join(FOLDER, "improved_utility_mean.csv")
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
            analyziz[X_NAME].append(line[1])
            # get the expected utility
            analyziz["count"].append(float(line[2]))
    
    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)

    # print the average count for each scheme of all intervals
    print(f"=== Average Offload Count ===")
    print(df.groupby("scheme")["count"].mean())

    # normalize the utility with the average utility of FastSA
    fastsa_utility = df[df["scheme"] == "FastSA"]["count"].mean()
    df["normalized_count"] = df.apply(
        lambda row: row["count"] / fastsa_utility, axis=1
    )
    # print(df.groupby("scheme")["normalized_count"].mean())

    # compute the difference with scheme SARound and normalize with the average count of SARound
    for scheme in SCHEME_ORDER:
        if scheme != "SARound":
            scheme_mean = df[df['scheme'] == scheme]['normalized_count'].mean()
            print(f"{scheme}: {(1 - scheme_mean) * 100:.2f}%")

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x=X_NAME, y="count", order=X_ORDER, hue="scheme", 
                     hue_order=SCHEME_ORDER, data=df, palette=COLOR_PALETTE, gap=0.1, errorbar=None)
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 30:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=7, color='black', xytext=(0, -5),
                        textcoords='offset points')
        elif p.get_height() > 0:
            ax.annotate(f'{p.get_height():.0f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=7, color='black', xytext=(0, 3),
                        textcoords='offset points')
    ax.set_xlabel(X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Offloaded Job Count", fontsize=9, fontweight='bold')
    ax.set_ylim(0, 120)
    plt.yticks(np.arange(0, 120, 30))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=LEGEND_NAME, title_fontsize=8, fontsize=8, loc="upper right", ncol=5, columnspacing=0.4, handletextpad=0.2, borderpad=0.2)
    # plt.tight_layout() # using tight layout to reduce the margin
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.125,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


def draw_offload_percentage():
    # read txt file from folder results, format: intervals scheme expected_utility
    # csv file format: "algorithm", "interval", "utility"
    expected_job_count = dict()
    file_name1 = os.path.join(FOLDER, "expected_job_count_mean.csv")
    with open(file_name1, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            scheme = SCHEME_MAP[line[0]]
            if scheme not in expected_job_count:
                expected_job_count[scheme] = {}
            expected_job_count[scheme][line[1]] = float(line[2])

    job_count_since_grant = dict()
    actual_job_count = dict()
    file_name2 = os.path.join(FOLDER, "improved_utility_mean.csv")
    with open(file_name2, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # algorithm,    interval,   utility:mean,       meetDlPkt:mean,     jobGeneratedSinceGranted:mean
            # FastSA,           10,   82.38414399996233,  667.9855555555556,        1046.2844444444445
            line = line.split(",")
            scheme = SCHEME_MAP[line[0]]
            if scheme not in job_count_since_grant:
                job_count_since_grant[scheme] = {}
                actual_job_count[scheme] = {}
            job_count_since_grant[scheme][line[1]] = float(line[4])
            actual_job_count[scheme][line[1]] = float(line[3])

    loss = {"scheme": [], X_NAME: [], "overhead_loss": [], "network_loss": []}
    for scheme in expected_job_count.keys():
        for interval in expected_job_count[scheme].keys():
            expected_count = expected_job_count[scheme][interval]
            actual_count = actual_job_count[scheme][interval]
            granted_count = job_count_since_grant[scheme][interval]
            overhead_loss = (expected_count - granted_count) / expected_count
            network_loss = (granted_count - actual_count) / expected_count
            loss["scheme"].append(scheme)
            loss[X_NAME].append(interval)
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
    bar_width = 0.2
    x = range(len(df_avg[X_NAME].unique()))
    # schemes = df_avg["scheme"].unique()
    # sort df_avg, for each scheme, by X_NAME order
    df_avg[X_NAME] = pd.Categorical(df_avg[X_NAME], categories=X_ORDER, ordered=True)
    df_avg = df_avg.sort_values([ "scheme", X_NAME])

    # use one color for each scheme, and use unfilled and filled bars for overhead and network loss
    dense_colors = ["#2F8AEB", "#F07D30", "#2BD954", "#F23630"]
    light_colors = ["#BCD3EB", "#F0D3C0", "#ADD9B7", "#F2C4C2"]
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

    ax.set_xticks([xx + bar_width * (len(SCHEME_ORDER) - 1) / 2 for xx in x])
    ax.set_xticklabels(df_avg[X_NAME].unique())
    # plt.tight_layout()
    ax.set_xlabel(X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Fallback Ratio", fontsize=9, fontweight='bold')
    ax.set_ylim(0, 1.25)
    plt.yticks(np.arange(0, 1.1, 0.2))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(fontsize=8, loc="upper right", ncol=4, columnspacing=0.2, handlelength=1, handletextpad=0.2, borderpad=0.2)
    # plt.tight_layout() # using tight layout to reduce the margin
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.137,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


if __name__ == "__main__":
    draw_expected_utility()
    draw_improved_utility()
    # draw_offload_percentage()

