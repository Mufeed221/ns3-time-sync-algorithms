import matplotlib.pyplot as plt
import csv
from pathlib import Path

# Setup paths
script_dir = Path(__file__).parent
csv_path = script_dir.parent / 'results' / 'processed' / 'distance_stats.csv'
figures_dir = script_dir.parent / 'figures'
figures_dir.mkdir(parents=True, exist_ok=True)

# Read CSV data using built-in csv module
distance = []
t1_lw_avg = []
t1_lt_avg = []
t1_dc_avg = []
t2_lw_avg = []
t2_lt_avg = []
t2_dc_avg = []

with open(csv_path, 'r') as file:
    reader = csv.DictReader(file)
    for row in reader:
        distance.append(float(row['Distance']))
        t1_lw_avg.append(float(row['T1_LW_Avg']))
        t1_lt_avg.append(float(row['T1_LT_Avg']))
        t1_dc_avg.append(float(row['T1_DC_Avg']))
        t2_lw_avg.append(float(row['T2_LW_Avg']))
        t2_lt_avg.append(float(row['T2_LT_Avg']))
        t2_dc_avg.append(float(row['T2_DC_Avg']))

print(f"Data loaded: {len(distance)} rows")

# Create and save plot
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# Plot averages
ax1.plot(distance, t1_lt_avg, 'r^-', label='LT-Sync', linewidth=2, markersize=8)
ax1.plot(distance, t1_lw_avg, 'bs-', label='LW-Sync', linewidth=2, markersize=8)
ax1.plot(distance, t1_dc_avg, 'go-', label='DC-Sync', linewidth=2, markersize=8)
ax1.set_xlabel('Initial Distance (m)')
ax1.set_ylabel('Offset Error (µs)')
ax1.set_title('(a) Track 1')
ax1.legend(loc='best')
ax1.grid(True, alpha=0.3)
ax1.set_xticks(distance)

# Plot averages
ax2.plot(distance, t2_lt_avg, 'r^-', label='LT-Sync', linewidth=2, markersize=8)
ax2.plot(distance, t2_lw_avg, 'bs-', label='LW-Sync', linewidth=2, markersize=8)
ax2.plot(distance, t2_dc_avg, 'go-', label='DC-Sync', linewidth=2, markersize=8)
ax2.set_xlabel('Initial Distance (m)')
ax2.set_ylabel('Offset Error (µs)')
ax2.set_title('(b) Track 2')
ax2.legend(loc='best')
ax2.grid(True, alpha=0.3)
ax2.set_xticks(distance)

plt.tight_layout()

# Save figure
save_path = figures_dir / 'distance_plot.svg'
plt.savefig(save_path, format="svg", bbox_inches="tight")
print(f"Figure saved: {save_path}")

plt.show()
