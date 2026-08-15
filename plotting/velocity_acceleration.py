import matplotlib.pyplot as plt
import csv
from pathlib import Path

# Setup paths
script_dir = Path(__file__).parent
csv_path_velocity = script_dir.parent / 'results' / 'processed' / 'velocity_stats.csv'
csv_path_acceleration = script_dir.parent / 'results' / 'processed' / 'acceleration_stats.csv'
figures_dir = script_dir.parent / 'figures'
figures_dir.mkdir(parents=True, exist_ok=True)

# Read CSV data using built-in csv module
velocity = []
lw_avg_v = []
lt_avg_v = []
dc_avg_v = []

acceleration = []
lw_avg_a = []
lt_avg_a = []
dc_avg_a = []

with open(csv_path_velocity, 'r') as file:
    reader = csv.DictReader(file)
    for row in reader:
        velocity.append(float(row['Init. Radial Velocity']))
        lw_avg_v.append(float(row['LW_Avg']))
        lt_avg_v.append(float(row['LT_Avg']))
        dc_avg_v.append(float(row['DC_Avg']))

print(f"Data loaded: {len(velocity)} rows")

with open(csv_path_acceleration, 'r') as file:
    reader = csv.DictReader(file)
    for row in reader:
        acceleration.append(float(row['Acceleration']))
        lw_avg_a.append(float(row['LW_Avg']))
        lt_avg_a.append(float(row['LT_Avg']))
        dc_avg_a.append(float(row['DC_Avg']))

print(f"Data loaded: {len(acceleration)} rows")

# Create and save plot
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# Plot averages
ax1.plot(velocity, lt_avg_v, 'r^-', label='LT-Sync', linewidth=2, markersize=8)
ax1.plot(velocity, lw_avg_v, 'bs-', label='LW-Sync', linewidth=2, markersize=8)
ax1.plot(velocity, dc_avg_v, 'go-', label='DC-Sync', linewidth=2, markersize=8)
ax1.set_xlabel('Initial Velocity (m/s)')
ax1.set_ylabel('Offset Error (µs)')
ax1.set_title('(a) Initial Velocity')
ax1.legend(loc='best')
ax1.grid(True, alpha=0.3)
ax1.set_xticks(velocity)

ax2.plot(acceleration, lt_avg_a, 'r^-', label='LT-Sync', linewidth=2, markersize=8)
ax2.plot(acceleration, lw_avg_a, 'bs-', label='LW-Sync', linewidth=2, markersize=8)
ax2.plot(acceleration, dc_avg_a, 'go-', label='DC-Sync', linewidth=2, markersize=8)
ax2.set_xlabel('Acceleration (m/s2)')
ax2.set_ylabel('Offset Error (µs)')
ax2.set_title('(b) Acceleration')
ax2.legend(loc='best')
ax2.grid(True, alpha=0.3)
ax2.set_xticks(acceleration)

plt.tight_layout()

# Save figure
save_path = figures_dir / 'velocity_and_acceleration_plot.svg'
plt.savefig(save_path, format="svg", bbox_inches="tight")
print(f"Figure saved: {save_path}")

plt.show()
