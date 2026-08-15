# Baseline Implementation Details

This document records the implementation choices, common simulation settings, message procedures, and equations used for the LT-Sync, DC-Sync, and UWGS baseline protocols. 
See [`simulation-parameters.md`](docs/simulation-parameters.md)

## 1. General Implementation Policy

All baseline protocols are evaluated using the same underwater acoustic channel, clock configuration, and measurement-noise settings whenever the protocol assumptions permit.

---

## 2. LT-Sync

### 2.1 Message Procedure

LT-Sync uses a three-message exchange:

1. The beacon node transmits a wake-up request.
2. The ordinary node receives the request and transmits a reply.
3. The beacon node receives the reply and transmits a final response.

### 2.2 Protocol Parameters

| Parameter           |      Value |
| ------------------- | ---------: |
| Reply backoff       |        5 s |
| Total message count | 3 messages |
| Simulation duration |       20 s |

### 2.3 Implemented Equations

The initial distance is estimated as

$$
l=\frac{c\left[(A_1-A_0)-(B_1-B_0)\right]}{2}
$$

where $c$ is the speed of sound.

The propagation delays are estimated using

$$
d_0=\frac{l}{c-v}
$$

$$
d_1=\frac{l+(B_1-A_0)v}{c}
$$

and

$$
d_2=\frac{l+(A_2-A_0)v}{c-v}
$$

The clock skew is estimated as

$$
\alpha=
\frac{B_2-B_0}
{A_2-A_0+d_2-d_0}
$$

The clock offset is estimated as

$$
\beta=
\frac{
(B_0+B_1)-\alpha(A_0+A_1+d_0-d_1)
}{2}
$$

---

## 3. DC-Sync

### 3.1 Message Procedure

DC-Sync performs repeated two-way message exchanges:

1. The beacon node transmits a request at each request interval.
2. The ordinary node receives the request and immediately transmits a reply.

The collected timestamps and Doppler measurements are then processed iteratively to estimate the clock skew and offset.

### 3.2 Protocol Parameters

| Parameter                        | Value |
| -------------------------------- | ----: |
| Request interval                 |   5 s |
| Reply backoff                    |   0 s |
| Number of two-way exchange pairs |    20 |
| Simulation duration              | 110 s |

The simulation duration is calculated as

$T_{\mathrm{sim}}=(N_{\mathrm{pairs}}*T_{\mathrm{interval}})+10 = (20 * 5)+10 = 110\ \mathrm{s}$

### 3.3 Implemented Equations

The relative velocity is refined using the clock-skew estimate obtained during each calibration iteration:

$$v(t_2) = \left[1-(1-a_{AB})\alpha\right]c$$

$$v(t_4) = \left[(1+a_{BA})\alpha-1\right]c$$

The estimated velocity samples are fitted using a cubic polynomial:

$$v(t)=p_3t^3+p_2t^2+p_1t+p_0$$

The fitted velocity model is integrated to estimate the distance variation during the $i$-th two-way exchange:

$$\Delta d_i = \int_{t_2[i]}^{t_4[i]}v(t),dt$$

The equivalent Doppler scaling factor is then calculated as

$$a_e[i] = \frac{\Delta d_i}
{c\left(t_4[i]-t_2[i]\right)}$$

Ordinary least squares is used to estimate the clock-skew and clock-offset parameters:

$$\hat{\mu}=[\hat{\alpha}\ \ \ \hat{\beta}]^T=(H^T\ H)^{-1}H^TZ$$

The design matrix is

$$H = \begin{bmatrix}
t_4[1](1 - a_e[1]) + t_1[1] & 2 - a_e[1] \\
\vdots & \vdots \\
t_4[i](1 - a_e[i]) + t_1[i] & 2 - a_e[i]
\end{bmatrix}$$

and the observation vector is

$$Z = \begin{bmatrix}
T_3[1] + (1 - a_e[1]) T_2[1] \\
\vdots \\
T_3[i] + (1 - a_e[i]) T_2[i]
\end{bmatrix}$$

