# Variables
SCALAR_NAME="savedEnergy:sum"
file="results/Full-Test-MEC-1/scheAll-false-countExeTime-false-FastLR-pilot-MAX_CQI.sca"
SCAVETOOL="opp_scavetool"

# Extract scalar value using opp_scavetool query
# value=$($SCAVETOOL query -l -f "name=~\"$SCALAR_NAME\"" "$file" | awk '/scalar/ {print $NF}')
value=$(grep "scalar" "$file" | grep "$SCALAR_NAME" | cut -d' ' -f4)

# Check and print
if [ -z "$value" ]; then
    echo "Failed to extract $SCALAR_NAME from $file"
    exit 1
fi

echo "Extracted $SCALAR_NAME = $value"
