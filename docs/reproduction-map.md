# Reproduction Map

This file maps manuscript results to the corresponding simulation source, batch script, raw data, processed data, and plotting script.

Update the figure and table numbers after the manuscript layout is finalized.

| Manuscript result | Simulation source | Batch script | Raw data | Processed data | Plotting script |
|---|---|---|---|---|---|
| Offset error vs. initial distance | `scratch/LW-Sync.cc`, `LT-Sync.cc`, `DC-Sync.cc` | `scripts/run_distance.sh` | `results/raw/distance_long.csv` | `results/processed/distance_stats.csv` | `plotting/distance.py` |
| Offset error vs. starting angle | `scratch/LW-Sync.cc`, `LT-Sync.cc`, `DC-Sync.cc` | `scripts/run_starting_angle.sh` | `results/raw/starting_angle_long.csv` | `results/processed/starting_angle_stats.csv` | `plotting/starting_angle.py` |
| Offset error vs. initial velocity | `scratch/LW-Sync.cc`, `LT-Sync.cc`, `DC-Sync.cc` | `scripts/run_velocity.sh` | `results/raw/velocity_long.csv` | `results/processed/velocity_stats.csv` | `plotting/velocity_acceletaion.py` |
| Offset error vs. acceleration | `scratch/LW-Sync.cc`, `LT-Sync.cc`, `DC-Sync.cc` | `scripts/run_acceleration.sh` | `results/raw/acceleration_long.csv` | `results/processed/acceleration_stats.csv` | `plotting/velocity_acceletaion.py` |
| Velocity-noise sensitivity | `scratch/LW-Sync.cc`, `LT-Sync.cc`, `DC-Sync.cc` | `scripts/run_velocity_noise.sh` | `results/raw/velocity_noise_long.csv` | `results/processed/velocity_noise_stats.csv` | `plotting/velocity_jitter_noise.py` |
| Timestamp-jitter sensitivity | `scratch/LW-Sync.cc`, `LT-Sync.cc`, `DC-Sync.cc` | `scripts/run_jitter_noise.sh` | `results/raw/jitter_noise_long.csv` | `results/processed/jitter_noise_stats.csv` | `plotting/velocity_jitter_noise.py` |
| Initial-offset sensitivity | `scratch/LW-Sync.cc`, `LT-Sync.cc`, `DC-Sync.cc` | `scripts/run_initial_offset.sh` | `results/raw/initial_offset_long.csv` | `results/processed/initial_offset_stats.csv` | `plotting/initial_offset_skew.py` |
| Initial-skew sensitivity | `scratch/LW-Sync.cc`, `LT-Sync.cc`, `DC-Sync.cc` | `scripts/run_initial_skew.sh` | `results/raw/initial_skew_long.csv` | `results/processed/initial_skew_stats.csv` | `plotting/initial_offset_skew.py` |
| skew vs. message count | `scratch/LW-Sync.cc`, `LT-Sync.cc`, `DC-Sync.cc` | `scripts/run_message_count.sh` | `results/raw/message_count_long.csv` | `results/processed/DC_message_count_stats.csv`, `results/processed/LT_skew_estimate_stats.csv`, `results/processed/LW_beacon_count_stats.csv` | `plotting/messages_count.py` |
| UWGS/LW-Sync offset vs. velocity | `scratch/UWGS.cc`, `LW-Sync.cc` | `scripts/run_uwgs_velocity.sh` | `results/raw/UWGS_velocity_long.csv` | `results/processed/UWGS_velocity_(offset)_stats.csv` | `plotting/UWGS_velocity.py` |
| UWGS/LW-Sync skew vs. velocity | `scratch/UWGS.cc`, `LW-Sync.cc` | `scripts/run_uwgs_velocity.sh` | `results/raw/UWGS_velocity_long.csv` | `results/processed/UWGS_velocity_(skew)_stats.csv` | `plotting/UWGS_velocity.py` |
| UWGS/LW-Sync offset vs. position noise | `scratch/UWGS.cc`, `LW-Sync.cc` | `scripts/run_uwgs_position_noise.sh` | `results/raw/UWGS_position_noise_long.csv` | `results/processed/UWGS_position_noise_(offset)_stats.csv` | `plotting/UWGS_position_noise.py` |
| UWGS/LW-Sync skew vs. position noise | `scratch/UWGS.cc`, `LW-Sync.cc` | `scripts/run_uwgs_position_noise.sh` | `results/raw/UWGS_position_noise_long.csv` | `results/processed/UWGS_position_noise_(skew)_stats.csv` | `plotting/UWGS_position_noise.py` |
| Energy per synchronization cycle |  `scratch/LW-Sync.cc`, `LT-Sync.cc`, `DC-Sync.cc` | `scripts/run_energy.sh` | `results/raw/energy.csv` | `results/processed/energy_stats.csv` | `plotting/energy.py` |
