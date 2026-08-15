import matplotlib.pyplot as plt
import csv
from pathlib import Path

# Setup paths
script_dir = Path(__file__).parent
csv_path_offset = script_dir.parent / 'results' / 'processed' / 'UWGS_position_noise_(offset)_stats.csv'
csv_path_skew = script_dir.parent / 'results' / 'processed' / 'UWGS_position_noise_(skew)_stats.csv'
figures_dir = script_dir.parent / 'figures'
figures_dir.mkdir(parents=True, exist_ok=True)

# Read CSV data using built-in csv module
noise1 = []
o_lw_avg = []
o_uwgs_avg = []

noise2 = []
s_lw_avg = []
s_uwgs_avg = []

with open(csv_path_offset, 'r') as file:
    reader = csv.DictReader(file)
    for row in reader:
        noise1.append(float(row['TrajectoryPositionNoise']))
        o_lw_avg.append(float(row['LW_Avg']))
        o_uwgs_avg.append(float(row['UWGS_Avg']))

print(f"Data loaded: {len(noise1)} rows")

with open(csv_path_skew, 'r') as file:
    reader = csv.DictReader(file)
    for row in reader:
        noise2.append(float(row['TrajectoryPositionNoise']))
        s_lw_avg.append(float(row['LW_Avg']))
        s_uwgs_avg.append(float(row['UWGS_Avg']))

print(f"Data loaded: {len(noise2)} rows")

# Create and save plot
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# Plot averages
ax1.plot(noise1, o_lw_avg, 'bs-', label='LW-Sync', linewidth=2, markersize=8)
ax1.plot(noise1, o_uwgs_avg, 'c^-', label='UWGS', linewidth=2, markersize=8)
ax1.set_xlabel('Trajectory-position noise Std. Dev. (m)')
ax1.set_ylabel('Offset Error (µs)')
ax1.set_title('(a) Mean Offset Error')
ax1.legend(loc='best')
ax1.grid(True, alpha=0.3)
ax1.set_xticks(noise1)
plt.tight_layout()

ax2.plot(noise2, s_lw_avg, 'bs-', label='LW-Sync', linewidth=2, markersize=8)
ax2.plot(noise2, s_uwgs_avg, 'c^-', label='UWGS', linewidth=2, markersize=8)
ax2.set_xlabel('Trajectory-position noise Std. Dev. (m)')
ax2.set_ylabel('Skew Error (ppm)')
ax2.set_title('(b) Mean Skew Error')
ax2.legend(loc='best')
ax2.grid(True, alpha=0.3)
ax2.set_xticks(noise2)
plt.tight_layout()

# Save figure
save_path = figures_dir / 'UWGS_position_noise_plot.svg'
plt.savefig(save_path, format="svg", bbox_inches="tight")
print(f"Figure saved: {save_path}")

plt.show()
