#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NS3_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$NS3_DIR" || exit 1

idx=1
for a in 0 1 2 3 4 5 6 7
do
    for i in {1..500}
    do
	./ns3 run "scratch/LW-Sync --run=$i --angleIndex=$a --plotNum=2 --track1=false --track2=true"
	./ns3 run "scratch/LT-Sync --run=$i --angleIndex=$a --plotNum=2 --track1=false --track2=true"
	./ns3 run "scratch/DC-Sync --run=$i --angleIndex=$a --plotNum=2 --track1=false --track2=true --totalRuns=500 --totalVariations=8 --currentVariationIndex=$idx"
    done
    idx=$((idx + 1))
done
bash "$NS3_DIR/csv/calculate_plot2.sh"
