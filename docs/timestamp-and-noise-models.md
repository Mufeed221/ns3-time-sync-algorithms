# Timestamp and Noise Models

This document describes how timestamps, clock granularity, timestamp jitter, velocity-estimation error, and trajectory-position noise are modeled in the simulations.

## 1. Timestamp Capture

Transmission timestamps are recorded immediately before the packet is passed to the transmission procedure.

Reception timestamps are recorded after successful MAC-layer packet reception and before the received timestamp is used by the synchronization algorithm.

The timestamp-noise model is applied only to reception timestamps. Transmission timestamps are not perturbed by the timestamp-noise random variable.

A new timestamp-noise sample is generated independently for every successfully received synchronization packet.

## 2. Timestamp Noise

Timestamp noise is modeled as a zero-mean Gaussian random variable:

$$n_t\sim\mathcal{N}(0,\sigma_t^2)$$

The recorded reception timestamp is

$$\tilde{T}_{\mathrm{RX}}=T_{\mathrm{RX}}+n_t$$

where $T_{\mathrm{RX}}$ is the timestamp before measurement noise is added and $\tilde{T}_{\mathrm{RX}}$ is the noisy timestamp used by the synchronization algorithm.

The default timestamp-noise standard deviation is

$$\sigma_t=20\ \mu\mathrm{s}$$

The jitter value is represented internally in seconds. For example,

$$20\ \mu\mathrm{s}=20\times10^{-6}\ \mathrm{s}$$

A separate random value is generated for each packet reception.

## 3. Clock Granularity

The configured clock granularity is

$$g=1\ \mu\mathrm{s}$$

The timestamps are represented using the ns-3 time system. No additional rounding or quantization operation is applied to force timestamps to integer multiples of the configured granularity.

Therefore, the granularity parameter records the nominal clock resolution used in the experiment configuration, while the implemented timestamps retain the precision provided by the ns-3 time representation.

## 4. Radial Velocity

The synchronization protocols use radial velocity rather than the complete velocity vector.

Let $\mathbf{p}*{\mathrm{node}}$ and $\mathbf{p}*{\mathrm{ref}}$ denote the positions of the mobile and reference nodes, respectively, and let $\mathbf{v}$ denote the mobile-node velocity vector.

The true radial velocity is calculated as

$$v_r=\frac{\mathbf{v}\cdot \left(\mathbf{p}_{\mathrm{node}}*\mathbf{p}_{\mathrm{ref}}\right)}
{\left|\mathbf{p}_{\mathrm{node}}*\mathbf{p}_{\mathrm{ref}}\right|}$$

A positive radial velocity indicates motion in the direction of increasing separation under the sign convention used by the implementation, while a negative value indicates decreasing separation.

## 5. Velocity-Noise

The measured radial velocity is modeled as

$$\tilde{v}_r = v_r+n_v$$

where

$$n_v\sim\mathcal{N}(0,\sigma_v^2)$$

The default velocity-noise standard deviation is

$$\sigma_v=0.2\ \mathrm{m/s}$$

A new velocity-noise sample is generated when a protocol obtains a radial-velocity measurement associated with a received packet.

LW-Sync, LT-Sync, and DC-Sync use the resulting noisy radial-velocity value in their synchronization calculations.

The noise is added after the true radial velocity has been calculated from the ns-3 mobility state:

$$\tilde{v}_r(t) = v_r(t)+n_v(t)$$

This models uncertainty in the Doppler-derived velocity estimate without changing the physical node trajectory.

## 6. Trajectory-Position Noise

Trajectory-position noise is modeled independently on the horizontal coordinates as

$$n_x\sim\mathcal{N}(0,\sigma_p^2)$$

$$n_y\sim\mathcal{N}(0,\sigma_p^2)$$

For the UWGS common-trajectory experiment, the perturbed coordinates are

$$x_N(t) = x(t)+n_x(t)$$

$$y_N(t)=y(t)+n_y(t)$$

where $x(t)$ and $y(t)$ are the coordinates produced by the nominal trajectory model.

The generated position errors are applied directly to the ns-3 mobility coordinates. Consequently, the noise changes the physical trajectory used by the channel and mobility calculations rather than perturbing only the position information supplied to the estimator.

Because LW-Sync and UWGS are executed using the same perturbed mobility model in this experiment, both protocols experience the same trajectory-position uncertainty.

For LW-Sync, this perturbation can also affect the calculated radial velocity because radial velocity depends on the node positions:

$$v_r=\frac{\mathbf{v}\cdot \left(\mathbf{p}_{\mathrm{node}}*\mathbf{p}_{\mathrm{ref}}\right)}
{\left|\mathbf{p}_{\mathrm{node}}*\mathbf{p}_{\mathrm{ref}}\right|}$$

For UWGS, the perturbed coordinates directly affect the propagation-distance and nonlinear clock-parameter calculations.

## 7. Independence of Noise Samples

Timestamp, velocity, and position noises are generated using separate random-variable streams.

The following quantities are treated as independent unless otherwise stated:

* timestamp-noise samples generated for different receptions,
* velocity-noise samples generated for different measurements,
* horizontal and vertical position-noise samples,
* and noise samples generated in different Monte Carlo runs.

The explicit stream assignments and run-number configuration are documented in [`random-streams.md`](docs/random-streams.md).

## 8. Units

| Quantity                  | Internal unit              | Reporting unit               |
| ------------------------- | -------------------------- | ---------------------------- |
| Timestamp noise           | Microseconds               | Microseconds                 |
| Clock offset              | Seconds                    | Seconds                      |
| Clock skew                | Dimensionless scale factor | Parts per million            |
| Velocity error            | Meters per second          | Meters per second            |
| Trajectory-position noise | Meters                     | Meters                       |
| Offset error              | Seconds                    | Microseconds                 |
| Skew error                | Dimensionless difference   | Parts per million            |
