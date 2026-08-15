# Simulation Parameters

This document records the common ns-3 configuration, clock settings, mobility models, protocol parameters, and experiment sweeps used in the study.

## 1. Software Environment

| Item                 | Value                 |
| -------------------- | --------------------- |
| ns-3 version         | 3.40                  |
| Operating system     | Ubuntu 20.04 or later |
| Compiler             | g++ 9 or later        |
| Numerical dependency | Eigen                 |
| Plotting environment | Python 3.9 or later   |

## 2. UAN Configuration

All evaluated protocols use the same underwater acoustic network configuration unless explicitly stated otherwise.

| Parameter               | Value                   |
| ----------------------- | ----------------------- |
| Channel framework       | ns-3 UAN                |
| Propagation model       | `UanPropModelThorp`     |
| Noise model             | `UanNoiseModelDefault`  |
| Wind speed              | 5 m/s                   |
| Shipping activity       | 0.5                     |
| MAC protocol            | `UanMacAloha`           |
| PHY model               | `UanPhyGen`             |
| Packet-error-rate model | `UanPhyPerGenDefault`   |
| SINR model              | `UanPhyCalcSinrDefault` |
| Modulation              | FSK                     |
| Carrier frequency       | 20 kHz                  |
| Bandwidth               | 4 kHz                   |
| Data rate               | 20 kbps                 |
| Symbol rate             | 2.4 ksym/s              |
| Transmission power      | 160 dB re 1 µPa         |
| Packet size             | 60 bytes                |
| Sound speed             | 1500 m/s                |

## 3. Default Clock and Measurement Parameters

| Parameter                           | Value   |
| ----------------------------------- | ------- |
| Initial distance                    | 500 m   |
| Initial clock offset                | 20 ms   |
| Initial clock skew                  | 200 ppm |
| Clock granularity                   | 1 µs    |
| Timestamp-noise standard deviation  | 20 µs   |
| Velocity-noise standard deviation   | 0.2 m/s |

Timestamp noise is injected after packet reception by adding a zero-mean Gaussian random value to the recorded receive timestamp.

Velocity noise is injected after packet reception by adding a zero-mean Gaussian random value to the true radial velocity at the corresponding reception time.

The resulting noisy radial-velocity measurement is

$$
\tilde{v}_r(t)=v_r(t)+n_v(t)
$$

where $v_r(t)$ is the true radial velocity and $n_v(t)$ is the generated velocity error.

## 4. Mobility Models

### 4.1 Track 1

Track 1 represents motion along a predominantly linear trajectory.

| Parameter              | Value       |
| ---------------------- | ----------- |
| Initial position       | $(500,0)$ m |
| Initial velocity       | 2 m/s       |
| Acceleration magnitude | 0.03 m/s²   |
| Maximum speed          | 5 m/s       |

The mobile node begins with a velocity of 2 m/s and accelerates until it reaches the configured maximum speed of 5 m/s.

The complete Track 1 implementation may also include deceleration, stopping, and direction-reversal phases depending on the selected simulation duration and experiment configuration.

### 4.2 Track 2

Track 2 represents circular motion.

| Parameter     | Value            |
| ------------- | ---------------- |
| Circle center | $(400,0)$ m      |
| Radius        | 100 m            |
| Initial angle | 0 rad            |
| Direction     | Counterclockwise |
| Linear speed  | 2 m/s            |

For a circular trajectory with radius $R$ and linear speed $v$, the angular velocity is 0.02 rad/s

$$\omega=\frac{v}{R}$$

Using the default values,

$$\omega=\frac{2}{100}=0.02\ \mathrm{rad/s}$$

The node coordinates are calculated as

$$x(t)=x_c+R\cos(\theta_0+\omega t)$$

$$y(t)=y_c+R\sin(\theta_0+\omega t)$$

where $(x_c,y_c)$ is the circle center and $\theta_0$ is the initial angle.

### 4.3 UWGS Common-Comparison Trajectory

LW-Sync and UWGS use the same physical trajectory in the dedicated comparison experiments.

| Parameter                               | Value                           |
| --------------------------------------- | ------------------------------- |
| Reference-node position                 | $(0,0)$ m                       |
| Mobile-node initial position            | $(100,-500)$ m                  |
| Default forward velocity                | 2 m/s                           |
| Velocity sweep                          | 0.5, 1, 2, 3, 4, and 5 m/s      |
| Sinusoidal amplitude                    | 3 m                             |
| Angular frequency                       | 0.05 rad/s                      |
| Position-noise standard-deviation sweep | 0, 0.5, 1, 2, 3, 4, 5, and 10 m |
| Packet-transmission interval            | 10 s                            |

The common trajectory is defined as

$$x(t)=x_0+A_x\sin(\omega t)$$

$$y(t)=y_0+V(t-t_0)$$

where $A_x$ is the sinusoidal amplitude, $\omega$ is the angular frequency, and $V$ is the forward velocity.

