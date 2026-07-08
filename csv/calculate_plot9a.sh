#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NS3_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

INPUT="$SCRIPT_DIR/plot9a.csv"
OUTPUT="$NS3_DIR/results/plot9a_stats(Beacon Count).csv"

echo "BeaconCount,T1_LWSkew_Avg,T1_LWSkew_Std,T2_LWSkew_Avg,T2_LWSkew_Std" > "$OUTPUT"
awk -F, 'NR > 2 && NF >= 4 {
    v=$1; lw1=$2; lw2=$4;
    s1[v]+=lw1; sq1[v]+=lw1*lw1;
    s2[v]+=lw2; sq2[v]+=lw2*lw2;
    count[v]++;
}
END {
    for (v in count) { n=count[v];
        print v "," s1[v]/n "," sqrt(abs((sq1[v]/n)-(s1[v]/n)^2)) "," \
                  s2[v]/n "," sqrt(abs((sq2[v]/n)-(s2[v]/n)^2));
    }
}
function abs(x){return x<0?-x:x}' "$INPUT" | sort -n >> "$OUTPUT"
