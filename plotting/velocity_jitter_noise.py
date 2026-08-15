import matplotlib.pyplot as plt
import csv
from pathlib import Path

# Setup paths
script_dir = Path(__file__).parent
csv_path_velocity = script_dir.parent / 'results' / 'processed' / 'velocity_noise_stats.csv'
csv_path_jitter = script_dir.parent / 'results' / 'processed' / 'jitter_noise_stats.csv'
figures_dir = script_dir.parent / 'figures'
figures_dir.mkdir(parents=True, exist_ok=True)

# Read CSV data using built-in csv module
velocity_noise = []
t1_lw_avg_v = []
t1_lw_std_v = []
t1_dc_avg_v = []
t1_dc_std_v = []
t2_lw_avg_v = []
t2_lw_std_v = []
t2_dc_avg_v = []
t2_dc_std_v = []

jitter_noise = []
t1_lw_avg_t = []
t1_lw_std_t = []
t1_dc_avg_t = []
t1_dc_std_t = []
t2_lw_avg_t = []
t2_lw_std_t = []
t2_dc_avg_t = []
t2_dc_std_t = []

with open(csv_path_velocity, 'r') as file:
    reader = csv.DictReader(file)
    for row in reader:
        velocity_noise.append(float(row['Vel_Noise']))
        t1_lw_avg_v.append(float(row['T1_LW_Avg']))
        t1_lw_std_v.append(float(row['T1_LW_Std']))
        t1_dc_avg_v.append(float(row['T1_DC_Avg']))
        t1_dc_std_v.append(float(row['T1_DC_Std']))
        t2_lw_avg_v.append(float(row['T2_LW_Avg']))
        t2_lw_std_v.append(float(row['T2_LW_Std']))
        t2_dc_avg_v.append(float(row['T2_DC_Avg']))
        t2_dc_std_v.append(float(row['T2_DC_Std']))

print(f"Data loaded: {len(velocity_noise)} rows")

with open(csv_path_jitter, 'r') as file:
    reader = csv.DictReader(file)
    for row in reader:
        jitter_noise.append(float(row['Jitter']))
        t1_lw_avg_t.append(float(row['T1_LW_Avg']))
        t1_lw_std_t.append(float(row['T1_LW_Std']))
        t1_dc_avg_t.append(float(row['T1_DC_Avg']))
        t1_dc_std_t.append(float(row['T1_DC_Std']))
        t2_lw_avg_t.append(float(row['T2_LW_Avg']))
        t2_lw_std_t.append(float(row['T2_LW_Std']))
        t2_dc_avg_t.append(float(row['T2_DC_Avg']))
        t2_dc_std_t.append(float(row['T2_DC_Std']))

print(f"Data loaded: {len(jitter_noise)} rows")

# Create and save plot
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# Plot averages with error bars
ax1.errorbar(velocity_noise, t1_lw_avg_v, yerr=t1_lw_std_v, fmt='bs-', label='LW-Sync (track1)', linewidth=2, markersize=8, capsize=4)
ax1.errorbar(velocity_noise, t2_lw_avg_v, yerr=t2_lw_std_v, fmt='X-', color='tab:orange', label='LW-Sync (track2)', linewidth=2, markersize=8, capsize=4)
ax1.errorbar(velocity_noise, t1_dc_avg_v, yerr=t1_dc_std_v, fmt='go-', label='DC-Sync (track1)', linewidth=2, markersize=8, capsize=4)
ax1.errorbar(velocity_noise, t2_dc_avg_v, yerr=t2_dc_std_v, fmt='yx-', label='DC-Sync (track2)', linewidth=2, markersize=8, capsize=4)
ax1.set_xlabel('Velocity Error Std. Dev. (m/s)')
ax1.set_ylabel('Offset Error (µs)')
ax1.set_title('(a) Velocity Error')
ax1.legend(loc='best')
ax1.grid(True, alpha=0.3)

ax2.errorbar(jitter_noise, t1_lw_avg_t, yerr=t1_lw_std_t, fmt='bs-', label='LW-Sync (track1)', linewidth=2, markersize=8, capsize=4)
ax2.errorbar(jitter_noise, t2_lw_avg_t, yerr=t2_lw_std_t, fmt='X-', color='tab:orange', label='LW-Sync (track2)', linewidth=2, markersize=8, capsize=4)
ax2.errorbar(jitter_noise, t1_dc_avg_t, yerr=t1_dc_std_t, fmt='go-', label='DC-Sync (track1)', linewidth=2, markersize=8, capsize=4)
ax2.errorbar(jitter_noise, t2_dc_avg_t, yerr=t2_dc_std_t, fmt='yx-', label='DC-Sync (track2)', linewidth=2, markersize=8, capsize=4)
ax2.set_xlabel('Jitter Std. Dev. (µs)')
ax2.set_ylabel('Offset Error (µs)')
ax2.set_title('(b) Jitter Noise')
ax2.legend(loc='best')
ax2.grid(True, alpha=0.3)
ax2.set_xticks(jitter_noise)

plt.tight_layout()

# Save figure
save_path = figures_dir / 'velocity_and_jitter_error_plot.svg'
plt.savefig(save_path, format="svg", bbox_inches="tight")
print(f"Figure saved: {save_path}")

plt.show()
