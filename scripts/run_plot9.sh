#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NS3_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$NS3_DIR" || exit 1

idx=1
for m in 10 15 20 25 30
do
    for i in {1..500}
    do
	./ns3 run "scratch/LW-Sync --run=$i --beaconCount=$m --plotNum=9 --track1=true --track2=false"
	./ns3 run "scratch/DC-Sync --run=$i --messagePairsCount=$m --plotNum=9 --track1=true --track2=false --totalRuns=500 --totalVariations=5 --currentVariationIndex=$idx"
	
	./ns3 run "scratch/LW-Sync --run=$i --beaconCount=$m --plotNum=9 --track1=false --track2=true"
	./ns3 run "scratch/DC-Sync --run=$i --messagePairsCount=$m --plotNum=9 --track1=false --track2=true --totalRuns=500 --totalVariations=5 --currentVariationIndex=$idx"
    done
    idx=$((idx + 1))
done

for i in {1..500}
do
	./ns3 run "scratch/LT-Sync --run=$i --plotNum=9 --track1=true --track2=false"
	./ns3 run "scratch/LT-Sync --run=$i --plotNum=9 --track1=false --track2=true"
done
bash "$NS3_DIR/csv/calculate_plot9a.sh"
bash "$NS3_DIR/csv/calculate_plot9b.sh"
bash "$NS3_DIR/csv/calculate_plot9c.sh"
