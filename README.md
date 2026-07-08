# ns3-time-sync-algorithms

This repository contains the source code for the underwater time synchronization algorithms presented in our paper. It includes only the files that were modified or added to **ns-3.40** and is intended to be merged into an existing ns-3.40 installation.

## Requirements

* ns-3.40
* Eigen 5.0.0
* g++ 9 or later
* Linux (Ubuntu 20.04 or newer recommended)

## Installation

### 1. Clone this repository

Clone the repository to any temporary location:

```bash
git clone https://github.com/Mufeed221/ns3-time-sync-algorithms.git
```

### 2. Copy the repository contents into ns-3.40

Copy all repository files into your existing ns-3.40 installation:

```bash
cp -r ns3-time-sync-algorithms/* ~/ns-allinone-3.40/ns-3.40/
cp -r ns3-time-sync-algorithms/.[!.]* ~/ns-allinone-3.40/ns-3.40/ 2>/dev/null
```

The files will be merged with the existing ns-3.40 directory, replacing the modified files.

### 3. Rebuild ns-3

```bash
cd ~/ns-allinone-3.40/ns-3.40

./ns3 clean
./ns3 configure
./ns3 build
```

### 4. Verify the installation

Run one of the included simulation programs:

```bash
./ns3 run "scratch/LW-Sync"
```

If the program builds and runs without compilation errors, the installation was successful.

## Repository Structure

```text
scratch/    Simulation programs
scripts/    Batch execution scripts
csv/        Data processing scripts
results/    Sample output data
```

## Notes

* This repository is **not** a standalone ns-3 distribution.
* A working installation of **ns-3.40** is required before copying these files.
* Existing files in the ns-3.40 directory may be overwritten by the modified versions contained in this repository.
