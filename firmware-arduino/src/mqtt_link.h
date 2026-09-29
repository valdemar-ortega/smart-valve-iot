#pragma once
#include "pressure_sensor.h"

void mqttBegin();                            // TLS client + broker address
void mqttKeep();                             // call from loop(); never blocks
void mqttPublish(const PressureReading& r);  // retained JSON on the pressure topic
