# Random Seeds and Streams

## Master Seed and Run Number

The ns-3 master seed is fixed at:

```cpp
RngSeedManager::SetSeed(1);
```

For each plotted parameter value, the run number varies from 1 to 1000:

```cpp
RngSeedManager::SetRun(runNumber);
```

## Meaning of One Run

One run is one independent realization for one combination of:

- protocol,
- mobility model,
- parameter value,
- and simulation configuration.

Therefore, 1000 runs are executed for every plotted data point rather than 1000 runs across the entire study.

## Random Variables

| Quantity | Distribution | Parameters | Used by |
|---|---|---|---|
| Timestamp jitter | Gaussian | mean 0, standard deviation `sigma_t` | Relevant protocols |
| Velocity-estimation error | Gaussian | mean 0, standard deviation `sigma_v` | Doppler-assisted protocols |
| Trajectory-position noise | Gaussian | mean 0, standard deviation `sigma_p` | UWGS common-trajectory experiment |

## Explicit Stream Assignment

```cpp
jitterRv->SetStream(1);
velocityRv->SetStream(2);
positionXRv->SetStream(3);
positionYRv->SetStream(4);
```

## Cross-Protocol Comparability

The same run number is used for protocols evaluated under the same parameter value and mobility model.

## Typical Batch Structure

```bash
SEED=1
FIRST_RUN=1
LAST_RUN=1000

for run in $(seq "${FIRST_RUN}" "${LAST_RUN}"); do
    ./ns3 run "scratch/LW-Sync --seed=${SEED} --run=${run}"
done
```

## Statistical Convention

For each plotted parameter value, the mean is calculated from the 1000 independent simulation runs as

$$\bar{x}\frac{1}{n}\sum_{i=1}^{n}x_i$$

where $n=1000$.

The reported standard deviation is the population standard deviation:

$$\sigma\sqrt{\frac{1}{n}\sum_{i=1}^{n}(x_i-\bar{x})^2}$$

The processing scripts implement the equivalent computational form

$$\sigma\sqrt{\frac{\sum_{i=1}^{n}x_i^2}{n}\bar{x}^2}$$

This convention corresponds to `ddof=0` in Python and NumPy.

The same definition is used throughout the result-processing scripts, plotting scripts, manuscript figures, and reported error bars.

