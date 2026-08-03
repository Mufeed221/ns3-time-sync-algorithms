import csv
from pathlib import Path

import matplotlib.pyplot as plt


# ============================================================
# Paths
# ============================================================

SCRIPT_DIR = Path(__file__).resolve().parent
RESULTS_DIR = SCRIPT_DIR.parent / "results" / "processed"
FIGURES_DIR = SCRIPT_DIR.parent / "figures"

FIGURES_DIR.mkdir(parents=True, exist_ok=True)


# ============================================================
# Default analytical parameters
# ============================================================

DEFAULT_CLOCK_SKEW = 200e-6   # 200 ppm as fractional skew
SOUND_SPEED = 1500.0          # m/s
REPLY_TIME = 0.024            # s

DEFAULT_DISTANCE = 500.0      # m
DEFAULT_VELOCITY_ERROR = 0.2  # m/s
DEFAULT_JITTER = 20e-6        # s


# ============================================================
# Analytical bound
# ============================================================

def analytical_bound_us(distance_m, velocity_error_mps, jitter_s, alpha):
    tau = distance_m / SOUND_SPEED

    bound_s = (
        alpha * (2.0 * tau + 0.5 * REPLY_TIME)
        + (velocity_error_mps / (2.0 * SOUND_SPEED))
          * (((1.0 - 2.0 * alpha) * tau) + REPLY_TIME)
        + (1.0 + alpha) * jitter_s
    )

    return bound_s * 1e6


# ============================================================
# CSV helper
# ============================================================

def read_columns(csv_path, x_column, mean_column, std_column):
    x_values = []
    means = []
    stds = []

    with csv_path.open("r", newline="", encoding="utf-8") as file:
        reader = csv.DictReader(file)

        for row in reader:
            x_text = row[x_column].strip()
            mean_text = row[mean_column].strip()
            std_text = row[std_column].strip()

            if not x_text or not mean_text or not std_text:
                continue

            if mean_text == "NA" or std_text == "NA":
                continue

            x_values.append(float(x_text))
            means.append(float(mean_text))
            stds.append(float(std_text))

    ordered = sorted(zip(x_values, means, stds), key=lambda t: t[0])
    x_values, means, stds = map(list, zip(*ordered))
    return x_values, means, stds


# ============================================================
# Plot helper
# ============================================================

def draw_subplot(
    ax,
    x_values,
    means,
    stds,
    analytical_bounds,
    xlabel,
    subtitle,
):
    # Analytical bound in gray
    ax.plot(
        x_values,
        analytical_bounds,
        marker="s",
        linewidth=1.8,
        color="gray",
        label="Analytical Error Bound",
        zorder=1,
    )

    # Simulated mean ± sigma
    ax.errorbar(
        x_values,
        means,
        yerr=stds,
        marker="o",
        linestyle="-",
        capsize=6,
        elinewidth=1.6,
        capthick=1.6,
        linewidth=1.8,
        label=r"Simulated mean $\pm \sigma$",
        zorder=3,
    )

    ax.set_xlabel(xlabel)
    ax.set_ylabel(r"Offset Error ($\mu$s)")
    ax.set_title(subtitle)
    ax.grid(True, alpha=0.3)
    ax.legend()


# ============================================================
# Combined 2x2 figure
# ============================================================

