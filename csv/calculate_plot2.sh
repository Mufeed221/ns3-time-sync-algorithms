#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NS3_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

INPUT="$SCRIPT_DIR/plot2.csv"
OUTPUT="$NS3_DIR/results/plot2_stats(Angle).csv"

# Header matching the single-track data structure
echo "Angle,LW_Avg,LW_Std,LT_Avg,LT_Std,DC_Avg,DC_Std" > "$OUTPUT"

awk -F, '
# Skip the header row (NR > 1)
NR > 1 && NF >= 4 {
    a=$1;      # Angle
    lw=$2;     # LW-Offset
    lt=$3;     # LT-Offset
    dc=$4;     # DC-Offset

    # Accumulate sums for Average and Standard Deviation
    s_lw[a] += lw; sq_lw[a] += lw*lw;
    s_lt[a] += lt; sq_lt[a] += lt*lt;
    s_dc[a] += dc; sq_dc[a] += dc*dc;
    
    count[a]++;
}
END {
    for (a in count) {
        n = count[a];
        
        # Calculate Averages
        a_lw = s_lw[a]/n;
        a_lt = s_lt[a]/n;
        a_dc = s_dc[a]/n;

        # Calculate Standard Deviations: sqrt( (sum_sq/n) - (avg^2) )
        st_lw = sqrt(abs((sq_lw[a]/n) - (a_lw^2)));
        st_lt = sqrt(abs((sq_lt[a]/n) - (a_lt^2)));
        st_dc = sqrt(abs((sq_dc[a]/n) - (a_dc^2)));

        print a "," a_lw "," st_lw "," a_lt "," st_lt "," a_dc "," st_dc;
    }
}
function abs(x) { return x < 0 ? -x : x }
' "$INPUT" | sort -n >> "$OUTPUT"
