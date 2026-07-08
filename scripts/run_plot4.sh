#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NS3_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$NS3_DIR" || exit 1

idx=1
for a in 0.01 0.03 0.05 0.07 0.1
do
    for i in {1..500}
    do
	./ns3 run "scratch/LW-Sync --run=$i --acceleration=$a --plotNum=4 --track1=true --track2=false"
	./ns3 run "scratch/LT-Sync --run=$i --acceleration=$a --plotNum=4 --track1=true --track2=false"
	./ns3 run "scratch/DC-Sync --run=$i --acceleration=$a --plotNum=4 --track1=true --track2=false --totalRuns=500 --totalVariations=5 --currentVariationIndex=$idx"
    done
    idx=$((idx + 1))
done
bash "$NS3_DIR/csv/calculate_plot4.sh"
