#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd "$SCRIPT_DIR" || exit 1

./run_distance.sh
./run_starting_angle.sh
./run_acceleration.sh
./run_velocity.sh
./run_velocity_noise.sh
./run_jitter_noise.sh
./run_initial_offset.sh
./run_initial_skew.sh
./run_message_count.sh
./run_uwgs_velocity.sh
./run_uwgs_position_noise.sh
./run_energy.sh
