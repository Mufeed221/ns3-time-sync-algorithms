#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NS3_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

INPUT="$SCRIPT_DIR/plot9b.csv"
OUTPUT="$NS3_DIR/results/plot9b_stats(Skew error).csv"

# Write the header for the 4 final values
echo "T1_LT_Avg,T1_LT_Std,T2_LT_Avg,T2_LT_Std" > "$OUTPUT"

awk -F, '
NR > 2 && NF >= 4 {
    lt1=$2; lt2=$4;
    
    # Accumulate sums and squared sums across all 500 runs
    s_lt1 += lt1; sq_lt1 += lt1 * lt1;
    s_lt2 += lt2; sq_lt2 += lt2 * lt2;
    count++;
}
END {
    if (count > 0) {
        # Track 1 Calculations
        mean1 = s_lt1 / count;
        var1 = (sq_lt1 / count) - (mean1 ^ 2);
        std1 = sqrt(abs(var1));
        
        # Track 2 Calculations
        mean2 = s_lt2 / count;
        var2 = (sq_lt2 / count) - (mean2 ^ 2);
        std2 = sqrt(abs(var2));
        
        # Output exactly the 4 requested values
        print mean1 "," std1 "," mean2 "," std2;
    }
}
function abs(x){return x<0?-x:x}' "$INPUT" >> "$OUTPUT"
