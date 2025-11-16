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
FOLDER = os.path.join(WORKING_DIR, "failure")

# === Plotting Parameters ===
LINK_LEGEND_NAME = "Link Fault Ratio"
NODE_LEGEND_NAME = "Node Fault Ratio"
MEASURED_LEGEND_NAME = "Fault Ratio"

PREDICT_HUE_ORDER = ["0.1", "0.2", "0.25"]
PREDICT_X_LABEL = "Fault Injection Time Intervals (s)"
PREDICT_X_ORDER = [f"{i}-\n{i+50}" for i in range(50, 900, 100)]
PREDICT_X_ORDER_WIDE = [f"{i}-{i+50}" for i in range(50, 900, 100)]

MEASURE_HUE_ORDER = ["0.1", "0.1-D", "0.2", "0.2-D", "0.25", "0.25-D"]
MEASURE_X_LABEL = "Fault Type"
MEASURE_X_ORDER = ["Link Fault", "Node Fault"]

INTERVAL = 50
COLOR_PALETTE  = "pastel"
# Manual mapping for colors and linestyles per fault ratio.
# Update these values if you want different colors or styles.
COLOR_MAP = {
    "0.1": "#4696EB", # blue
    "0.2": "#41D964", # red
    "0.25": "#7D49F5",   # black
}
LINESTYLE_MAP = {
    "0.1": "-",
    "0.2": "-",
    "0.25": "-",
}
MARKER_MAP = {
    "0.1": "s",
    "0.2": "o",
    "0.25": "d",
}

PROB_MAP = {
    "0": "0-D",
    "0.1": "0.1-D",
    "0.2": "0.2-D",
    "0.25": "0.25-D",
}


