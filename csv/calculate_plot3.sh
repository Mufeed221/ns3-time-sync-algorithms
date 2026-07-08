#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NS3_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

INPUT="$SCRIPT_DIR/plot3.csv"
OUTPUT="$NS3_DIR/resutls/plot3_stats(Init. Radial Velocity).csv"

# Header matching the single-track data structure
echo "Init. Radial Velocity,LW_Avg,LW_Std,LT_Avg,LT_Std,DC_Avg,DC_Std" > "$OUTPUT"

awk -F, '
# Skip the header row (NR > 1)
NR > 1 && NF >= 4 {
    v=$1;      # Velocity
    lw=$2;     # LW-Offset
    lt=$3;     # LT-Offset
    dc=$4;     # DC-Offset

    # Accumulate sums for Average and Standard Deviation
    s_lw[v] += lw; sq_lw[v] += lw*lw;
    s_lt[v] += lt; sq_lt[v] += lt*lt;
    s_dc[v] += dc; sq_dc[v] += dc*dc;
    
    count[v]++;
}
END {
    for (v in count) {
        n = count[v];
        
        # Calculate Averages
        a_lw = s_lw[v]/n;
        a_lt = s_lt[v]/n;
        a_dc = s_dc[v]/n;

        # Calculate Standard Deviations: sqrt( (sum_sq/n) - (avg^2) )
        st_lw = sqrt(abs((sq_lw[v]/n) - (a_lw^2)));
        st_lt = sqrt(abs((sq_lt[v]/n) - (a_lt^2)));
        st_dc = sqrt(abs((sq_dc[v]/n) - (a_dc^2)));

        print v "," a_lw "," st_lw "," a_lt "," st_lt "," a_dc "," st_dc;
    }
}
function abs(x) { return x < 0 ? -x : x }
' "$INPUT" | sort -n >> "$OUTPUT"
