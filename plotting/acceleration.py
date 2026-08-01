import matplotlib.pyplot as plt
import csv
from pathlib import Path

# Setup paths
script_dir = Path(__file__).parent
csv_path = script_dir.parent / 'results' / 'processed' / 'acceleration_stats.csv'
figures_dir = script_dir.parent / 'figures'
figures_dir.mkdir(parents=True, exist_ok=True)

# Read CSV data using built-in csv module
acceleration = []
lw_avg = []
lt_avg = []
dc_avg = []

with open(csv_path, 'r') as file:
    reader = csv.DictReader(file)
    for row in reader:
        acceleration.append(float(row['Acceleration']))
        lw_avg.append(float(row['LW_Avg']))
        lt_avg.append(float(row['LT_Avg']))
        dc_avg.append(float(row['DC_Avg']))

print(f"Data loaded: {len(acceleration)} rows")

# Create and save plot
fig, ax1 = plt.subplots(figsize=(14, 6))

# Plot averages
ax1.plot(acceleration, lt_avg, 'r^-', label='LT-Sync', linewidth=2, markersize=8)
ax1.plot(acceleration, lw_avg, 'bs-', label='LW-Sync', linewidth=2, markersize=8)
ax1.plot(acceleration, dc_avg, 'go-', label='DC-Sync', linewidth=2, markersize=8)
ax1.set_xlabel('Acceleration (m/s2)')
ax1.set_ylabel('Offset Error (µs)')
ax1.set_title('Acceleration Analysis')
ax1.legend(loc='best')
ax1.grid(True, alpha=0.3)
ax1.set_xticks(acceleration)
plt.tight_layout()

# Save figure
save_path = figures_dir / 'acceleration plot.png'
plt.savefig(save_path, dpi=300, bbox_inches='tight')
print(f"Figure saved: {save_path}")

plt.show()
