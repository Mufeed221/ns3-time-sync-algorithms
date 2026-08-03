import matplotlib.pyplot as plt
import csv
from pathlib import Path

# Setup paths
script_dir = Path(__file__).parent
csv_path = script_dir.parent / 'results' / 'processed' / 'energy_stats.csv'
figures_dir = script_dir.parent / 'figures'
figures_dir.mkdir(parents=True, exist_ok=True)

# Read CSV data using built-in csv module
events = []
error_tolerance = []
lw_energy = []
lt_energy = []
dc_energy = []
        
with csv_path.open("r", newline="", encoding="utf-8") as file:
    reader = csv.DictReader(file)

    for row in reader:
        # These fields exist in all five rows.
        events.append(float(row["Avg. events per week"]))
        lw_energy.append(float(row["LW-Sync Consumed Energy (J)"]))

        tolerance_text = row["Error Tolerance (s)"].strip()
        dc_text = row["DC-Sync Consumed Energy (J)"].strip()
        lt_text = row["LT-Sync Consumed Energy (J)"].strip()

        # The fifth row contains only LW-Sync data.
        if tolerance_text and dc_text and lt_text:
            error_tolerance.append(float(tolerance_text))
            dc_energy.append(float(dc_text))
            lt_energy.append(float(lt_text))

print(f"Data loaded: {len(events)} rows")
print(f"Data loaded: {len(error_tolerance)} rows")

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# Plot 1: LW-Sync

bar_width = 0.25
ax1.bar(
    events,
    lw_energy,
    width=bar_width,
    color='b',
    label="LW-Sync",
)

ax1.set_xlabel("Average Events per Week")
ax1.set_ylabel("Weekly Consumed Energy (J)")
ax1.set_title("(a) LW-Sync")
ax1.set_xticks(events)
ax1.legend()
ax1.grid(True, axis="y", alpha=0.3)

# Plot 2: DC-Sync and LT-Sync

bar_width = 0.002
dc_positions = [
    tolerance - bar_width / 2
    for tolerance in error_tolerance
]

lt_positions = [
    tolerance + bar_width / 2
    for tolerance in error_tolerance
]

ax2.bar(
    dc_positions,
    dc_energy,
    width=bar_width,
    label="DC-Sync",
    color='g',
)

ax2.bar(
    lt_positions,
    lt_energy,
    width=bar_width,
    label="LT-Sync",
    color='r',
)

ax2.set_xlabel("Error Tolerance (s)")
ax2.set_ylabel("Weekly Consumed Energy (J)")
ax2.set_title("(b) DC-Sync and LT-Sync")
ax2.set_xticks(error_tolerance)
ax2.set_xticklabels(
    [f"{value:.2f}" for value in error_tolerance]
)
ax2.legend()
ax2.set_yscale('log')
ax2.grid(True, axis="y", alpha=0.3)

plt.tight_layout()

save_path = figures_dir / "energy_plot.png"
plt.savefig(save_path, dpi=300, bbox_inches="tight")

print(f"Figure saved: {save_path}")

plt.show()
