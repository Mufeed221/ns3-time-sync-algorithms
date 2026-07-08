#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NS3_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$NS3_DIR" || exit 1

idx=1
for tn in 10e-6 20e-6 30e-6 40e-6 50e-6
do
    for i in {1..500}
    do
    	./ns3 run "scratch/LW-Sync --run=$i --timeError=$tn --plotNum=6 --track1=true --track2=false"
	./ns3 run "scratch/LT-Sync --run=$i --timeError=$tn --plotNum=6 --track1=true --track2=false"
	./ns3 run "scratch/DC-Sync --run=$i --timeError=$tn --plotNum=6 --track1=true --track2=false --totalRuns=500 --totalVariations=5 --currentVariationIndex=$idx"
	
	./ns3 run "scratch/LW-Sync --run=$i --timeError=$tn --plotNum=6 --track1=false --track2=true"
	./ns3 run "scratch/LT-Sync --run=$i --timeError=$tn --plotNum=6 --track1=false --track2=true"
	./ns3 run "scratch/DC-Sync --run=$i --timeError=$tn --plotNum=6 --track1=false --track2=true --totalRuns=500 --totalVariations=5 --currentVariationIndex=$idx"
    done
    idx=$((idx + 1))
done
bash "$NS3_DIR/csv/calculate_plot6.sh"
