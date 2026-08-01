#!/usr/bin/env bash

set -u
set -o pipefail

TOTAL_RUNS=1000

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Use all CPU processors visible to the Ubuntu VM.
# You may override it manually:
#   MAX_PARALLEL=5 ./run_distance.sh
MAX_PARALLEL="${MAX_PARALLEL:-$(nproc)}"

# Support placing this script either:
#   1. In the ns-3 root directory, or
#   2. In a directory directly below the ns-3 root.
if [[ -x "$SCRIPT_DIR/build/scratch/ns3.40-LW-Sync-default" ]]; then
    NS3_DIR="$SCRIPT_DIR"
elif [[ -x "$SCRIPT_DIR/../build/scratch/ns3.40-LW-Sync-default" ]]; then
    NS3_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
else
    echo "Error: Could not locate the ns-3 build directory."
    echo "Expected executable:"
    echo "  build/scratch/ns3.40-LW-Sync-default"
    exit 1
fi

LW_BINARY="$NS3_DIR/build/scratch/ns3.40-LW-Sync-default"
LT_BINARY="$NS3_DIR/build/scratch/ns3.40-LT-Sync-default"
DC_BINARY="$NS3_DIR/build/scratch/ns3.40-DC-Sync-default"

RAW_DIR="$NS3_DIR/results/raw"
PROCESSED_DIR="$NS3_DIR/results/processed"

OUTPUT_FILE="$RAW_DIR/distance_long.csv"
STATS_FILE="$PROCESSED_DIR/distance_stats.csv"

DISTANCES=(500 600 700 800 900 1000)
PROTOCOLS=("LW-Sync" "LT-Sync" "DC-Sync")
TRACKS=(1 2)

if ! command -v parallel >/dev/null 2>&1; then
    echo "Error: GNU Parallel is not installed."
    echo "Install it using:"
    echo "  sudo apt install parallel"
    exit 1
fi

if ! [[ "$MAX_PARALLEL" =~ ^[1-9][0-9]*$ ]]; then
    echo "Error: MAX_PARALLEL must be a positive integer."
    exit 1
fi

for binary in "$LW_BINARY" "$LT_BINARY" "$DC_BINARY"; do
    if [[ ! -x "$binary" ]]; then
        echo "Error: Executable not found or not executable:"
        echo "  $binary"
        exit 1
    fi
done

mkdir -p "$RAW_DIR"
mkdir -p "$PROCESSED_DIR"

# Remove results from a previous execution.
rm -f "$OUTPUT_FILE"
rm -f "$STATS_FILE"

# Write the raw CSV header.
echo "Run,InitialDistance,Track,Protocol,OffsetErrorUs" \
    > "$OUTPUT_FILE"

run_one()
{
    local protocol="$1"
    local track_number="$2"
    local distance="$3"
    local run_number="$4"

    local binary
    local track1
    local track2

    case "$protocol" in
        LW-Sync)
            binary="$LW_BINARY"
            ;;

        LT-Sync)
            binary="$LT_BINARY"
            ;;

        DC-Sync)
            binary="$DC_BINARY"
            ;;

        *)
            echo "Unknown protocol: $protocol" >&2
            return 1
            ;;
    esac

    case "$track_number" in
        1)
            track1=true
            track2=false
            ;;

        2)
            track1=false
            track2=true
            ;;

        *)
            echo "Invalid track number: $track_number" >&2
            return 1
            ;;
    esac

    "$binary" \
        --run="$run_number" \
        --initDistance="$distance" \
        --plotNum=1 \
        --track1="$track1" \
        --track2="$track2" \
        2>/dev/null |
        sed -n 's/^CSV_RESULT,//p'
}

export -f run_one
export LW_BINARY
export LT_BINARY
export DC_BINARY

