#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NS3_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$NS3_DIR" || exit 1

idx=1
for o in 0.02 0.05 1 5 10 20
do
    for i in {1..500}
    do
	./ns3 run "scratch/LW-Sync --run=$i --clockOffset=$o --plotNum=7 --track1=true --track2=false"
	./ns3 run "scratch/LT-Sync --run=$i --clockOffset=$o --plotNum=7 --track1=true --track2=false"
	./ns3 run "scratch/DC-Sync --run=$i --clockOffset=$o --plotNum=7 --track1=true --track2=false --totalRuns=500 --totalVariations=6 --currentVariationIndex=$idx"
	
	./ns3 run "scratch/LW-Sync --run=$i --clockOffset=$o --plotNum=7 --track1=false --track2=true"
	./ns3 run "scratch/LT-Sync --run=$i --clockOffset=$o --plotNum=7 --track1=false --track2=true"
	./ns3 run "scratch/DC-Sync --run=$i --clockOffset=$o --plotNum=7 --track1=false --track2=true --totalRuns=500 --totalVariations=6 --currentVariationIndex=$idx"
    done
    idx=$((idx + 1))
done
bash "$NS3_DIR/csv/calculate_plot7.sh"
