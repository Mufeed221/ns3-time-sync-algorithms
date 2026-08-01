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

COUNTS=(10 15 20 25 30)
TRACKS=(1 2)

OUTPUT_FILE="$RAW_DIR/message_count_long.csv"
LW_STATS="$PROCESSED_DIR/LW_beacon_count_stats.csv"
LT_STATS="$PROCESSED_DIR/LT_skew_estimate_stats.csv"
DC_STATS="$PROCESSED_DIR/DC_message_count_stats.csv"

for binary in "$LW_BINARY" "$LT_BINARY" "$DC_BINARY"; do
    [[ -x "$binary" ]] || { echo "Missing executable: $binary"; exit 1; }
done

echo "Run,MessageCount,Track,Protocol,SkewErrorPpm" > "$OUTPUT_FILE"

run_counted()
{
    local protocol="$1"
    local track="$2"
    local count="$3"
    local run="$4"
    local binary track1 track2 argument

    if [[ "$track" == "1" ]]; then track1=true; track2=false;
    else track1=false; track2=true; fi

    if [[ "$protocol" == "LW-Sync" ]]; then
        binary="$LW_BINARY"
        argument="--beaconCount=$count"
    else
        binary="$DC_BINARY"
        argument="--messagePairsCount=$count"
    fi

    "$binary" --run="$run" "$argument" --plotNum=9 \
        --track1="$track1" --track2="$track2" 2>/dev/null |
        sed -n 's/^CSV_RESULT,//p'
}

run_lt()
{
    local track="$1"
    local run="$2"
    local track1 track2

    if [[ "$track" == "1" ]]; then track1=true; track2=false;
    else track1=false; track2=true; fi

    "$LT_BINARY" --run="$run" --plotNum=9 \
        --track1="$track1" --track2="$track2" 2>/dev/null |
        sed -n 's/^CSV_RESULT,//p'
}

export -f run_counted run_lt
export LW_BINARY LT_BINARY DC_BINARY

EXPECTED_COUNTED=$((TOTAL_RUNS * ${#COUNTS[@]} * 2 * ${#TRACKS[@]}))
EXPECTED_LT=$((TOTAL_RUNS * ${#TRACKS[@]}))
EXPECTED_ROWS=$((EXPECTED_COUNTED + EXPECTED_LT))

echo "========================================="
echo "Message-count experiment"
echo "VM processors:        $(nproc)"
echo "Parallel workers:     $MAX_PARALLEL"
echo "Expected result rows: $EXPECTED_ROWS"
echo "========================================="

{
parallel --jobs "$MAX_PARALLEL" --bar --line-buffer --halt soon,fail=1 \
    run_counted {1} {2} {3} {4} \
    ::: LW-Sync DC-Sync \
    ::: "${TRACKS[@]}" \
    ::: "${COUNTS[@]}" \
    ::: $(seq 1 "$TOTAL_RUNS")

parallel --jobs "$MAX_PARALLEL" --bar --line-buffer --halt soon,fail=1 \
    run_lt {1} {2} \
    ::: "${TRACKS[@]}" \
    ::: $(seq 1 "$TOTAL_RUNS")
} >> "$OUTPUT_FILE"

status=$?
echo
((status == 0)) || { echo "Experiment failed."; exit "$status"; }

ACTUAL_ROWS=$(($(wc -l < "$OUTPUT_FILE") - 1))
if ((ACTUAL_ROWS != EXPECTED_ROWS)); then
    echo "Error: expected $EXPECTED_ROWS rows, collected $ACTUAL_ROWS."
    exit 1
fi

# LW and DC grouped by message count and track.
calculate_counted()
{
    local protocol="$1"
    local output="$2"
    local prefix="$3"
    echo "MsgCount,T1_${prefix}_Avg,T1_${prefix}_Std,T2_${prefix}_Avg,T2_${prefix}_Std" > "$output"
    awk -F',' -v wanted="$protocol" '
    BEGIN { OFS="," }
    NR==1 { next }
    NF>=5 && $4==wanted {
        c=$2; t=$3; value=$5+0;
        key=c SUBSEP t;
        sum[key]+=value; sumsq[key]+=value*value; count[key]++;
        counts[c]=1;
    }
    END {
        for (c in counts) {
            t1=c SUBSEP "1"; t2=c SUBSEP "2";
            print c,mean(t1),stddev(t1),mean(t2),stddev(t2);
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

calculate_counted "LW-Sync" "$LW_STATS" "LWSkew"
calculate_counted "DC-Sync" "$DC_STATS" "DCSkew"

echo "T1_LT_Avg,T1_LT_Std,T2_LT_Avg,T2_LT_Std" > "$LT_STATS"
awk -F',' '
NR==1 { next }
NF>=5 && $4=="LT-Sync" {
    t=$3; value=$5+0;
    sum[t]+=value; sumsq[t]+=value*value; count[t]++;
}
END {
    print mean("1"),stddev("1"),mean("2"),stddev("2");
}
function mean(k) { return count[k] ? sum[k]/count[k] : "NA" }
function stddev(k,avg,var) {
    if (!count[k]) return "NA";
    avg=sum[k]/count[k];
    var=sumsq[k]/count[k]-avg*avg;
    if (var<0 && var>-1e-12) var=0;
    return sqrt(var);
}' "$OUTPUT_FILE" >> "$LT_STATS"

echo "Completed successfully."
echo "Raw results: $OUTPUT_FILE"
echo "LW stats:    $LW_STATS"
echo "LT stats:    $LT_STATS"
echo "DC stats:    $DC_STATS"
