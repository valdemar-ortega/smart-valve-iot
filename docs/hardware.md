# Hardware

## Bill of materials

| Qty | Part | Notes |
|---|---|---|
| 1 | ESP32-S3-DevKitC-1 | Any flash size ≥ 4 MB |
| 1 | Analog pressure transducer, 0.5–4.5 V output, 5 V supply | Used here: 0–174 psi (1.2 MPa), 1/4" NPT |
| 1 | 1 kΩ resistor | Divider, top |
| 1 | 2 kΩ resistor | Divider, bottom |
| 1 | Tuya / Smart Life Wi-Fi valve | Controlled through the Tuya cloud, not wired to the ESP32 |
| — | 5 V USB supply, wires | |

Optional but recommended: a 100 nF capacitor from the ADC pin to GND, as
close to the ESP32 as possible, to filter noise picked up by long sensor
cables.

## Wiring

```
             5V ─────────────────────────┐
                                         │
                                  ┌──────┴──────┐
                                  │  Pressure   │
                                  │ transducer  │
                                  └──┬───────┬──┘
                                 SIG │       │ GND
                                     │       │
                                   [1 kΩ]    │
                                     │       │
   ESP32-S3 GPIO4 (ADC1_CH3) ────────┤       │
                                     │       │
                                   [2 kΩ]    │
                                     │       │
   ESP32-S3 GND ─────────────────────┴───────┘
```

| Transducer wire | Connects to |
|---|---|
| Red (V+) | DevKitC **5V** pin |
| Black (GND) | DevKitC **GND** |
| Yellow/green (signal) | 1 kΩ → GPIO4, and 2 kΩ from GPIO4 to GND |

Wire colors vary between vendors: check your sensor's datasheet.

## Why the divider

The ESP32-S3 ADC reads at most ~3.1 V (12 dB attenuation), and its pins are
**not 5 V tolerant**. The divider scales the sensor output by 2/3:

| Sensor output | At GPIO4 |
|---|---|
| 0.5 V (0 psi) | 0.33 V |
| 4.5 V (full scale) | 3.00 V |
| 5.0 V (shorted signal) | 3.33 V → reported as a fault |

The divider draws 1.5 mA at full scale, fine for any transducer rated for
a 1 mA+ load. Its 667 Ω output impedance is low enough for the ADC's
sample-and-hold.

## Calibration

The default calibration (318 mV → 0 psi, 3000 mV → 174 psi) was measured
on the prototype. Resistor tolerance shifts these values, so calibrate
yours:

1. Flash the firmware and open the monitor (`idf.py monitor`).
2. With the line **depressurized**, note the `mV` value in the log. That is
   your *zero* voltage.
3. Either apply a known pressure (a reference gauge) and compute the
   full-scale voltage by linear extrapolation, or keep 3000 mV if you
   have no reference.
4. `idf.py menuconfig` → *Smart Valve configuration* → *Pressure sensor*,
   enter both values, rebuild and flash.

A different sensor range only needs a new *Full-scale pressure* value
(in tenths of psi, e.g. 1000 psi → 10000).

## ADC pin choice

Only **ADC1** pins (GPIO1–GPIO10 on the ESP32-S3) work while Wi-Fi is on;
ADC2 is shared with the radio. The firmware refuses to start if an ADC2
pin is configured.
