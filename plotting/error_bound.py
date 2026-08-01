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

CLOCK_SKEW = 200e-6          # 200 ppm converted to fractional skew
SOUND_SPEED = 1500.0         # m/s
REPLY_TIME = 0.024           # seconds

DEFAULT_DISTANCE = 500.0     # meters
DEFAULT_VELOCITY_ERROR = 0.2 # m/s
DEFAULT_JITTER = 20e-6       # seconds


# ============================================================
# Analytical maximum-error function
# ============================================================

def analytical_bound_us(
    distance_m: float,
    velocity_error_mps: float,
    jitter_s: float,
) -> float:
    propagation_delay = distance_m / SOUND_SPEED

    skew_component = CLOCK_SKEW * (
        2.0 * propagation_delay
        + 0.5 * REPLY_TIME
    )

    velocity_component = (
        velocity_error_mps / (2.0 * SOUND_SPEED)
    ) * (
        (1.0 - 2.0 * CLOCK_SKEW) * propagation_delay
        + REPLY_TIME
    )

    jitter_component = (
        1.0 + CLOCK_SKEW
    ) * jitter_s

    bound_seconds = (
        skew_component
        + velocity_component
        + jitter_component
    )

    return bound_seconds * 1e6


# ============================================================
# CSV-reading helper
# ============================================================

def read_columns(
    csv_path: Path,
    x_column: str,
    mean_column: str,
    std_column: str,
):
    x_values = []
    means = []
    standard_deviations = []

    with csv_path.open("r", newline="", encoding="utf-8") as file:
        reader = csv.DictReader(file)

        required_columns = {x_column, mean_column, std_column}
        available_columns = set(reader.fieldnames or [])
        missing_columns = required_columns - available_columns

        if missing_columns:
            raise ValueError(
                f"Missing columns in {csv_path.name}: {sorted(missing_columns)}\n"
                f"Available columns: {sorted(available_columns)}"
            )

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
            standard_deviations.append(float(std_text))

    if not x_values:
        raise ValueError(f"No valid rows were read from {csv_path}")

    ordered_rows = sorted(
        zip(x_values, means, standard_deviations),
        key=lambda row: row[0],
    )

    x_values, means, standard_deviations = map(list, zip(*ordered_rows))

    return x_values, means, standard_deviations


# ============================================================
# Generic plotting helper
# ============================================================

def create_comparison_plot(
    x_values,
    simulated_means,
    simulated_stds,
    analytical_bounds,
    xlabel: str,
    title: str,
    output_filename: str,
):
    fig, ax = plt.subplots(figsize=(8, 6))

    # Analytical bound first, in gray.
    ax.plot(
        x_values,
        analytical_bounds,
        marker="s",
        linewidth=1.8,
        color="gray",
        label="Maximum analytical bound",
        zorder=1,
    )

    # Simulated mean ± sigma on top.
    ax.errorbar(
        x_values,
        simulated_means,
        yerr=simulated_stds,
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
    ax.set_title(title)
    ax.grid(True, alpha=0.3)
    ax.legend()

    fig.tight_layout()

    output_path = FIGURES_DIR / output_filename
    fig.savefig(output_path, dpi=300, bbox_inches="tight")
    print(f"Figure saved: {output_path}")

    plt.close(fig)


# ============================================================
# Plot 1: Distance
# ============================================================

def plot_distance_comparison():
    csv_path = RESULTS_DIR / "distance_stats.csv"

    distances, means, stds = read_columns(
        csv_path=csv_path,
        x_column="Distance",
        mean_column="T1_LW_Avg",
        std_column="T1_LW_Std",
    )

    analytical_bounds = [
        analytical_bound_us(
            distance_m=distance,
            velocity_error_mps=DEFAULT_VELOCITY_ERROR,
            jitter_s=DEFAULT_JITTER,
        )
        for distance in distances
    ]

    create_comparison_plot(
        x_values=distances,
        simulated_means=means,
        simulated_stds=stds,
        analytical_bounds=analytical_bounds,
        xlabel="Initial Distance (m)",
        title="Maximum Analytical Bound vs. Simulated LW-Sync Error",
        output_filename="analytical_bound vs distance.png",
    )


# ============================================================
# Plot 2: Timestamp jitter
# ============================================================

def plot_jitter_comparison():
    csv_path = RESULTS_DIR / "jitter_noise_stats.csv"

    jitter_us, means, stds = read_columns(
        csv_path=csv_path,
        x_column="Jitter",
        mean_column="T1_LW_Avg",
        std_column="T1_LW_Std",
    )

    analytical_bounds = [
        analytical_bound_us(
            distance_m=DEFAULT_DISTANCE,
            velocity_error_mps=DEFAULT_VELOCITY_ERROR,
            jitter_s=jitter * 1e-6,
        )
        for jitter in jitter_us
    ]

    create_comparison_plot(
        x_values=jitter_us,
        simulated_means=means,
        simulated_stds=stds,
        analytical_bounds=analytical_bounds,
        xlabel=r"Maximum Timestamp Error, $\delta_t$ ($\mu$s)",
        title="Maximum Analytical Bound vs. Simulated LW-Sync Error",
        output_filename="analytical bound vs jitter.png",
    )


# ============================================================
# Plot 3: Velocity error
# ============================================================

def plot_velocity_error_comparison():
    csv_path = RESULTS_DIR / "velocity_noise_stats.csv"

    velocity_errors, means, stds = read_columns(
        csv_path=csv_path,
        x_column="Vel_Noise",
        mean_column="T1_LW_Avg",
        std_column="T1_LW_Std",
    )

    # Ignore the first point where velocity noise = 0
    filtered_rows = [
        (v, m, s)
        for v, m, s in zip(velocity_errors, means, stds)
        if v != 0.0
    ]

    velocity_errors = [row[0] for row in filtered_rows]
    means = [row[1] for row in filtered_rows]
    stds = [row[2] for row in filtered_rows]

    analytical_bounds = [
        analytical_bound_us(
            distance_m=DEFAULT_DISTANCE,
            velocity_error_mps=velocity_error,
            jitter_s=DEFAULT_JITTER,
        )
        for velocity_error in velocity_errors
    ]

    create_comparison_plot(
        x_values=velocity_errors,
        simulated_means=means,
        simulated_stds=stds,
        analytical_bounds=analytical_bounds,
        xlabel=r"Maximum Velocity Error, $\delta_v$ (m/s)",
        title="Maximum Analytical Bound vs. Simulated LW-Sync Error",
        output_filename="analytical_bound vs velocity_error.png",
    )


# ============================================================
# Main
# ============================================================

def main():
    plot_distance_comparison()
    plot_jitter_comparison()
    plot_velocity_error_comparison()
    print("All analytical-bound comparison plots were generated.")


if __name__ == "__main__":
    main()
