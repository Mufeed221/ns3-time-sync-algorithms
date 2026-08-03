import matplotlib.pyplot as plt
import csv
from pathlib import Path

# Setup paths
script_dir = Path(__file__).parent
csv_path_dc = script_dir.parent / 'results' / 'processed' / 'DC_message_count_stats.csv'
csv_path_lw = script_dir.parent / 'results' / 'processed' / 'LW_beacon_count_stats.csv'
figures_dir = script_dir.parent / 'figures'
figures_dir.mkdir(parents=True, exist_ok=True)

# Read CSV data using built-in csv module
message_count = []
t1_dc_avg = []
t2_dc_avg = []

beacon_count = []
t1_lw_avg = []
t1_lw_std = []
t2_lw_avg = []
t2_lw_std = []

with open(csv_path_dc, 'r') as file:
    reader = csv.DictReader(file)
    for row in reader:
        message_count.append(float(row['MsgCount']))
        t1_dc_avg.append(float(row['T1_DCSkew_Avg']))
        t2_dc_avg.append(float(row['T2_DCSkew_Avg']))

print(f"Data loaded: {len(message_count)} rows")

with open(csv_path_lw, 'r') as file:
    reader = csv.DictReader(file)
    for row in reader:
        beacon_count.append(float(row['MsgCount']))
        t1_lw_avg.append(float(row['T1_LWSkew_Avg']))
        t1_lw_std.append(float(row['T1_LWSkew_Std']))
        t2_lw_avg.append(float(row['T2_LWSkew_Avg']))
        t2_lw_std.append(float(row['T2_LWSkew_Std']))

print(f"Data loaded: {len(beacon_count)} rows")

# Create and save plot
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# Plot averages
ax1.errorbar(message_count, t1_dc_avg, fmt='go-', label='DC-Sync (track1)', linewidth=2, markersize=8, capsize=4)
ax1.errorbar(message_count, t2_dc_avg, fmt='yx-', label='DC-Sync (track2)', linewidth=2, markersize=8, capsize=4)
ax1.set_xlabel('Two-way Message Exchange Pairs')
ax1.set_ylabel('Skew Error (ppm)')
ax1.set_title('(a) DC-Sync')
ax1.legend(loc='best')
ax1.grid(True, alpha=0.3)
ax1.set_yscale('log')
ax1.set_xticks(message_count)
plt.tight_layout()

ax2.errorbar(beacon_count, t1_lw_avg, yerr=t1_lw_std, fmt='bs-', label='LW-Sync (track1)', linewidth=2, markersize=8, capsize=4)
ax2.errorbar(beacon_count, t2_lw_avg, yerr=t2_lw_std, fmt='X-', color='tab:orange', label='LW-Sync (track2)', linewidth=2, markersize=8, capsize=4)
ax2.set_xlabel('One-way Beacons')
ax2.set_ylabel('Skew Error (ppm)')
ax2.set_title('(b) LW-Sync')
ax2.legend(loc='best')
ax2.grid(True, alpha=0.3)
ax2.set_xticks(beacon_count)
plt.tight_layout()

# Save figure
save_path = figures_dir / 'messages_count_plot.png'
plt.savefig(save_path, dpi=300, bbox_inches='tight')
print(f"Figure saved: {save_path}")

plt.show()