def draw_failure_performance_measured():
    # read txt file from folder results, format: intervals scheme expected_utility
    analyziz = {"type": [], "ratio": [], "value": []}

    link_dict_data = {key: dict() for key in MEASURE_HUE_ORDER}
    link_file_name = os.path.join(FOLDER, "link_failure_measured_data.csv")
    with open(link_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")
            
            # determine the time interval
            t = int(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            end_t = start_t + INTERVAL
            time_interval = f"({start_t},{end_t}]"

            routeUpdate = line[1]
            ratio = line[2]
            if routeUpdate == "false":
                ratio = PROB_MAP[ratio]
            util = float(line[3])
            value = link_dict_data[ratio].get(time_interval, 0) + util
            link_dict_data[ratio][time_interval] = value

    node_dict_data = {key: dict() for key in MEASURE_HUE_ORDER}
    node_file_name = os.path.join(FOLDER, "node_failure_measured_data.csv")
    with open(node_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")
            
            # determine the time interval
            t = int(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            end_t = start_t + INTERVAL
            time_interval = f"({start_t},{end_t}]"

            routeUpdate = line[1]
            ratio = line[2]
            if routeUpdate == "false":
                ratio = PROB_MAP[ratio]
            util = float(line[3])
            value = node_dict_data[ratio].get(time_interval, 0) + util
            node_dict_data[ratio][time_interval] = value

    base_dict_data = {"0": dict()}
    base_file_name = os.path.join(FOLDER, "failure_base_measured_data.csv")
    with open(base_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")
            
            # determine the time interval
            t = int(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            end_t = start_t + INTERVAL
            time_interval = f"({start_t},{end_t}]"

            routeUpdate = line[1]
            util = float(line[3])
            value = base_dict_data["0"].get(time_interval, 0) + util
            base_dict_data["0"][time_interval] = value

    # convert dict_data to analyziz
    for ratio in MEASURE_HUE_ORDER:
        for time_interval in link_dict_data[ratio].keys():
            analyziz["type"].append("Link Fault")
            analyziz["ratio"].append(ratio)
            normal_value = link_dict_data[ratio][time_interval] / base_dict_data["0"][time_interval]
            analyziz["value"].append(normal_value)
        for time_interval in node_dict_data[ratio].keys():
            analyziz["type"].append("Node Fault")
            analyziz["ratio"].append(ratio)
            normal_value = node_dict_data[ratio][time_interval] / base_dict_data["0"][time_interval]
            analyziz["value"].append(normal_value)

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)
    # print the mean value for each ratio and type
    print("=== Failure Performance Measured Summary ===")
    for failure_type in ["Link Fault", "Node Fault"]:
        for ratio in ["0.1", "0.2", "0.25", "0.1-D", "0.2-D", "0.25-D"]:
            sub = df[(df["ratio"] == ratio) & (df["type"] == failure_type)]
            if sub.empty:
                continue
            avg_value = sub["value"].mean()
            print(f"Average value for {failure_type} with ratio {ratio}: {avg_value:.3f}")


    # start drawing, box plot
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x="type", y="value", order=MEASURE_X_ORDER, hue="ratio", hue_order=MEASURE_HUE_ORDER,
                    data=df, palette="pastel", gap=0.1, errorbar=None)
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 0:
            ax.annotate(f'{p.get_height():.2f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=6, color='black', xytext=(0, -6),
                        textcoords='offset points')
    # ax = sns.barplot(x="type", y="value", order=MEASURE_X_ORDER, hue="ratio", hue_order=MEASURE_HUE_ORDER,
                    # data=df, palette="pastel", errorbar="sd", err_kws={'linewidth': 1}, capsize=0.3, gap=0.1)
    ax.set_xlabel(MEASURE_X_LABEL, fontsize=10, weight="bold")
    ax.set_ylabel("Ratio to Fault-Free (Measured)", fontsize=10, weight="bold")
    ax.set_ylim(0.2, 1.45)
    plt.yticks(np.arange(0.2, 1.1, 0.2))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=MEASURED_LEGEND_NAME, title_fontsize=9, fontsize=9, loc="upper left", ncol=3, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.142,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


def draw_failure_performance_measured_wide():
    # read txt file from folder results, format: intervals scheme expected_utility
    analyziz = {"type": [], "ratio": [], "value": []}

    link_dict_data = {key: dict() for key in ["0", "0-D"] + MEASURE_HUE_ORDER}
    link_file_name = os.path.join(FOLDER, "link_failure_measured_data.csv")
    with open(link_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")
            
            # determine the time interval
            t = int(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            end_t = start_t + INTERVAL
            time_interval = f"({start_t},{end_t}]"

            routeUpdate = line[1]
            ratio = line[2]
            if routeUpdate == "false":
                ratio = PROB_MAP[ratio]
            util = float(line[3])
            value = link_dict_data[ratio].get(time_interval, 0) + util
            link_dict_data[ratio][time_interval] = value

    node_dict_data = {key: dict() for key in ["0", "0-D"] + MEASURE_HUE_ORDER}
    node_file_name = os.path.join(FOLDER, "node_failure_measured_data.csv")
    with open(node_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")
            
            # determine the time interval
            t = int(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            end_t = start_t + INTERVAL
            time_interval = f"({start_t},{end_t}]"

            routeUpdate = line[1]
            ratio = line[2]
            if routeUpdate == "false":
                ratio = PROB_MAP[ratio]
            util = float(line[3])
            value = node_dict_data[ratio].get(time_interval, 0) + util
            node_dict_data[ratio][time_interval] = value

    base_dict_data = {"0": dict()}
    base_file_name = os.path.join(FOLDER, "failure_base_measured_data.csv")
    with open(base_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")
            
            # determine the time interval
            t = int(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            end_t = start_t + INTERVAL
            time_interval = f"({start_t},{end_t}]"

            routeUpdate = line[1]
            util = float(line[3])
            value = base_dict_data["0"].get(time_interval, 0) + util
            base_dict_data["0"][time_interval] = value

    # convert dict_data to analyziz
    for ratio in MEASURE_HUE_ORDER:
        for time_interval in link_dict_data[ratio].keys():
            analyziz["type"].append("Link Fault")
            analyziz["ratio"].append(ratio)
            normal_value = link_dict_data[ratio][time_interval] / base_dict_data["0"][time_interval]
            analyziz["value"].append(normal_value)
        for time_interval in node_dict_data[ratio].keys():
            analyziz["type"].append("Node Fault")
            analyziz["ratio"].append(ratio)
            normal_value = node_dict_data[ratio][time_interval] / base_dict_data["0"][time_interval]
            analyziz["value"].append(normal_value)

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)
    # print the mean value for each ratio and type
    print("=== Failure Performance Measured Summary ===")
    for ratio in ["0.1", "0.2", "0.25", "0.1-D", "0.2-D", "0.25-D"]:
        for failure_type in ["Link Fault", "Node Fault"]:
            sub = df[(df["ratio"] == ratio) & (df["type"] == failure_type)]
            if sub.empty:
                continue
            avg_value = sub["value"].mean()
            print(f"{failure_type} with ratio {ratio}: {avg_value:.3f}")


    # start drawing, box plot
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x="type", y="value", order=MEASURE_X_ORDER, hue="ratio", hue_order=MEASURE_HUE_ORDER,
                    data=df, palette="pastel", gap=0.1, errorbar=None)
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 0:
            ax.annotate(f'{p.get_height():.2f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=6, color='black', xytext=(0, -6),
                        textcoords='offset points')
    # ax = sns.barplot(x="type", y="value", order=MEASURE_X_ORDER, hue="ratio", hue_order=MEASURE_HUE_ORDER,
                    # data=df, palette="pastel", errorbar="sd", err_kws={'linewidth': 1}, capsize=0.3, gap=0.1)
    ax.set_xlabel(MEASURE_X_LABEL, fontsize=10, weight="bold")
    ax.set_ylabel("Ratio to Fault-Free (Measured)", fontsize=10, weight="bold")
    ax.set_ylim(0.2, 1.3)
    plt.yticks(np.arange(0.2, 1.1, 0.2))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=MEASURED_LEGEND_NAME, title_fontsize=9, fontsize=9, loc="upper left", ncol=6, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.093,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


def draw_failure_performance_predicted_wide():
    # read txt file from folder results, format: intervals scheme expected_utility
    analyziz = {"type": [], "ratio": [], "value": []}

    link_dict_data = {key: dict() for key in ["0"] + PREDICT_HUE_ORDER}
    link_file_name = os.path.join(FOLDER, "link_failure_predicted_data.csv")
    with open(link_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")
            
            routeUpdate = line[1]
            if routeUpdate == "false":
                continue

            # determine the time interval
            ratio = line[2]
            t = float(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            if (t - start_t) < 0.1 and ratio != "0":
                continue
            end_t = start_t + INTERVAL
            time_interval = f"({start_t},{end_t}]"
            util = float(line[3])
            value = link_dict_data[ratio].get(time_interval, 0) + util
            link_dict_data[ratio][time_interval] = value

    node_dict_data = {key: dict() for key in ["0"] + PREDICT_HUE_ORDER}
    node_file_name = os.path.join(FOLDER, "node_failure_predicted_data.csv")
    with open(node_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")
            
            routeUpdate = line[1]
            if routeUpdate == "false":
                continue

            # determine the time interval
            ratio = line[2]
            t = float(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            if (t - start_t) < 0.1 and ratio != "0":
                continue
            end_t = start_t + INTERVAL
            time_interval = f"({start_t},{end_t}]"
            util = float(line[3])
            value = node_dict_data[ratio].get(time_interval, 0) + util
            node_dict_data[ratio][time_interval] = value

    base_dict_data = {"0": dict()}
    base_file_name = os.path.join(FOLDER, "failure_base_predicted_data.csv")
    with open(base_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")
            
            # determine the time interval
            t = float(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            end_t = start_t + INTERVAL
            time_interval = f"({start_t},{end_t}]"
            util = float(line[3])
            value = base_dict_data["0"].get(time_interval, 0) + util
            base_dict_data["0"][time_interval] = value

    # convert dict_data to analyziz
    for ratio in PREDICT_HUE_ORDER:
        for time_interval in link_dict_data[ratio].keys():
            analyziz["type"].append("Link Fault")
            analyziz["ratio"].append(ratio)
            normal_value = link_dict_data[ratio][time_interval] / base_dict_data["0"][time_interval]
            analyziz["value"].append(normal_value)
        for time_interval in node_dict_data[ratio].keys():
            analyziz["type"].append("Node Fault")
            analyziz["ratio"].append(ratio)
            normal_value = node_dict_data[ratio][time_interval] / base_dict_data["0"][time_interval]
            analyziz["value"].append(normal_value)

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)
    # print the mean value for each ratio and type
    print("=== Fault Performance Predicted Summary ===")
    for fault_type in ["Link Fault", "Node Fault"]:
        for ratio in PREDICT_HUE_ORDER:
            sub = df[(df["ratio"] == ratio) & (df["type"] == fault_type)]
            if sub.empty:
                continue
            avg_value = sub["value"].mean()
            print(f"{fault_type} with ratio {ratio}: {avg_value:.3f}")


    # start drawing, box plot
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    ax = sns.barplot(x="type", y="value", order=MEASURE_X_ORDER, hue="ratio", hue_order=PREDICT_HUE_ORDER,
                    data=df, palette="pastel", gap=0.5, errorbar=None)
    # add value on the top of the bar, only for the bar with height > 0
    for p in ax.patches:
        if p.get_height() > 0:
            ax.annotate(f'{p.get_height():.2f}', (p.get_x() + p.get_width() / 2., p.get_height()),
                        ha='center', va='center', fontsize=6, color='black', xytext=(0, -6),
                        textcoords='offset points')
    # ax = sns.barplot(x="type", y="value", order=MEASURE_X_ORDER, hue="ratio", hue_order=MEASURE_HUE_ORDER,
                    # data=df, palette="pastel", errorbar="sd", err_kws={'linewidth': 1}, capsize=0.3, gap=0.1)
    ax.set_xlabel(MEASURE_X_LABEL, fontsize=10, weight="bold")
    ax.set_ylabel("Ratio to Fault-Free (Predicted)", fontsize=10, weight="bold")
    ax.set_ylim(0.2, 1.3)
    plt.yticks(np.arange(0.2, 1.1, 0.2))
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=9)
    ax.legend(title=MEASURED_LEGEND_NAME, title_fontsize=9, fontsize=9, loc="upper left", ncol=6, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.093,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()


def draw_node_failure_performance_measured():
    # read txt file from folder results, format: intervals scheme expected_utility
    analyziz = {"ratio": [], "time_interval": [], "utility": []}

    dict_data = {key: dict() for key in ["0"] + PREDICT_HUE_ORDER}
    file_name = os.path.join(FOLDER, "node_failure_measured_data.csv")
    with open(file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")
            
            routeUpdate = line[1]
            if routeUpdate == "false":
                continue

            # determine the time interval
            ratio = line[2]
            t = float(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            end_t = start_t + INTERVAL
            time_interval = f"{start_t}-\n{end_t}"
            util = float(line[3])
            value = dict_data[ratio].get(time_interval, 0) + util
            dict_data[ratio][time_interval] = value

    base_dict_data = {"0": dict()}
    base_file_name = os.path.join(FOLDER, "failure_base_measured_data.csv")
    with open(base_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")
            
            # determine the time interval
            t = int(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            end_t = start_t + INTERVAL
            time_interval = f"({start_t},{end_t}]"

            routeUpdate = line[1]
            util = float(line[3])
            value = base_dict_data["0"].get(time_interval, 0) + util
            base_dict_data["0"][time_interval] = value
    
    # convert dict_data to analyziz
    for ratio in PREDICT_HUE_ORDER:
        for time_interval in dict_data[ratio].keys():
            analyziz["ratio"].append(ratio)
            analyziz["time_interval"].append(time_interval)
            analyziz["utility"].append(dict_data[ratio][time_interval] / base_dict_data["0"][time_interval])

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)
    # df["time_interval"] = pd.Categorical(df["time_interval"], categories=X_ORDER, ordered=True)

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    # ax = sns.lineplot(x="time", y="utility", hue="ratio", 
    #                  hue_order=Link_ORDER, data=df, palette=COLOR_PALETTE)
    # Draw each ratio separately so we can control exact color and linestyle
    plt.figure(figsize=(6, 4))
    ax = plt.gca()
    for ratio in PREDICT_HUE_ORDER:
        sub = df[df["ratio"] == ratio]
        if sub.empty:
            continue
        sns.lineplot(data=sub, x="time_interval", y="utility", ax=ax,
                     label=ratio,
                     color=COLOR_MAP.get(ratio, None),
                     linestyle=LINESTYLE_MAP.get(ratio, "-"),
                     marker=MARKER_MAP.get(ratio, "o"),
                     markersize=6,
                     linewidth=1)
    ax.set_xlabel(PREDICT_X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Ratio to Fault-Free (Measured)", fontsize=9, fontweight='bold')
    ax.set_ylim(0.5, 1.5)
    plt.yticks(np.arange(0.6, 1.3, 0.1))
    # ax.set_xlim(0, 900)
    # plt.xticks(np.arange(75, 900, INTERVAL*2))
    # plt.yticks(np.arange(0, 260, 30))
    # ax.set_yscale('log')
    # ymin, ymax = ax.get_ylim()
    # ax.set_ylim(ymin, ymax * 5)   # 20% extra space
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=8)
    ax.legend(title=NODE_LEGEND_NAME, title_fontsize=9, fontsize=9, loc="upper left", ncol=4, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.201,
        left=0.136,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()

    # compute the average value difference for different ratios
    # only consider the time points where time % 100 == 75
    print("=== Node Failure Measured Summary ===")
    for ratio in PREDICT_HUE_ORDER:
        sub = df[df["ratio"] == ratio]
        if sub.empty:
            continue
        avg_diff = sub["utility"].mean()
        print(f"Average value for {ratio}: {avg_diff:.3f}")


def draw_link_failure_performance_predicted():
    # read txt file from folder results, format: intervals scheme expected_utility
    analyziz = {"ratio": [], "time_interval": [], "utility": []}

    dict_data = {key: dict() for key in ["0"] + PREDICT_HUE_ORDER}
    file_name = os.path.join(FOLDER, "link_failure_predicted_data.csv")
    with open(file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")
            
            routeUpdate = line[1]
            if routeUpdate == "false":
                continue

            # determine the time interval
            ratio = line[2]
            t = float(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            if (t - start_t) < 0.1 and ratio != "0":
                continue
            end_t = start_t + INTERVAL
            time_interval = f"{start_t}-\n{end_t}"
            util = float(line[3])
            value = dict_data[ratio].get(time_interval, 0) + util
            dict_data[ratio][time_interval] = value

    base_dict_data = {"0": dict()}
    base_file_name = os.path.join(FOLDER, "failure_base_predicted_data.csv")
    with open(base_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")

            # determine the time interval
            t = float(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            end_t = start_t + INTERVAL
            time_interval = f"{start_t}-\n{end_t}"
            util = float(line[3])
            value = base_dict_data["0"].get(time_interval, 0) + util
            base_dict_data["0"][time_interval] = value

    # convert dict_data to analyziz
    for ratio in PREDICT_HUE_ORDER:
        for time_interval in dict_data[ratio].keys():
            analyziz["ratio"].append(ratio)
            analyziz["time_interval"].append(time_interval)
            analyziz["utility"].append(dict_data[ratio][time_interval] / base_dict_data["0"][time_interval])

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)
    # df["time_interval"] = pd.Categorical(df["time_interval"], categories=X_ORDER, ordered=True)

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    # ax = sns.lineplot(x="time", y="utility", hue="ratio", 
    #                  hue_order=Link_ORDER, data=df, palette=COLOR_PALETTE)
    # Draw each ratio separately so we can control exact color and linestyle
    plt.figure(figsize=(6, 4))
    ax = plt.gca()
    for ratio in PREDICT_HUE_ORDER:
        sub = df[df["ratio"] == ratio]
        if sub.empty:
            continue
        sns.lineplot(data=sub, x="time_interval", y="utility", ax=ax,
                     label=ratio,
                     color=COLOR_MAP.get(ratio, None),
                     linestyle=LINESTYLE_MAP.get(ratio, "-"),
                     marker=MARKER_MAP.get(ratio, "o"),
                     markersize=6,
                     linewidth=1)
    ax.set_xlabel(PREDICT_X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Ratio to Fault-Free (Predicted)", fontsize=9, fontweight='bold')
    ax.set_ylim(0.5, 1.5)
    plt.yticks(np.arange(0.6, 1.3, 0.1))
    # ax.set_xlim(0, 900)
    # plt.xticks(np.arange(75, 900, INTERVAL*2))
    # plt.yticks(np.arange(0, 260, 30))
    # ax.set_yscale('log')
    # ymin, ymax = ax.get_ylim()
    # ax.set_ylim(ymin, ymax * 5)   # 20% extra space
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=8)
    ax.legend(title=LINK_LEGEND_NAME, title_fontsize=9, fontsize=9, loc="upper left", ncol=4, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.201,
        left=0.136,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()

    # compute the average value difference for different ratios
    # only consider the time points where time % 100 == 75
    print("=== Link Failure Predicted Summary ===")
    for ratio in PREDICT_HUE_ORDER:
        sub = df[df["ratio"] == ratio]
        if sub.empty:
            continue
        avg_diff = sub["utility"].mean()
        print(f"Average value for {ratio}: {avg_diff:.3f}")


def draw_node_failure_performance_predicted():
    # read txt file from folder results, format: intervals scheme expected_utility
    analyziz = {"ratio": [], "time_interval": [], "utility": []}

    dict_data = {key: dict() for key in ["0"] + PREDICT_HUE_ORDER}
    file_name = os.path.join(FOLDER, "node_failure_predicted_data.csv")
    with open(file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")
            
            routeUpdate = line[1]
            if routeUpdate == "false":
                continue

            # determine the time interval
            ratio = line[2]
            t = float(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            if (t - start_t) < 0.1 and ratio != "0":
                continue
            end_t = start_t + INTERVAL
            time_interval = f"{start_t}-\n{end_t}"
            util = float(line[3])
            value = dict_data[ratio].get(time_interval, 0) + util
            dict_data[ratio][time_interval] = value

    base_dict_data = {"0": dict()}
    base_file_name = os.path.join(FOLDER, "failure_base_predicted_data.csv")
    with open(base_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")

            # determine the time interval
            t = float(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            end_t = start_t + INTERVAL
            time_interval = f"{start_t}-\n{end_t}"
            util = float(line[3])
            value = base_dict_data["0"].get(time_interval, 0) + util
            base_dict_data["0"][time_interval] = value

    # convert dict_data to analyziz
    for ratio in PREDICT_HUE_ORDER:
        for time_interval in dict_data[ratio].keys():
            analyziz["ratio"].append(ratio)
            analyziz["time_interval"].append(time_interval)
            analyziz["utility"].append(dict_data[ratio][time_interval] / base_dict_data["0"][time_interval])

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)
    # df["time_interval"] = pd.Categorical(df["time_interval"], categories=X_ORDER, ordered=True)

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    # ax = sns.lineplot(x="time", y="utility", hue="ratio", 
    #                  hue_order=Link_ORDER, data=df, palette=COLOR_PALETTE)
    # Draw each ratio separately so we can control exact color and linestyle
    plt.figure(figsize=(6, 4))
    ax = plt.gca()
    for ratio in PREDICT_HUE_ORDER:
        sub = df[df["ratio"] == ratio]
        if sub.empty:
            continue
        sns.lineplot(data=sub, x="time_interval", y="utility", ax=ax,
                     label=ratio,
                     color=COLOR_MAP.get(ratio, None),
                     linestyle=LINESTYLE_MAP.get(ratio, "-"),
                     marker=MARKER_MAP.get(ratio, "o"),
                     markersize=6,
                     linewidth=1)
    ax.set_xlabel(PREDICT_X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Ratio to Fault-Free (Predicted)", fontsize=9, fontweight='bold')
    ax.set_ylim(0.5, 1.5)
    plt.yticks(np.arange(0.6, 1.3, 0.1))
    # ax.set_xlim(0, 900)
    # plt.xticks(np.arange(75, 900, INTERVAL*2))
    # plt.yticks(np.arange(0, 260, 30))
    # ax.set_yscale('log')
    # ymin, ymax = ax.get_ylim()
    # ax.set_ylim(ymin, ymax * 5)   # 20% extra space
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=8)
    ax.legend(title=NODE_LEGEND_NAME, title_fontsize=9, fontsize=9, loc="upper left", ncol=4, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.201,
        left=0.136,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()

    # compute the average value difference for different ratios
    # only consider the time points where time % 100 == 75
    print("=== Node Failure Predicted Summary ===")
    for ratio in PREDICT_HUE_ORDER:
        sub = df[df["ratio"] == ratio]
        if sub.empty:
            continue
        avg_diff = sub["utility"].mean()
        print(f"Average value for {ratio}: {avg_diff:.3f}")


def draw_node_failure_performance_predicted_wide():
    # read txt file from folder results, format: intervals scheme expected_utility
    analyziz = {"ratio": [], "time_interval": [], "utility": []}

    dict_data = {key: dict() for key in ["0"] + PREDICT_HUE_ORDER}
    file_name = os.path.join(FOLDER, "node_failure_predicted_data.csv")
    with open(file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")
            
            routeUpdate = line[1]
            if routeUpdate == "false":
                continue

            # determine the time interval
            ratio = line[2]
            t = float(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            if (t - start_t) < 0.1 and ratio != "0":
                continue
            end_t = start_t + INTERVAL
            time_interval = f"{start_t}-{end_t}"
            util = float(line[3])
            value = dict_data[ratio].get(time_interval, 0) + util
            dict_data[ratio][time_interval] = value

    base_dict_data = {"0": dict()}
    base_file_name = os.path.join(FOLDER, "failure_base_predicted_data.csv")
    with open(base_file_name, 'r') as f:
        # discard the first line
        f.readline()
        while True:
            line = f.readline()
            if not line:
                break
            # split the line by comma
            # e.g., time,routeUpdate,errorProb,utility:vector,meetDlPkt:vector
            #       0,      false,      0.4,    46.50699999999995,  327.0
            line = line.split(",")

            # determine the time interval
            t = float(line[0])
            isOdd = int(t / INTERVAL) % 2 == 1
            if not isOdd:
                continue
            start_t = (int(t / INTERVAL)) * INTERVAL
            end_t = start_t + INTERVAL
            time_interval = f"{start_t}-\n{end_t}"
            util = float(line[3])
            value = base_dict_data["0"].get(time_interval, 0) + util
            base_dict_data["0"][time_interval] = value

    # convert dict_data to analyziz
    for ratio in PREDICT_HUE_ORDER:
        for time_interval in dict_data[ratio].keys():
            analyziz["ratio"].append(ratio)
            analyziz["time_interval"].append(time_interval)
            analyziz["utility"].append(dict_data[ratio][time_interval] / base_dict_data["0"][time_interval])

    # create a dataframe
    df = pd.DataFrame.from_dict(analyziz)
    # df["time_interval"] = pd.Categorical(df["time_interval"], categories=X_ORDER, ordered=True)

    # start drawing, bar plot, grouped by scheme
    # scienceplots.style()
    rc('font', weight='bold')
    plt.style.use(['science', 'no-latex', "grid", "light"])
    # ax = sns.lineplot(x="time", y="utility", hue="ratio", 
    #                  hue_order=Link_ORDER, data=df, palette=COLOR_PALETTE)
    # Draw each ratio separately so we can control exact color and linestyle
    plt.figure(figsize=(6, 4))
    ax = plt.gca()
    for ratio in PREDICT_HUE_ORDER:
        sub = df[df["ratio"] == ratio]
        if sub.empty:
            continue
        sns.lineplot(data=sub, x="time_interval", y="utility", ax=ax,
                     label=ratio,
                     color=COLOR_MAP.get(ratio, None),
                     linestyle=LINESTYLE_MAP.get(ratio, "-"),
                     marker=MARKER_MAP.get(ratio, "o"),
                     markersize=6,
                     linewidth=1)
    ax.set_xlabel(PREDICT_X_LABEL, fontsize=10, fontweight='bold')
    ax.set_ylabel("Ratio to Fault-Free (Predicted)", fontsize=9, fontweight='bold')
    ax.set_ylim(0.5, 1.2)
    plt.yticks(np.arange(0.6, 1.1, 0.1))
    # ax.set_xlim(0, 900)
    # plt.xticks(np.arange(75, 900, INTERVAL*2))
    # plt.yticks(np.arange(0, 260, 30))
    # ax.set_yscale('log')
    # ymin, ymax = ax.get_ylim()
    # ax.set_ylim(ymin, ymax * 5)   # 20% extra space
    plt.yticks(fontsize=9)
    plt.xticks(fontsize=8)
    ax.legend(title=NODE_LEGEND_NAME, title_fontsize=9, fontsize=9, loc="upper left", ncol=4, columnspacing=0.5, handletextpad=0.2, borderpad=0.2)
    # ax.legend(title=LEGEND_NAME, title_fontsize=12, fontsize=12, loc="upper left", ncol=4, columnspacing=0, handletextpad=0.2, borderaxespad=0.2, borderpad=0.2)
    # Adjust margins and spacing
    plt.subplots_adjust(
        top=0.99,
        bottom=0.15,
        left=0.093,
        right=0.993,
        hspace=0.2,  # Height spacing between rows
        wspace=0.2   # Width spacing between columns
    )
    plt.show()

    # compute the average value difference for different ratios
    # only consider the time points where time % 100 == 75
    print("=== Node Failure Predicted Summary ===")
    for ratio in PREDICT_HUE_ORDER:
        sub = df[df["ratio"] == ratio]
        if sub.empty:
            continue
        avg_diff = sub["utility"].mean()
        print(f"Average value for {ratio}: {avg_diff:.3f}")


if __name__ == "__main__":
    # draw_failure_performance_measured()
    # draw_link_failure_performance_predicted()
    # draw_node_failure_performance_predicted()
    # draw_node_failure_performance_measured()
    draw_failure_performance_measured_wide()
    draw_failure_performance_predicted_wide()
    # draw_node_failure_performance_predicted_wide()

