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
SCHEME_ORDER = ["DistIS", "FastIS", "SARound", "IDAssign", "Game"]
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



def draw_measured_utility():
    # read txt file from folder results, format: intervals scheme expected_utility
    # csv file format: "algorithm", "interval", "utility"
    analyziz = {"scheme": [], X_NAME: [], "utility": []}

    dist_file_name = os.path.join(WORKING_DIR, "EXP1/improved_utility_mean.csv")
    with open(dist_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline() # algorithm,mapScale,appCount,utility
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            # only take Scheme DistIS from this file
            if line[0] != "DistIS" or int(line[2]) != 3:
                continue

            # get the scheme
            analyziz["scheme"].append(SCHEME_MAP[line[0]])
            # get the modulation mode
            analyziz[X_NAME].append(SCALE_MAP[line[1]])
            # get the expected utility
            analyziz["utility"].append(float(line[3]))

    able_file_name = os.path.join(WORKING_DIR, "EXP2/improved_utility_mean.csv")
    with open(able_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline() # algorithm,mapScale,appCount,utility
            if not line:
                break
            # split the line by comma
            line = line.split(",")
            # only take appCount 3 from this file
            if int(line[2]) != 3:
                continue

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


if __name__ == "__main__":
    draw_measured_utility()

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
