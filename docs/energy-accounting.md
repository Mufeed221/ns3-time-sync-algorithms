# Energy Accounting

This document records the ns-3 energy-model configuration, measurement procedure, reporting format, and long-term energy calculations used in the synchronization experiments.

## 1. Energy Models

The implementation uses the following ns-3 energy components:

* `BasicEnergySource`
* `AcousticModemEnergyModelHelper`
* `DeviceEnergyModelContainer`

A `BasicEnergySource` is installed on each underwater node. The acoustic modem energy model is then attached to the corresponding UAN network device.

The implementation should preserve the following attachment order:

1. Create the underwater nodes.
2. Install the UAN network devices.
3. Install a `BasicEnergySource` on each node.
4. Configure the acoustic modem power-state values.
5. Attach the acoustic modem energy model to each UAN device and its associated energy source.
6. Read the remaining energy at the beginning and end of the selected measurement window.

The relevant helper configuration follows the general form:

```cpp
BasicEnergySourceHelper energySource;
energySource.Set("BasicEnergySourceInitialEnergyJ",
                 DoubleValue(1000000.0));
energySource.Set("BasicEnergySupplyVoltageV",
                 DoubleValue(12.0));

EnergySourceContainer sources =
    energySource.Install(nodes);

AcousticModemEnergyModelHelper modemEnergy;

modemEnergy.Set("TxPowerW", DoubleValue(50.0));
modemEnergy.Set("RxPowerW", DoubleValue(0.158));
modemEnergy.Set("IdlePowerW", DoubleValue(0.158));
modemEnergy.Set("SleepPowerW", DoubleValue(0.0058));
```

The exact attribute names should be checked against the version used in ns-3.40 and the corresponding implementation source file.

## 2. Modem Power States

The acoustic modem uses the following power-state configuration.

| State            |  Power |
| ---------------- | -----: |
| Transmission, TX |   50 W |
| Reception, RX    | 158 mW |
| Idle             | 158 mW |
| Sleep            | 5.8 mW |

In watts, the receive, idle, and sleep values are

$$P_{\mathrm{RX}}=0.158\ \mathrm{W}$$

$$P_{\mathrm{idle}}=0.158\ \mathrm{W}$$

$$P_{\mathrm{sleep}}=0.0058\ \mathrm{W}$$

The modem energy model integrates the power consumed in each state over the amount of time spent in that state.

For a state with constant power $P_s$ and duration $\Delta t_s$, the consumed energy is

$$E_s=P_s\Delta t_s$$

The total energy consumed during a measurement window is therefore

$E_{\mathrm{total}} = E_{\mathrm{TX}} + E_{\mathrm{RX}} + E_{\mathrm{idle}} + E_{\mathrm{sleep}}$

## 3. Energy Source Configuration

Each node is initialized using the following energy-source configuration:

```text
Initial energy: 1,000,000 J
Supply voltage: 12 V
```

Thus,

$E_{\mathrm{initial,node}} = 1\ \mathrm{MJ}$

The large initial-energy value prevents node depletion during a simulation and allows the energy model to be used primarily as an accounting mechanism.

## 4. Measurement Window

Energy consumption is measured over one complete synchronization procedure.

The measurement begins when the synchronization application starts and continues until the final required synchronization message is received. Protocol-required request intervals, waiting periods, and reply-backoff intervals occurring during this procedure are therefore included in the measured energy.

The measurement window is defined as

$T_{\mathrm{window}} = t_{\mathrm{last\ reception}} - t_{\mathrm{application\ start}}$

where $t_{\mathrm{application\ start}}$ is the application start time and $t_{\mathrm{last\ reception}}$ is the reception time of the final message required by the protocol.

Energy consumed after the final synchronization message is received is not included. This behavior was verified by increasing the total simulation duration and observing that the reported energy values remained unchanged.

The same measurement boundaries are applied to the beacon node and the ordinary node.


## 5. Consumed Energy

For one node, the consumed energy is

$E_{\mathrm{consumed,node}} = E_{\mathrm{start,node}} + E_{\mathrm{end,node}}$

When the measurement begins at simulation time zero, this may equivalently be written as

$E_{\mathrm{consumed,node}} = E_{\mathrm{initial,node}} + E_{\mathrm{remaining,node}}$

The energy consumed by the complete two-node synchronization pair is

$E_{\mathrm{protocol}} = E_{\mathrm{beacon}} + E_{\mathrm{ordinary}}$

