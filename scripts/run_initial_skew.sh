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

VALUES=(100 200 400 800 1600 3200)
PROTOCOLS=("LW-Sync" "LT-Sync" "DC-Sync")
TRACKS=(1 2)

OUTPUT_FILE="$RAW_DIR/initial_skew_long.csv"
STATS_FILE="$PROCESSED_DIR/initial_skew_stats.csv"

for protocol in "${PROTOCOLS[@]}"; do
    case "$protocol" in
        LW-Sync) binary="$LW_BINARY" ;;
        LT-Sync) binary="$LT_BINARY" ;;
        DC-Sync) binary="$DC_BINARY" ;;
    esac
    [[ -x "$binary" ]] || { echo "Missing executable: $binary"; exit 1; }
done

echo "Run,ClockSkew,Track,Protocol,OffsetErrorUs" > "$OUTPUT_FILE"

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

    if [[ "$track_number" == "1" ]]; then
        track1=true
        track2=false
    else
        track1=false
        track2=true
    fi

    "$binary" \
        --run="$run_number" \
        --clockSkew="$parameter" \
        --plotNum=8 \
        --track1="$track1" \
        --track2="$track2" \
        2>/dev/null | sed -n 's/^CSV_RESULT,//p'
}

export -f run_one
export LW_BINARY LT_BINARY DC_BINARY

EXPECTED_ROWS=$((TOTAL_RUNS * ${#VALUES[@]} * ${#PROTOCOLS[@]} * ${#TRACKS[@]}))

echo "========================================="
echo "Initial-skew experiment"
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

echo "ClockSkew,T1_LW_Avg,T1_LW_Std,T1_LT_Avg,T1_LT_Std,T1_DC_Avg,T1_DC_Std,T2_LW_Avg,T2_LW_Std,T2_LT_Avg,T2_LT_Std,T2_DC_Avg,T2_DC_Std" > "$STATS_FILE"
awk -F',' '
BEGIN { OFS="," }
NR==1 { next }
NF>=5 {
    p=$2; t=$3; protocol=$4; value=$5+0;
    key=p SUBSEP t SUBSEP protocol;
    sum[key]+=value; sumsq[key]+=value*value; count[key]++;
    parameters[p]=1;
}
END {
    for (p in parameters) {
        lw1=p SUBSEP "1" SUBSEP "LW-Sync";
        lt1=p SUBSEP "1" SUBSEP "LT-Sync";
        dc1=p SUBSEP "1" SUBSEP "DC-Sync";
        lw2=p SUBSEP "2" SUBSEP "LW-Sync";
        lt2=p SUBSEP "2" SUBSEP "LT-Sync";
        dc2=p SUBSEP "2" SUBSEP "DC-Sync";
        print p,
              mean(lw1),stddev(lw1),mean(lt1),stddev(lt1),mean(dc1),stddev(dc1),
              mean(lw2),stddev(lw2),mean(lt2),stddev(lt2),mean(dc2),stddev(dc2);
    }
}
function mean(k) { return count[k] ? sum[k]/count[k] : "NA" }
function stddev(k,avg,var) {
    if (!count[k]) return "NA";
    avg=sum[k]/count[k];
    var=sumsq[k]/count[k]-avg*avg;
    if (var<0 && var>-1e-12) var=0;
    return sqrt(var);
}' "$OUTPUT_FILE" | sort -t',' -k1,1n >> "$STATS_FILE"

echo "Completed successfully."
echo "Raw rows: $ACTUAL_ROWS"
echo "Raw results: $OUTPUT_FILE"
echo "Statistics:  $STATS_FILE"
