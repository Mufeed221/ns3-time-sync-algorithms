# LW-Sync Reproducibility Package

This repository contains the ns-3.40 implementations, experiment scripts, raw and processed results, plotting tools, and documentation used to evaluate LW-Sync and the comparison protocols.

## Included Protocols

- **LW-Sync**: proposed low-complexity, event-driven offset-correction method.
- **LT-Sync**: lightweight Doppler-assisted baseline.
- **DC-Sync**: iterative Doppler-compensation baseline.
- **UWGS**: geometry-based synchronization baseline used in the dedicated common-trajectory comparison.

## Repository Structure

```text
.
├── scratch/                    # ns-3 protocol implementations
├── scripts/                    # Batch scripts for parameter sweeps
├── plotting/                   # Plotting and statistics-verification scripts
├── results/
│   ├── raw/                    # Individual Monte Carlo outputs
│   └── processed/              # Mean and standard-deviation files
├── figures/                    # Regenerated manuscript figures
├── docs/                       # Detailed reproducibility documentation
├── README.md
└── LICENSE
```

## Requirements

- Ubuntu 20.04 or later
- ns-3.40
- Eigen

## Installation

### 1. Install the required packages

Update the package index and install the dependencies required to build ns-3.40 and run the experiments:

```bash
sudo apt update

sudo apt install -y \
    build-essential \
    g++ \
    python3 \
    cmake \
    make \
    ninja-build \
    git \
    ccache \
    wget \
    libeigen3-dev \
    gsl-bin \
    libgsl-dev \
    libgslcblas0 \
    libxml2 \
    libxml2-dev \
    libsqlite3-dev \
    libboost-all-dev \
    openmpi-bin \
    openmpi-common \
    libopenmpi-dev \
    tcpdump \
    wireshark \
    sqlite3 \
    graphviz \
    imagemagick \
    doxygen \
    parallel
```

Optional visualization and documentation packages can be installed with:

```bash
sudo apt install -y \
    gir1.2-goocanvas-2.0 \
    python3-gi \
    python3-gi-cairo \
    python3-pygraphviz \
    gir1.2-gtk-3.0 \
    ipython3 \
    qtbase5-dev \
    qtchooser \
    qt5-qmake \
    qtbase5-dev-tools \
    python3-sphinx \
    dia \
    texlive \
    dvipng \
    latexmk \
    texlive-extra-utils \
    texlive-latex-extra \
    texlive-font-utils
```

### 2. Download and extract ns-3.40

Create a workspace directory:

```bash
mkdir -p ~/workspace
cd ~/workspace
```

Download `ns-allinone-3.40.tar.bz2` from the official ns-3 release page:

https://www.nsnam.org/releases/ns-3-40/download/

Extract the archive:

```bash
tar -xjf ns-allinone-3.40.tar.bz2
cd ns-allinone-3.40/ns-3.40
```

### 3. Install Eigen 5.0.0

UWGS requires Eigen for its matrix and nonlinear-estimation operations. Install Eigen 5.0.0 from source:

```bash
cd ~/workspace

wget https://gitlab.com/libeigen/eigen/-/archive/5.0.0/eigen-5.0.0.tar.gz
tar -xzf eigen-5.0.0.tar.gz
cd eigen-5.0.0

mkdir -p build
cd build

cmake ..
make
sudo make install
```

Verify that Eigen was installed:

```bash
ls /usr/local/include/eigen3/Eigen
```

### 4. Add the reproducibility files to ns-3

Copy the protocol implementation files from this repository into the ns-3.40 `scratch/` directory:

```bash
cp scratch/*.cc ~/workspace/ns-allinone-3.40/ns-3.40/scratch/
```

Copy the experiment, plotting, documentation, and results directories into the ns-3.40 directory:

```bash
cp -r scripts plotting docs results figures \
    ~/workspace/ns-allinone-3.40/ns-3.40/
```

Any modified ns-3 source files required by the implementations must also be copied to the locations specified in the repository documentation.

After copying the files, the relevant directory structure should resemble:

```text
ns-3.40/
├── scratch/
│   ├── LW-Sync.cc
│   ├── LT-Sync.cc
│   ├── DC-Sync.cc
│   └── UWGS.cc
├── scripts/
├── plotting/
├── results/
├── figures/
└── docs/
```

### 5. Configure and build ns-3

From the ns-3.40 directory, remove any previous build files:

```bash
cd ~/workspace/ns-allinone-3.40/ns-3.40

./ns3 clean
rm -rf cmake-cache build
```

Configure ns-3:

```bash
./ns3 configure \
    --build-profile=default \
    --disable-examples \
    --disable-tests \
    --disable-werror
```

Build the project:

```bash
./ns3 build
```

### 6. Verify the installation

Run one LW-Sync simulation:

```bash
./ns3 run "scratch/LW-Sync --run=1"
```

A successful execution confirms that ns-3 and the LW-Sync implementation were compiled correctly.

The remaining implementations can be checked with:

```bash
./ns3 run "scratch/LT-Sync --run=1"
./ns3 run "scratch/DC-Sync --run=1"
./ns3 run "scratch/UWGS --run=1"
```


## Quick Verification

These commands verify compilation and basic execution. They do not reproduce the full statistical study.

```bash
./ns3 run "scratch/LW-Sync --run=1"
./ns3 run "scratch/LT-Sync --run=1"
./ns3 run "scratch/DC-Sync --run=1"
./ns3 run "scratch/UWGS --run=1"
```

## Reproducing the Experiments

Representative commands are:

```bash
bash scripts/run_distance.sh
bash scripts/run_velocity.sh
bash scripts/run_acceleration.sh
bash scripts/run_velocity_noise.sh
bash scripts/run_jitter_noise.sh
bash scripts/run_initial_offset.sh
bash scripts/run_initial_skew.sh
bash scripts/run_message_count.sh
bash scripts/run_energy.sh
bash scripts/run_uwgs_velocity.sh
bash scripts/run_uwgs_position_noise.sh
```

## Generating the Figures

Generate all figures:

```bash
bash plotting/plot_all.sh
```

## Results and Units

- Raw results contain one row per independent simulation run.
- Processed results contain the mean and standard deviation for each parameter value.
- Offset error is reported in microseconds.
- Skew error is reported in parts per million.
- Energy is reported in joules unless otherwise stated.

## Random-Seed Policy

The ns-3 master seed is fixed at 1. Independent realizations are generated by varying the run number from 1 to 500 for each protocol, mobility condition, and plotted parameter value.

See [`docs/random-streams.md`](docs/random-streams.md).

## Documentation

- [`simulation-parameters.md`](docs/simulation-parameters.md)
- [`random-streams.md`](docs/random-streams.md)
- [`baseline-implementations.md`](docs/baseline-implementations.md)
- [`timestamp-and-noise-models.md`](docs/timestamp-and-noise-models.md)
- [`energy-accounting.md`](docs/energy-accounting.md)
- [`reproduction-map.md`](docs/reproduction-map.md)

## Code and Data Availability

The repository contains the implementation code, simulation scripts, raw and processed results, plotting tools, and methodological documentation used for the manuscript.

## License

See `LICENSE`.