def main():
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    axes = axes.flatten()

    # --------------------------------------------------------
    # (a) Distance
    # --------------------------------------------------------
    distance_csv = RESULTS_DIR / "distance_stats.csv"

    distances, distance_means, distance_stds = read_columns(
        distance_csv,
        x_column="Distance",
        mean_column="T1_LW_Avg",
        std_column="T1_LW_Std",
    )

    distance_bounds = [
        analytical_bound_us(
            distance_m=d,
            velocity_error_mps=DEFAULT_VELOCITY_ERROR,
            jitter_s=DEFAULT_JITTER,
            alpha=DEFAULT_CLOCK_SKEW,
        )
        for d in distances
    ]

    draw_subplot(
        axes[0],
        distances,
        distance_means,
        distance_stds,
        distance_bounds,
        xlabel="Initial Distance (m)",
        subtitle="(a) Initial Distance",
    )

    # --------------------------------------------------------
    # (b) Jitter
    # --------------------------------------------------------
    jitter_csv = RESULTS_DIR / "jitter_noise_stats.csv"

    jitter_us, jitter_means, jitter_stds = read_columns(
        jitter_csv,
        x_column="Jitter",
        mean_column="T1_LW_Avg",
        std_column="T1_LW_Std",
    )

    jitter_bounds = [
        analytical_bound_us(
            distance_m=DEFAULT_DISTANCE,
            velocity_error_mps=DEFAULT_VELOCITY_ERROR,
            jitter_s=j * 1e-6,   # CSV is in microseconds
            alpha=DEFAULT_CLOCK_SKEW,
        )
        for j in jitter_us
    ]

    draw_subplot(
        axes[1],
        jitter_us,
        jitter_means,
        jitter_stds,
        jitter_bounds,
        xlabel=r"Jitter Std. Dev. (µs)",
        subtitle="(b) Jitter Std. Dev.",
    )

    # --------------------------------------------------------
    # (c) Velocity noise
    # --------------------------------------------------------
    velocity_noise_csv = RESULTS_DIR / "velocity_noise_stats.csv"

    vel_noise, vel_means, vel_stds = read_columns(
        velocity_noise_csv,
        x_column="Vel_Noise",
        mean_column="T1_LW_Avg",
        std_column="T1_LW_Std",
    )

    filtered = [
        (v, m, s)
        for v, m, s in zip(vel_noise, vel_means, vel_stds)
        if v != 0.0
    ]

    vel_noise = [row[0] for row in filtered]
    vel_means = [row[1] for row in filtered]
    vel_stds = [row[2] for row in filtered]

    vel_bounds = [
        analytical_bound_us(
            distance_m=DEFAULT_DISTANCE,
            velocity_error_mps=v,
            jitter_s=DEFAULT_JITTER,
            alpha=DEFAULT_CLOCK_SKEW,
        )
        for v in vel_noise
    ]

    draw_subplot(
        axes[2],
        vel_noise,
        vel_means,
        vel_stds,
        vel_bounds,
        xlabel=r"Velocity Error Std. Dev. (m/s)",
        subtitle="(c) Velocity Error Std. Dev.",
    )

    # --------------------------------------------------------
    # (d) Initial skew
    # --------------------------------------------------------
    skew_csv = RESULTS_DIR / "initial_skew_stats.csv"

    # If your first column name is different, replace "ClockSkew"
    # with the actual column name in initial_skew_stats.csv.
    skew_ppm, skew_means, skew_stds = read_columns(
        skew_csv,
        x_column="ClockSkew",
        mean_column="T1_LW_Avg",
        std_column="T1_LW_Std",
    )

    skew_bounds = [
        analytical_bound_us(
            distance_m=DEFAULT_DISTANCE,
            velocity_error_mps=DEFAULT_VELOCITY_ERROR,
            jitter_s=DEFAULT_JITTER,
            alpha=skew * 1e-6,   # ppm -> fractional skew
        )
        for skew in skew_ppm
    ]

    draw_subplot(
        axes[3],
        skew_ppm,
        skew_means,
        skew_stds,
        skew_bounds,
        xlabel="Initial Skew (ppm)",
        subtitle="(d) Initial Skew",
    )

    fig.suptitle(
        "Analytical error bound  vs. simulated LW-Sync error",
        fontsize=14,
    )

    plt.tight_layout(rect=[0, 0, 1, 0.97])

    save_path = FIGURES_DIR / "analytical_bound_comparison_combined.png"
    plt.savefig(save_path, dpi=300, bbox_inches="tight")

    print(f"Figure saved: {save_path}")

    plt.show()


if __name__ == "__main__":
    main()
