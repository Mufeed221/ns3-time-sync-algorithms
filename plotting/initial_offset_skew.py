import matplotlib.pyplot as plt
import csv
from pathlib import Path

# Setup paths
script_dir = Path(__file__).parent
csv_path_offset = script_dir.parent / 'results' / 'processed' / 'initial_offset_stats.csv'
csv_path_skew = script_dir.parent / 'results' / 'processed' / 'initial_skew_stats.csv'
figures_dir = script_dir.parent / 'figures'
figures_dir.mkdir(parents=True, exist_ok=True)

# Read CSV data using built-in csv module
clock_offset = []
o_lw_avg = []
o_lt_avg = []
o_dc_avg = []

clock_skew = []
s_lw_avg = []
s_lt_avg = []
s_dc_avg = []

with open(csv_path_offset, 'r') as file:
    reader = csv.DictReader(file)
    for row in reader:
        clock_offset.append(float(row['ClockOffset']))
        o_lw_avg.append(float(row['T1_LW_Avg']))
        o_lt_avg.append(float(row['T1_LT_Avg']))
        o_dc_avg.append(float(row['T1_DC_Avg']))

print(f"Data loaded: {len(clock_offset)} rows")

with open(csv_path_skew, 'r') as file:
    reader = csv.DictReader(file)
    for row in reader:
        clock_skew.append(float(row['ClockSkew']))
        s_lw_avg.append(float(row['T1_LW_Avg']))
        s_lt_avg.append(float(row['T1_LT_Avg']))
        s_dc_avg.append(float(row['T1_DC_Avg']))

print(f"Data loaded: {len(clock_skew)} rows")

# Create and save plot
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# Plot averages
ax1.plot(clock_offset, o_lt_avg, 'r^-', label='LT-Sync', linewidth=2, markersize=8)
ax1.plot(clock_offset, o_lw_avg, 'bs-', label='LW-Sync', linewidth=2, markersize=8)
ax1.plot(clock_offset, o_dc_avg, 'go-', label='DC-Sync', linewidth=2, markersize=8)
ax1.set_xlabel('Initial Offset (s)')
ax1.set_ylabel('Offset Error (µs)')
ax1.set_title('(a) Initial Offset')
ax1.legend(loc='best')
ax1.grid(True, alpha=0.3)
ax1.set_yscale('log')
plt.tight_layout()

ax2.plot(clock_skew, s_lt_avg, 'r^-', label='LT-Sync', linewidth=2, markersize=8)
ax2.plot(clock_skew, s_lw_avg, 'bs-', label='LW-Sync', linewidth=2, markersize=8)
ax2.plot(clock_skew, s_dc_avg, 'go-', label='DC-Sync', linewidth=2, markersize=8)
ax2.set_xlabel('Initial Skew (ppm)')
ax2.set_ylabel('Offset Error (µs)')
ax2.set_title('(b) Initial Skew')
ax2.legend(loc='best')
ax2.grid(True, alpha=0.3)
ax2.set_yscale('log')
plt.tight_layout()

# Save figure
save_path = figures_dir / 'initial_offset_and_skew_plot.svg'
plt.savefig(save_path, format="svg", bbox_inches="tight")
print(f"Figure saved: {save_path}")

plt.show()
