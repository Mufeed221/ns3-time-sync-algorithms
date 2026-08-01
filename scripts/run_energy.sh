#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NS3_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$NS3_DIR" || exit 1
   ./ns3 run "scratch/LW-Sync --plotNum=10" > /dev/null 2>&1 
   ./ns3 run "scratch/LT-Sync --plotNum=10" > /dev/null 2>&1 
   ./ns3 run "scratch/DC-Sync --plotNum=10" > /dev/null 2>&1 
   
   printf "\n=== COMPLETE! energy results script ===\n"
   
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ -d "$SCRIPT_DIR/results" ]]; then
    NS3_DIR="$SCRIPT_DIR"
elif [[ -d "$SCRIPT_DIR/../results" ]]; then
    NS3_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
else
    echo "Error: Could not locate the results directory."
    exit 1
fi

RAW_DIR="$NS3_DIR/results/raw"
PROCESSED_DIR="$NS3_DIR/results/processed"

ENERGY_FILE="$RAW_DIR/energy.csv"
DISTANCE_STATS="$PROCESSED_DIR/distance_stats.csv"
LT_SKEW_STATS="$PROCESSED_DIR/LT_skew_estimate_stats.csv"
DC_SKEW_STATS="$PROCESSED_DIR/DC_message_count_stats.csv"

OUTPUT_FILE="$PROCESSED_DIR/energy_stats.csv"

mkdir -p "$PROCESSED_DIR"

for file in \
    "$ENERGY_FILE" \
    "$DISTANCE_STATS" \
    "$LT_SKEW_STATS" \
    "$DC_SKEW_STATS"
do
    if [[ ! -f "$file" ]]; then
        echo "Error: Required file not found:"
        echo "  $file"
        exit 1
    fi
done

# ------------------------------------------------------------
# Read total energy consumed by one synchronization execution.
#
# energy.csv format:
# Protocol,Beacon Node(J),Ordinary Node(J),Total(J)
# ------------------------------------------------------------

LW_TOTAL_ENERGY=$(
    awk -F',' '
        $1 == "LW-Sync" {
            gsub(/\r/, "", $4)
            print $4
            exit
        }
    ' "$ENERGY_FILE"
)

LT_TOTAL_ENERGY=$(
    awk -F',' '
        $1 == "LT-Sync" {
            gsub(/\r/, "", $4)
            print $4
            exit
        }
    ' "$ENERGY_FILE"
)

DC_TOTAL_ENERGY=$(
    awk -F',' '
        $1 == "DC-Sync" {
            gsub(/\r/, "", $4)
            print $4
            exit
        }
    ' "$ENERGY_FILE"
)

# ------------------------------------------------------------
# Read offset errors at an initial distance of 500 m.
#
# distance_stats.csv:
# Distance,
# T1_LW_Avg,T1_LW_Std,
# T1_LT_Avg,T1_LT_Std,
# T1_DC_Avg,T1_DC_Std,...
#
# LT offset average = column 4
# DC offset average = column 6
#
# These values are assumed to be in microseconds.
# ------------------------------------------------------------

read -r LT_BETA_ERROR_US DC_BETA_ERROR_US < <(
    awk -F',' '
        NR > 1 && ($1 + 0) == 500 {
            gsub(/\r/, "", $4)
            gsub(/\r/, "", $6)

            print $4, $6
            exit
        }
    ' "$DISTANCE_STATS"
)

# Convert microseconds to seconds.
LT_BETA_ERROR_S=$(awk -v value="$LT_BETA_ERROR_US" \
    'BEGIN { printf "%.15g", value * 1e-6 }')

DC_BETA_ERROR_S=$(awk -v value="$DC_BETA_ERROR_US" \
    'BEGIN { printf "%.15g", value * 1e-6 }')

# ------------------------------------------------------------
# Read LT-Sync skew error.
#
# LT_skew_estimate_stats.csv:
# T1_LT_Avg,T1_LT_Std,T2_LT_Avg,T2_LT_Std
# value is taken from the second row, first column.
#
# The value is in ppm and is converted to fractional skew.
# ------------------------------------------------------------

LT_ALPHA_ERROR_PPM=$(
    awk -F',' '
        NR == 2 {
            gsub(/\r/, "", $1)
            print $1
            exit
        }
    ' "$LT_SKEW_STATS"
)

LT_ALPHA_ERROR=$(
    awk -v value="$LT_ALPHA_ERROR_PPM" \
        'BEGIN { printf "%.15g", value * 1e-6 }'
)

# ------------------------------------------------------------
# Read DC-Sync skew error for message count 20.
#
# DC_message_count_stats.csv:
# MsgCount,T1_DCSkew_Avg,T1_DCSkew_Std,...
#
# T1 DC skew average = column 2.
# The value is in ppm and is converted to fractional skew.
# ------------------------------------------------------------

DC_ALPHA_ERROR_PPM=$(
    awk -F',' '
        NR > 1 && ($1 + 0) == 20 {
            gsub(/\r/, "", $2)
            print $2
            exit
        }
    ' "$DC_SKEW_STATS"
)

