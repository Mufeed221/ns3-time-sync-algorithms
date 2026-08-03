#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

python distance.py
python velocity_acceleration.py
python starting_angle.py
python initial_offset_skew.py
python messages_count.py
python UWGS_position_noise.py
python UWGS_velocity.py
python velocity_jitter_noise.py
python energy.py
python error_bound.py
