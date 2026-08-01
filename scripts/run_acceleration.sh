#!/usr/bin/env bash

set -u
set -o pipefail

TOTAL_RUNS=1000
MAX_PARALLEL="${MAX_PARALLEL:-$(nproc)}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ -x "$SCRIPT_DIR/build/scratch/ns3.40-LW-Sync-default" ]]; then
    NS3_DIR="$SCRIPT_DIR"
elif [[ -x "$SCRIPT_DIR/../build/scratch/ns3.40-LW-Sync-default" ]]; then
    NS3_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
else
    echo "Error: Could not locate the ns-3 build directory."
    exit 1
fi

LW_BINARY="$NS3_DIR/build/scratch/ns3.40-LW-Sync-default"
LT_BINARY="$NS3_DIR/build/scratch/ns3.40-LT-Sync-default"
DC_BINARY="$NS3_DIR/build/scratch/ns3.40-DC-Sync-default"

RAW_DIR="$NS3_DIR/results/raw"
PROCESSED_DIR="$NS3_DIR/results/processed"
mkdir -p "$RAW_DIR" "$PROCESSED_DIR"

if ! command -v parallel >/dev/null 2>&1; then
    echo "Error: GNU Parallel is not installed."
    echo "Install it with: sudo apt install parallel"
    exit 1
fi

if ! [[ "$MAX_PARALLEL" =~ ^[1-9][0-9]*$ ]]; then
    echo "Error: MAX_PARALLEL must be a positive integer."
    exit 1
fi

VALUES=(0.01 0.03 0.05 0.07 0.1)
PROTOCOLS=("LW-Sync" "LT-Sync" "DC-Sync")
TRACKS=(1)

OUTPUT_FILE="$RAW_DIR/acceleration_long.csv"
STATS_FILE="$PROCESSED_DIR/acceleration_stats.csv"

for protocol in "${PROTOCOLS[@]}"; do
    case "$protocol" in
        LW-Sync) binary="$LW_BINARY" ;;
        LT-Sync) binary="$LT_BINARY" ;;
        DC-Sync) binary="$DC_BINARY" ;;
    esac
    [[ -x "$binary" ]] || { echo "Missing executable: $binary"; exit 1; }
done

echo "Run,Acceleration,Track,Protocol,OffsetErrorUs" > "$OUTPUT_FILE"

run_one()
{
    local protocol="$1"
    local track_number="$2"
    local parameter="$3"
    local run_number="$4"
    local binary track1 track2

    case "$protocol" in
        LW-Sync) binary="$LW_BINARY" ;;
        LT-Sync) binary="$LT_BINARY" ;;
        DC-Sync) binary="$DC_BINARY" ;;
        *) return 1 ;;
    esac

    track1=true
    track2=false

    "$binary" \
        --run="$run_number" \
        --acceleration="$parameter" \
        --plotNum=4 \
        --track1="$track1" \
        --track2="$track2" \
        2>/dev/null | sed -n 's/^CSV_RESULT,//p'
}

export -f run_one
export LW_BINARY LT_BINARY DC_BINARY

EXPECTED_ROWS=$((TOTAL_RUNS * ${#VALUES[@]} * ${#PROTOCOLS[@]} * ${#TRACKS[@]}))

echo "========================================="
echo "Acceleration experiment"
echo "VM processors:        $(nproc)"
echo "Parallel workers:     $MAX_PARALLEL"
echo "Expected result rows: $EXPECTED_ROWS"
echo "Raw output:           $OUTPUT_FILE"
echo "Processed output:     $STATS_FILE"
echo "========================================="

parallel --jobs "$MAX_PARALLEL" --bar --line-buffer --halt soon,fail=1 \
    run_one {1} {2} {3} {4} \
    ::: "${PROTOCOLS[@]}" \
    ::: "${TRACKS[@]}" \
    ::: "${VALUES[@]}" \
    ::: $(seq 1 "$TOTAL_RUNS") \
    >> "$OUTPUT_FILE"

status=$?
echo
((status == 0)) || { echo "Experiment failed."; exit "$status"; }

ACTUAL_ROWS=$(($(wc -l < "$OUTPUT_FILE") - 1))
if ((ACTUAL_ROWS != EXPECTED_ROWS)); then
    echo "Error: expected $EXPECTED_ROWS rows, collected $ACTUAL_ROWS."
    exit 1
fi

echo "Acceleration,LW_Avg,LW_Std,LT_Avg,LT_Std,DC_Avg,DC_Std" \
    > "$STATS_FILE"

awk -F',' '
BEGIN {
    OFS = ","
}

NR == 1 {
    next
}

NF >= 5 {
    acceleration = $2
    protocol = $4
    value = $5 + 0

    key = acceleration SUBSEP protocol

    sum[key] += value
    sumsq[key] += value * value
    count[key]++

    accelerations[acceleration] = 1
}

END {
    for (acceleration in accelerations) {
        lw = acceleration SUBSEP "LW-Sync"
        lt = acceleration SUBSEP "LT-Sync"
        dc = acceleration SUBSEP "DC-Sync"

        print acceleration,
              mean(lw), stddev(lw),
              mean(lt), stddev(lt),
              mean(dc), stddev(dc)
    }
}

function mean(key) {
    return count[key] ? sum[key] / count[key] : "NA"
}

function stddev(key, average, variance) {
    if (!count[key]) {
        return "NA"
    }

    average = sum[key] / count[key]
    variance = (sumsq[key] / count[key]) - (average * average)

    if (variance < 0 && variance > -1e-12) {
        variance = 0
    }

    return sqrt(variance)
}
' "$OUTPUT_FILE" |
    sort -t',' -k1,1n \
    >> "$STATS_FILE"

echo "Completed successfully."
echo "Raw rows: $ACTUAL_ROWS"
echo "Raw results: $OUTPUT_FILE"
echo "Statistics:  $STATS_FILE"