For the position-noise experiments, the implemented trajectory becomes

$$x_N(t)=x_0+A_x\sin(\omega t)+n_x(t)$$

$$y_N(t)=y_0+V(t-t_0)+n_y(t)$$

where $n_x(t)$ and $n_y(t)$ are independently generated zero-mean Gaussian position-error samples.

## 5. Protocol Parameters

### 5.1 LW-Sync

| Parameter                              | Value                                    |
| -------------------------------------- | ---------------------------------------- |
| Default number of one-way beacons      | 10                                       |
| Beacon interval                        | 5 s                                      |
| Initial offset phase                   | Request, reply, and returned data packet |
| Velocity input                         | PHY-derived radial velocity              |
| Velocity refinement                    | None                                     |

LW-Sync first performs a two-way exchange to obtain an initial clock-offset estimate. It then uses one-way beacon transmissions to estimate clock skew and refine the offset under mobility.

### 5.2 LT-Sync

| Parameter           | Value                       |
| ------------------- | --------------------------- |
| Reply backoff       | 5 s                         |
| Initial distance    | 500 m                       |
| Velocity input      | PHY-derived radial velocity |
| Total message count | 3                           |

LT-Sync uses a single three-message synchronization exchange.

### 5.3 DC-Sync

| Parameter                                | Value                            |
| ---------------------------------------- | -------------------------------- |
| Default number of two-way exchange pairs | 20                               |
| Request interval                         | 5 s                              |
| Default refinement iterations            | 5                                |
| Velocity fitting model                   | Cubic polynomial                 |
| Parameter estimation                     | Iterative ordinary least squares |

DC-Sync repeatedly updates the estimated velocity, equivalent Doppler scaling factors, clock skew, and clock offset over the configured refinement iterations.

### 5.4 UWGS

| Parameter                              | Value                  |
| -------------------------------------- | ---------------------- |
| Required successfully received packets | 5 one-way packets      |
| Clock parameters                       | Estimated jointly      |
| Solver                                 | Nonlinear root-finding |
| Initial guess                          | $a=1$, $b=0$           |
| Residual tolerance                     | $10^{-14}$             |
| Maximum iterations                     | 40                     |

## 6. Parameter Sweeps

Unless otherwise stated, 1000 independent Monte Carlo runs are performed for every protocol, mobility model, and parameter value.

| Experiment                          | Evaluated values                                      | Runs per point |
| ----------------------------------- | ----------------------------------------------------- | -------------: |
| Initial distance                    | 500, 600, 700, 800, 900, and 1000 m                   |            500 |
| Starting angle                      | 0°, 45°, 90°, 135°, 180°, 225°, 270°, and 315°        |            500 |
| Initial velocity                    | 0, 1, 2, 3, 4, and 5 m/s                              |            500 |
| Acceleration                        | 0.01, 0.03, 0.05, 0.07, and 0.1 m/s²                  |            500 |
| Velocity-error standard deviation   | 0, 0.1, 0.2, 0.3, 0.4, and 0.5 m/s                    |            500 |
| Timestamp-jitter standard deviation | 10, 20, 30, 40, and 50 µs                             |            500 |
| Initial clock offset                | 0.02, 0.05, 1, 5, 10, and 20 s                        |            500 |
| Initial clock skew                  | 100, 200, 400, 800, 1600, and 3200 ppm                |            500 |
| LW-Sync beacon count                | 10, 15, 20, 25, and 30 one-way beacons                |            500 |
| DC-Sync exchange-pair count         | 10, 15, 20, 25, and 30 Two-way Message Exchange Pairs |            500 |
| UWGS common-trajectory velocity     | 0.5, 1, 2, 3, 4, and 5 m/s                            |            500 |
| UWGS trajectory-position noise      | 0, 0.5, 1, 2, 3, 4, 5, and 10 m                       |            500 |

## 7. Monte Carlo Interpretation

One simulation run represents one independent realization of a specific combination of:

* protocol,
* mobility model,
* parameter value,
* random run number,
* and simulation configuration.

One Thousand independent runs are executed for every plotted data point rather than 1000 runs being shared across the complete study.

For each data point, the processed results report the arithmetic mean and population standard deviation over the 1000 runs.

The estimated values and true values in LW-Sync are in oppsite signs, making the error calculated as this

$$e_{\mathrm{skew,ppm}} = \left| \hat{\alpha}+\alpha\right|\times10^6$$
$$e_{\mathrm{offset,µs}} = \left| \hat{\beta}+\beta\right|\times10^6$$

For the rest of the protocols the error is calculated as this

$$e_{\mathrm{skew,ppm}} = \left| \hat{\alpha}-\alpha\right|\times10^6$$
$$e_{\mathrm{offset,µs}} = \left| \hat{\beta}-\beta\right|\times10^6$$

where $\alpha$ is the true clock-skew, $\hat{\alpha}$ is its estimate, and $\beta$ is the true clock-offset, $\hat{\beta}$ is its estimate