The Doppler scaling factors, fitted velocity model, and clock parameters are updated during the calibration iterations until the configured stopping condition is reached.

---

## 4. UWGS

### 4.1 Message Procedure

UWGS uses the first five successfully received one-way synchronization packets.

The beacon node periodically transmits packets containing its transmission timestamps and the required trajectory information. The ordinary node records the corresponding reception timestamps and uses the five timestamp pairs to solve for the clock parameters.

### 4.2 Protocol Parameters

| Parameter                 |                        Value |
| ------------------------- | ---------------------------: |
| Number of one-way packets |                            5 |
| Transmission interval     |                         10 s |
| Packet-loss handling      | No packet loss is introduced |
| Simulation duration       |                         60 s |

The simulation duration is calculated as

$T_{\mathrm{sim}} = (N_{\mathrm{packets}} * T_{\mathrm{interval}})+10 = (5 * 10)+10 = 60\ \mathrm{s}$

### 4.3 Implemented Trajectory Equations

The ordinary-node coordinates at reception time $t_r$ are calculated as

$$
x(t_r)=x_0+A_x\sin(\omega t_r)
$$

$$
y(t_r)=y_0+V(t_r-t_0)
$$

The propagation distance between the beacon position $(x_R,y_R)$ and the ordinary-node position is

$$p(t_r)=\sqrt{(x(t_r)-x_R)^2+(y(t_r)-y_R)^2}c$$

The corresponding propagation delay is

$$d(t_r)=\frac{p(t_r)}{c}$$

### 4.4 Clock-Parameter Estimation

For each received packet, the timestamp residual is calculated as

$$\Delta t_i= a \prime{t_i}+b-(i-1)T-\frac{p_i}{c}$$

where $T$ is the packet-transmission interval and $p_i$ is the propagation distance associated with the $i$-th packet.

A nonlinear system is constructed from the five received timestamps:

$$
f(\hat{a},\hat{b})=0
$$

The first equation is

$$f_1(\hat{a}, \hat{b}) = \frac{\Delta t_1}{2(t_3 - t_1)(t_2 - t_1)} + \frac{\Delta t_3}{2(t_3 - t_1)(t_3 - t_2)} - \frac{\Delta t_2}{2(t_3 - t_2)(t_2 - t_1)} - \frac{\Delta t_2}{2(t_4 - t_2)(t_3 - t_2)} - \frac{\Delta t_4}{2(t_4 - t_2)(t_4 - t_3)} + \frac{\Delta t_3}{2(t_4 - t_3)(t_3 - t_2)}$$

The second equation is

$$f_2(\hat{a}, \hat{b}) = \frac{\Delta t_1}{2(t_3 - t_1)(t_2 - t_1)} + \frac{\Delta t_3}{2(t_3 - t_1)(t_3 - t_2)} - \frac{\Delta t_2}{2(t_3 - t_2)(t_2 - t_1)} - \frac{\Delta t_3}{2(t_5 - t_3)(t_4 - t_3)} - \frac{\Delta t_5}{2(t_5 - t_3)(t_5 - t_4)} + \frac{\Delta t_4}{2(t_5 - t_4)(t_4 - t_3)}$$

After the nonlinear solver converges, the clock parameters are recovered as

$$\hat{\alpha}=\frac{1}{\hat{a}}$$

$$\hat{\beta}=-\frac{\hat{b}}{\hat{a}}$$

### 4.5 Mobility Model and Trajectory Position-Noise Injection

For the trajectory-position-noise experiments, noise is applied directly to the $x$- and $y$-coordinates of the ordinary-node trajectory.

The node coordinates are updated every 0.5 s according to

$x_N(t)=x_0+A_x sin(\omega t)+n_x(t)$

$y_N(t)=y_0+V(t-t_0)+n_y(t)$

where $n_x(t)$ and $n_y(t)$ are independently generated position-error samples.

This implementation models uncertainty in the trajectory information available to UWGS rather than adding noise only to the distance value after the node position has been calculated.