NUMBER_OF_DISTANCES=${#DISTANCES[@]}
NUMBER_OF_PROTOCOLS=${#PROTOCOLS[@]}
NUMBER_OF_TRACKS=${#TRACKS[@]}

EXPECTED_ROWS=$(( \
    TOTAL_RUNS *
    NUMBER_OF_DISTANCES *
    NUMBER_OF_PROTOCOLS *
    NUMBER_OF_TRACKS
))

echo "========================================="
echo "Distance experiment"
echo "========================================="
echo "VM processors:        $(nproc)"
echo "Parallel workers:     $MAX_PARALLEL"
echo "Monte Carlo runs:     $TOTAL_RUNS"
echo "Distances:            ${DISTANCES[*]}"
echo "Protocols:            ${PROTOCOLS[*]}"
echo "Tracks:               ${TRACKS[*]}"
echo "Expected result rows: $EXPECTED_ROWS"
echo "Raw output:           $OUTPUT_FILE"
echo "Processed output:     $STATS_FILE"
echo "========================================="

parallel \
    --jobs "$MAX_PARALLEL" \
    --bar \
    --line-buffer \
    --halt soon,fail=1 \
    run_one {1} {2} {3} {4} \
    ::: "${PROTOCOLS[@]}" \
    ::: "${TRACKS[@]}" \
    ::: "${DISTANCES[@]}" \
    ::: $(seq 1 "$TOTAL_RUNS") \
    >> "$OUTPUT_FILE"

parallel_status=$?

echo

if ((parallel_status != 0)); then
    echo "========================================="
    echo "Distance experiment failed."
    echo "Check the job log:"
    echo "========================================="
    exit "$parallel_status"
fi

# Exclude the CSV header from the result-row count.
ACTUAL_ROWS=$(($(wc -l < "$OUTPUT_FILE") - 1))

if ((ACTUAL_ROWS != EXPECTED_ROWS)); then
    echo "========================================="
    echo "Error: Result-row count differs."
    echo "Expected rows: $EXPECTED_ROWS"
    echo "Actual rows:   $ACTUAL_ROWS"
    echo
    echo "One or more simulations may have finished without"
    echo "printing a CSV_RESULT line."
    echo "========================================="
    exit 1
fi

echo "Raw simulations completed successfully."
echo "Collected rows: $ACTUAL_ROWS"
echo
echo "Calculating mean and standard deviation..."

# Write processed CSV header.
echo \
"Distance,T1_LW_Avg,T1_LW_Std,T1_LT_Avg,T1_LT_Std,T1_DC_Avg,T1_DC_Std,T2_LW_Avg,T2_LW_Std,T2_LT_Avg,T2_LT_Std,T2_DC_Avg,T2_DC_Std" \
    > "$STATS_FILE"

awk -F',' '
BEGIN {
    OFS = ","
}

NR == 1 {
    next
}

NF >= 5 {
    distance = $2
    track = $3
    protocol = $4
    value = $5 + 0

    key = distance SUBSEP track SUBSEP protocol

    sum[key] += value
    sumsq[key] += value * value
    count[key]++

    distances[distance] = 1
}

END {
    for (distance in distances) {
        lw1 = distance SUBSEP "1" SUBSEP "LW-Sync"
        lt1 = distance SUBSEP "1" SUBSEP "LT-Sync"
        dc1 = distance SUBSEP "1" SUBSEP "DC-Sync"

        lw2 = distance SUBSEP "2" SUBSEP "LW-Sync"
        lt2 = distance SUBSEP "2" SUBSEP "LT-Sync"
        dc2 = distance SUBSEP "2" SUBSEP "DC-Sync"

        print distance,
              mean(lw1), stddev(lw1),
              mean(lt1), stddev(lt1),
              mean(dc1), stddev(dc1),
              mean(lw2), stddev(lw2),
              mean(lt2), stddev(lt2),
              mean(dc2), stddev(dc2)
    }
}

function mean(key) {
    if (count[key] == 0) {
        return "NA"
    }

    return sum[key] / count[key]
}

function stddev(key, average, variance) {
    if (count[key] == 0) {
        return "NA"
    }

    average = sum[key] / count[key]

    # Population standard deviation, matching your previous script.
    variance = (sumsq[key] / count[key]) - (average * average)

    # Prevent sqrt() of a tiny negative value caused by floating-point
    # rounding.
    if (variance < 0 && variance > -1e-12) {
        variance = 0
    }

    return sqrt(variance)
}
' "$OUTPUT_FILE" |
    sort -t',' -k1,1n \
    >> "$STATS_FILE"

stats_status=${PIPESTATUS[0]}

if ((stats_status != 0)); then
    echo "Error: Failed to calculate the processed statistics."
    exit "$stats_status"
fi

PROCESSED_ROWS=$(($(wc -l < "$STATS_FILE") - 1))

if ((PROCESSED_ROWS != NUMBER_OF_DISTANCES)); then
    echo "Warning: Unexpected number of processed distance rows."
    echo "Expected: $NUMBER_OF_DISTANCES"
    echo "Actual:   $PROCESSED_ROWS"
fi

echo
echo "========================================="
echo "Distance experiment completed successfully."
echo "========================================="
echo "Parallel workers: $MAX_PARALLEL"
echo "Raw rows:         $ACTUAL_ROWS"
echo "Processed rows:   $PROCESSED_ROWS"
echo
echo "Raw results:"
echo "  $OUTPUT_FILE"
echo
echo "Processed statistics:"
echo "  $STATS_FILE"
echo
echo "GNU Parallel job log:"
echo "========================================="
