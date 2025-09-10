#!/bin/bash

# Works with OMNeT++ 6.x (opp_scavetool)
# Searches for .sca files in a given directory
# Filters by values of scheAll and countExeTime (both adjustable)
# Extracts a specific scalar (e.g., vehSavedEnergy:sum)
# Parses file names to extract algorithm and pilot
# Writes output as a CSV: algorithm,pilot,energySaving

SCAVETOOL="opp_scavetool"
# === Configurable Parameters ===
SCALAR_NAME="vehSavedEnergy:sum"             # Just the scalar name
SCHE_ALL="true"                           # or "false"
COUNT_EXE_TIME="true"                     # or "false"
INPUT_DIR="results/Full-Test-MEC-1"
OUTPUT_DIR="result-analysis/mec1/full"

# === Ensure output directory exists ===
if [ ! -d "$OUTPUT_DIR" ]; then
    mkdir -p "$OUTPUT_DIR"
fi
OUTPUT_CSV="${OUTPUT_DIR}/scheAll_${SCHE_ALL}_countExeTime_${COUNT_EXE_TIME}_actual_energy_sum.csv"

# === Initialize CSV ===
echo "algorithm,pilot,energy" > "$OUTPUT_CSV"

# Loop through all .sca files in the folder
for file in "$INPUT_DIR"/*.sca; do
    filename=$(basename "$file")

    # Parse scheAll and countExeTime from filename
    scheAll=$(echo "$filename" | grep -oP 'scheAll-\K[^-]+')
    countExeTime=$(echo "$filename" | grep -oP 'countExeTime-\K[^-]+')

    # Check if both parameters match the required values
    if [[ "$scheAll" != "$SCHE_ALL" || "$countExeTime" != "$COUNT_EXE_TIME" ]]; then
        # echo "Skipping $filename (scheAll: $scheAll, countExeTime: $countExeTime)"
        continue
    fi

    # Extract algorithm (after countExeTime) and pilot
    algorithm=$(echo "$filename" | sed -n 's/.*countExeTime-[^-]*-\([^-.]*\).*/\1/p')
    pilot=$(echo "$filename" | grep -oP 'pilot-\K[^-.]+')

    # Extract all matching scalar lines, then loop over them
    # because each app has its own savedEnergy, there will be multiple lines
    # Each scalar line format: scalar <module> <name> <value>
    # e.g., scalar VEC.ue[1].app[0] vehSavedEnergy:sum 1170455.4980001
    grep "^scalar" "$file" | grep "$SCALAR_NAME" | while read -r line; do
        # Extract module and value (4 fields: scalar, module, name, value)
        value=$(echo "$line" | cut -d' ' -f4)
        # Write to CSV
        echo "$algorithm,$pilot,$value" >> "$OUTPUT_CSV"
    done
done

echo "Filtered scalar extraction complete."
echo "Saved to: $OUTPUT_CSV"