DC_ALPHA_ERROR=$(
    awk -v value="$DC_ALPHA_ERROR_PPM" \
        'BEGIN { printf "%.15g", value * 1e-6 }'
)

# ------------------------------------------------------------
# Validate extracted values.
# ------------------------------------------------------------

check_value()
{
    local name="$1"
    local value="$2"

    if [[ -z "$value" || "$value" == "NA" ]]; then
        echo "Error: Could not obtain $name."
        exit 1
    fi

    if ! [[ "$value" =~ ^[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?$ ]]; then
        echo "Error: Invalid numeric value for $name: $value"
        exit 1
    fi
}

check_value "LW total energy" "$LW_TOTAL_ENERGY"
check_value "LT total energy" "$LT_TOTAL_ENERGY"
check_value "DC total energy" "$DC_TOTAL_ENERGY"

check_value "LT offset error" "$LT_BETA_ERROR_S"
check_value "DC offset error" "$DC_BETA_ERROR_S"

check_value "LT skew error" "$LT_ALPHA_ERROR"
check_value "DC skew error" "$DC_ALPHA_ERROR"

# Skew error must be greater than zero because it is used as a divisor.
awk -v lt="$LT_ALPHA_ERROR" -v dc="$DC_ALPHA_ERROR" '
BEGIN {
    if (lt <= 0 || dc <= 0) {
        exit 1
    }
}
' || {
    echo "Error: LT or DC skew error is zero or negative."
    exit 1
}

# ------------------------------------------------------------
# Generate processed results.
# ------------------------------------------------------------

echo \
"Avg. events per week,LW-Sync Consumed Energy (J),Error Tolerance (s),DC-Sync Consumed Energy (J),LT-Sync Consumed Energy (J)" \
    > "$OUTPUT_FILE"

EVENTS=(1 2 3 4 5)
ERROR_TOLERANCES=(0.02 0.03 0.04 0.05)

echo \
"Avg. events per week,LW-Sync Consumed Energy (J),Error Tolerance (s),DC-Sync Consumed Energy (J),LT-Sync Consumed Energy (J)" \
> "$OUTPUT_FILE"

EVENTS=(1 2 3 4 5)
ERROR_TOLERANCES=(0.02 0.03 0.04 0.05)

for index in "${!EVENTS[@]}"; do
    events="${EVENTS[$index]}"

    lw_weekly_energy=$(
        awk -v events="$events" -v energy="$LW_TOTAL_ENERGY" '
        BEGIN {
            printf "%.10g", events * energy
        }'
    )

    # The fifth row contains only LW-Sync energy.
    if ((index >= ${#ERROR_TOLERANCES[@]})); then
        echo "$events,$lw_weekly_energy,,," >> "$OUTPUT_FILE"
        continue
    fi

    tolerance="${ERROR_TOLERANCES[$index]}"

    awk \
        -v events="$events" \
        -v tolerance="$tolerance" \
        -v lwWeeklyEnergy="$lw_weekly_energy" \
        -v ltEnergy="$LT_TOTAL_ENERGY" \
        -v dcEnergy="$DC_TOTAL_ENERGY" \
        -v ltBeta="$LT_BETA_ERROR_S" \
        -v dcBeta="$DC_BETA_ERROR_S" \
        -v ltAlpha="$LT_ALPHA_ERROR" \
        -v dcAlpha="$DC_ALPHA_ERROR" '
    BEGIN {
        secondsPerWeek = 604800

        ltInterval = (tolerance - ltBeta) / ltAlpha
        dcInterval = (tolerance - dcBeta) / dcAlpha

        if (ltInterval <= 0 || dcInterval <= 0) {
            print "Error: invalid synchronization interval" > "/dev/stderr"
            exit 1
        }

        ltWeeklyCycles = secondsPerWeek / ltInterval
        dcWeeklyCycles = secondsPerWeek / dcInterval

        ltWeeklyEnergy = ltWeeklyCycles * ltEnergy
        dcWeeklyEnergy = dcWeeklyCycles * dcEnergy

        printf "%d,%.10g,%.6g,%.10g,%.10g\n", \
            events, \
            lwWeeklyEnergy, \
            tolerance, \
            dcWeeklyEnergy, \
            ltWeeklyEnergy
    }' >> "$OUTPUT_FILE" || exit 1
done

echo "========================================="
echo "Energy statistics completed successfully."
echo "========================================="
echo "LW energy per event:    $LW_TOTAL_ENERGY J"
echo "LT energy per cycle:    $LT_TOTAL_ENERGY J"
echo "DC energy per cycle:    $DC_TOTAL_ENERGY J"
echo
echo "LT offset error:        $LT_BETA_ERROR_US us"
echo "DC offset error:        $DC_BETA_ERROR_US us"
echo "LT skew error:          $LT_ALPHA_ERROR_PPM ppm"
echo "DC skew error:          $DC_ALPHA_ERROR_PPM ppm"
echo
echo "Processed output:"
echo "  $OUTPUT_FILE"
echo "========================================="   
