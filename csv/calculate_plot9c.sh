#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NS3_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

INPUT="$SCRIPT_DIR/plot9c.csv"
OUTPUT="$NS3_DIR/results/plot9c_stats(Messages Count).csv"

echo "MsgCount,T1_DCSkew_Avg,T1_DCSkew_Std,T2_DCSkew_Avg,T2_DCSkew_Std" > "$OUTPUT"
awk -F, 'NR > 2 && NF >= 4 {
    v=$1; dc1=$2; dc2=$4;
    s_dc1[v]+=dc1; sq_dc1[v]+=dc1*dc1;
    s_dc2[v]+=dc2; sq_dc2[v]+=dc2*dc2;
    count[v]++;
}
END {
    for (v in count) { 
    	n=count[v];
        print v "," s_dc1[v]/n "," sqrt(abs((sq_dc1[v]/n)-(s_dc1[v]/n)^2)) "," s_dc2[v]/n "," sqrt(abs((sq_dc2[v]/n)-(s_dc2[v]/n)^2));
    }
}
function abs(x){return x<0?-x:x}' "$INPUT" | sort -n >> "$OUTPUT"
