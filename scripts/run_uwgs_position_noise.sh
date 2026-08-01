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
UWGS_BINARY="$NS3_DIR/build/scratch/ns3.40-UWGS-default"

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

VALUES=(0 0.5 1 2 3 4 5 10)
PROTOCOLS=("LW-Sync" "UWGS")

OUTPUT_FILE="$RAW_DIR/UWGS_position_noise_long.csv"
OFFSET_STATS="$PROCESSED_DIR/UWGS_position_noise_(offset)_stats.csv"
SKEW_STATS="$PROCESSED_DIR/UWGS_position_noise_(skew)_stats.csv"

[[ -x "$LW_BINARY" ]] || { echo "Missing executable: $LW_BINARY"; exit 1; }
[[ -x "$UWGS_BINARY" ]] || { echo "Missing executable: $UWGS_BINARY"; exit 1; }

echo "Run,TrajectoryPositionNoise,Protocol,Metric,Value" > "$OUTPUT_FILE"

run_one()
{
    local protocol="$1"
    local parameter="$2"
    local run_number="$3"
    local binary

    if [[ "$protocol" == "LW-Sync" ]]; then
        binary="$LW_BINARY"
        "$binary" --run="$run_number" --trajPosNoiseStd="$parameter" --plotNum=2 \
            --track1=false --track2=false --trackUWGS=true 2>/dev/null |
            sed -n 's/^CSV_RESULT,//p'
    else
        binary="$UWGS_BINARY"
        "$binary" --run="$run_number" --trajPosNoiseStd="$parameter" --plotNum=2 \
            2>/dev/null | sed -n 's/^CSV_RESULT,//p'
    fi
}

export -f run_one
export LW_BINARY UWGS_BINARY

# Two metric rows (Offset and Skew) are expected from each simulation.
EXPECTED_ROWS=$((TOTAL_RUNS * ${#VALUES[@]} * ${#PROTOCOLS[@]} * 2))

echo "========================================="
echo "UWGS position-noise experiment"
echo "VM processors:        $(nproc)"
echo "Parallel workers:     $MAX_PARALLEL"
echo "Expected result rows: $EXPECTED_ROWS"
echo "========================================="

parallel --jobs "$MAX_PARALLEL" --bar --line-buffer --halt soon,fail=1 \
    run_one {1} {2} {3} \
    ::: "${PROTOCOLS[@]}" \
    ::: "${VALUES[@]}" \
    ::: $(seq 1 "$TOTAL_RUNS") \
    >> "$OUTPUT_FILE"

status=$?
echo
((status == 0)) || { echo "Experiment failed."; exit "$status"; }

ACTUAL_ROWS=$(($(wc -l < "$OUTPUT_FILE") - 1))
if ((ACTUAL_ROWS != EXPECTED_ROWS)); then
    echo "Error: expected $EXPECTED_ROWS rows, collected $ACTUAL_ROWS."
    echo "Each run must print both Offset and Skew CSV_RESULT records."
    exit 1
fi

calculate_stats()
{
    local metric="$1"
    local output="$2"
    echo "TrajectoryPositionNoise,LW_Avg,LW_Std,UWGS_Avg,UWGS_Std" > "$output"

    awk -F',' -v wanted="$metric" '
    BEGIN { OFS="," }
    NR==1 { next }
    NF>=5 && $4==wanted {
        p=$2; protocol=$3; value=$5+0;
        key=p SUBSEP protocol;
        sum[key]+=value; sumsq[key]+=value*value; count[key]++;
        parameters[p]=1;
    }
    END {
        for (p in parameters) {
            lw=p SUBSEP "LW-Sync";
            uw=p SUBSEP "UWGS";
            print p,mean(lw),stddev(lw),mean(uw),stddev(uw);
        }
    }
    function mean(k) { return count[k] ? sum[k]/count[k] : "NA" }
    function stddev(k,avg,var) {
        if (!count[k]) return "NA";
        avg=sum[k]/count[k];
        var=sumsq[k]/count[k]-avg*avg;
        if (var<0 && var>-1e-12) var=0;
        return sqrt(var);
    }' "$OUTPUT_FILE" | sort -t',' -k1,1n >> "$output"
}

calculate_stats "Offset" "$OFFSET_STATS"
calculate_stats "Skew" "$SKEW_STATS"

echo "Completed successfully."
echo "Raw results:   $OUTPUT_FILE"
echo "Offset stats:  $OFFSET_STATS"
echo "Skew stats:    $SKEW_STATS"
