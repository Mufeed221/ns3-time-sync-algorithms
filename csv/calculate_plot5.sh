#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NS3_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

INPUT="$SCRIPT_DIR/plot5.csv"
OUTPUT="$NS3_DIR/results/plot5_stats(Velocity Noise).csv"

echo "Vel_Noise,T1_LW_Avg,T1_LW_Std,T1_LT_Avg,T1_LT_Std,T1_DC_Avg,T1_DC_Std,T2_LW_Avg,T2_LW_Std,T2_LT_Avg,T2_LT_Std,T2_DC_Avg,T2_DC_Std" > "$OUTPUT"
awk -F, 'NR > 2 && NF >= 8 {
    v=$1; lw1=$2; lt1=$3; dc1=$4; lw2=$6; lt2=$7; dc2=$8;
    s_lw1[v]+=lw1; sq_lw1[v]+=lw1*lw1; s_lt1[v]+=lt1; sq_lt1[v]+=lt1*lt1; s_dc1[v]+=dc1; sq_dc1[v]+=dc1*dc1;
    s_lw2[v]+=lw2; sq_lw2[v]+=lw2*lw2; s_lt2[v]+=lt2; sq_lt2[v]+=lt2*lt2; s_dc2[v]+=dc2; sq_dc2[v]+=dc2*dc2;
    count[v]++;
}
END {
    for (v in count) { n=count[v];
        print v "," s_lw1[v]/n "," sqrt(abs((sq_lw1[v]/n)-(s_lw1[v]/n)^2)) "," s_lt1[v]/n "," sqrt(abs((sq_lt1[v]/n)-(s_lt1[v]/n)^2)) "," s_dc1[v]/n "," sqrt(abs((sq_dc1[v]/n)-(s_dc1[v]/n)^2)) "," \
                  s_lw2[v]/n "," sqrt(abs((sq_lw2[v]/n)-(s_lw2[v]/n)^2)) "," s_lt2[v]/n "," sqrt(abs((sq_lt2[v]/n)-(s_lt2[v]/n)^2)) "," s_dc2[v]/n "," sqrt(abs((sq_dc2[v]/n)-(s_dc2[v]/n)^2));
    }
}
function abs(x){return x<0?-x:x}' "$INPUT" | sort -n >> "$OUTPUT"
