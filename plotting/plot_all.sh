#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

python acceleration.py
python starting_angle.py
python distance.py
python initial_offset_skew.py
python messages_count.py
python UWGS_position_noise.py
python UWGS_velocity.py
python velocity.py
python velocity_noise.py
python energy.py
python error_bound.py
