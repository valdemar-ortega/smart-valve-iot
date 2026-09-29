#include <Arduino.h>
#include "pressure_sensor.h"

static const int SENSOR_PIN = 4;   // ADC1_CH3; ADC2 pins do not work with Wi-Fi on

// ------------------------------------------------------------------
// Calibration, measured AT THE PIN (after the 1k/2k divider):
//   V_ZERO = 0.318 V -> 0 psi
//   V_FULL = 3.000 V -> 174 psi (full scale)
// A different divider or sensor only needs these three values changed.
// ------------------------------------------------------------------
static const float V_ZERO  = 0.318f;
static const float V_FULL  = 3.0f;
static const float PSI_MAX = 174.0f;

// Nominal ADC reference. The ESP-IDF version uses the chip's eFuse
// calibration instead, which is more accurate.
static const float V_REF   = 3.30f;
static const int   ADC_MAX = 4095;

// Outside this window the reading is a wiring fault, not a pressure.
static const float V_FAULT_LOW  = 0.15f;
static const float V_FAULT_HIGH = 3.25f;

void sensorBegin() {
    analogReadResolution(12);
    analogSetPinAttenuation(SENSOR_PIN, ADC_11db);
    Serial.println("[SENSOR] ADC ready on GPIO4");
}

PressureReading sensorRead() {
    PressureReading r = {0.0f, 0.0f, 0, false};

    // Average 20 samples to smooth ADC noise.
    long sum = 0;
    for (int i = 0; i < 20; i++) {
        sum += analogRead(SENSOR_PIN);
        delay(2);
    }
    r.raw = sum / 20;
    r.volts = (r.raw * V_REF) / ADC_MAX;

    if (r.volts < V_FAULT_LOW || r.volts > V_FAULT_HIGH) {
        r.fault = true;
        return r;
    }

    // Linear map V_ZERO..V_FULL -> 0..PSI_MAX, clamped.
    r.psi = ((r.volts - V_ZERO) / (V_FULL - V_ZERO)) * PSI_MAX;
    r.psi = constrain(r.psi, 0.0f, PSI_MAX);

    return r;
}
