import matplotlib.pyplot as plt
import csv
from pathlib import Path

# Setup paths
script_dir = Path(__file__).parent
csv_path = script_dir.parent / 'results' / 'processed' / 'jitter_noise_stats.csv'
figures_dir = script_dir.parent / 'figures'
figures_dir.mkdir(parents=True, exist_ok=True)

# Read CSV data using built-in csv module
jitter_noise = []
t1_lw_avg = []
t1_lw_std = []
t1_dc_avg = []
t1_dc_std = []
t2_lw_avg = []
t2_lw_std = []
t2_dc_avg = []
t2_dc_std = []

with open(csv_path, 'r') as file:
    reader = csv.DictReader(file)
    for row in reader:
        jitter_noise.append(float(row['Jitter']))
        t1_lw_avg.append(float(row['T1_LW_Avg']))
        t1_lw_std.append(float(row['T1_LW_Std']))
        t1_dc_avg.append(float(row['T1_DC_Avg']))
        t1_dc_std.append(float(row['T1_DC_Std']))
        t2_lw_avg.append(float(row['T2_LW_Avg']))
        t2_lw_std.append(float(row['T2_LW_Std']))
        t2_dc_avg.append(float(row['T2_DC_Avg']))
        t2_dc_std.append(float(row['T2_DC_Std']))

print(f"Data loaded: {len(jitter_noise)} rows")

# Create and save plot
fig, ax1 = plt.subplots(figsize=(14, 6))

# Plot averages with error bars
ax1.errorbar(jitter_noise, t1_lw_avg, yerr=t1_lw_std, fmt='bs-', label='LW-Sync (track1)', linewidth=2, markersize=8, capsize=4)
ax1.errorbar(jitter_noise, t2_lw_avg, yerr=t2_lw_std, fmt='X-', color='tab:orange', label='LW-Sync (track2)', linewidth=2, markersize=8, capsize=4)
ax1.errorbar(jitter_noise, t1_dc_avg, yerr=t1_dc_std, fmt='go-', label='DC-Sync (track1)', linewidth=2, markersize=8, capsize=4)
ax1.errorbar(jitter_noise, t2_dc_avg, yerr=t2_dc_std, fmt='yx-', label='DC-Sync (track2)', linewidth=2, markersize=8, capsize=4)

ax1.set_xlabel('Jitter Std. Dev. (µs)')
ax1.set_ylabel('Offset Error (µs)')
ax1.set_title('Jitter Noise Analysis')
ax1.legend(loc='best')
ax1.grid(True, alpha=0.3)
ax1.set_xticks(jitter_noise)
plt.tight_layout()

# Save figure
save_path = figures_dir / 'jitter noise plot.png'
plt.savefig(save_path, dpi=300, bbox_inches='tight')
print(f"Figure saved: {save_path}")

plt.show()
