import matplotlib.pyplot as plt
import csv
from pathlib import Path

# Setup paths
script_dir = Path(__file__).parent
csv_path_offset = script_dir.parent / 'results' / 'processed' / 'UWGS_velocity_(offset)_stats.csv'
csv_path_skew = script_dir.parent / 'results' / 'processed' / 'UWGS_velocity_(skew)_stats.csv'
figures_dir = script_dir.parent / 'figures'
figures_dir.mkdir(parents=True, exist_ok=True)

# Read CSV data using built-in csv module
velocity1 = []
o_lw_avg = []
o_uwgs_avg = []

velocity2 = []
s_lw_avg = []
s_uwgs_avg = []

with open(csv_path_offset, 'r') as file:
    reader = csv.DictReader(file)
    for row in reader:
        velocity1.append(float(row['Velocity']))
        o_lw_avg.append(float(row['LW_Avg']))
        o_uwgs_avg.append(float(row['UWGS_Avg']))

print(f"Data loaded: {len(velocity1)} rows")

with open(csv_path_skew, 'r') as file:
    reader = csv.DictReader(file)
    for row in reader:
        velocity2.append(float(row['Velocity']))
        s_lw_avg.append(float(row['LW_Avg']))
        s_uwgs_avg.append(float(row['UWGS_Avg']))

print(f"Data loaded: {len(velocity2)} rows")

# Create and save plot
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# Plot averages
ax1.plot(velocity1, o_lw_avg, 'bs-', label='LW-Sync', linewidth=2, markersize=8)
ax1.plot(velocity1, o_uwgs_avg, 'c^-', label='UWGS', linewidth=2, markersize=8)
ax1.set_xlabel('Velocity (m/s)')
ax1.set_ylabel('Offset Error (µs)')
ax1.set_title('Mean Offset Error')
ax1.legend(loc='best')
ax1.grid(True, alpha=0.3)
ax1.set_xticks(velocity1)
plt.tight_layout()

ax2.plot(velocity2, s_lw_avg, 'bs-', label='LW-Sync', linewidth=2, markersize=8)
ax2.plot(velocity2, s_uwgs_avg, 'c^-', label='UWGS', linewidth=2, markersize=8)
ax2.set_xlabel('Velocity (m/s)')
ax2.set_ylabel('Skew Error (ppm)')
ax2.set_title('Mean Skew Error')
ax2.legend(loc='best')
ax2.grid(True, alpha=0.3)
ax2.set_xticks(velocity2)
plt.tight_layout()

# Save figure
save_path = figures_dir / 'UWGS velocity plot.png'
plt.savefig(save_path, dpi=300, bbox_inches='tight')
print(f"Figure saved: {save_path}")

plt.show()
