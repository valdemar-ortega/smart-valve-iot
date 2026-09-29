#pragma once

struct PressureReading {
    float psi;     // pressure; 0 when faulted
    float volts;   // voltage at the ADC pin (after the divider)
    int   raw;     // averaged raw ADC code
    bool  fault;   // open wire, no sensor, or short / over-range
};

void sensorBegin();
PressureReading sensorRead();
