// Smart Valve pressure node — Arduino prototype.
// Reads the pressure sensor every 2 s and publishes it over MQTT/TLS.
// Nothing in loop() blocks, so readings continue while Wi-Fi or the
// broker reconnect.

#include <Arduino.h>
#include "wifi_link.h"
#include "pressure_sensor.h"
#include "mqtt_link.h"

static const unsigned long SAMPLE_PERIOD_MS = 2000;
static unsigned long lastSample = 0;

void setup() {
    Serial.begin(115200);
    delay(500);

    sensorBegin();
    wifiBegin();
    mqttBegin();

    Serial.println("[MAIN] Ready, publishing over MQTT");
}

void loop() {
    wifiKeep();
    mqttKeep();

    unsigned long now = millis();
    if (now - lastSample >= SAMPLE_PERIOD_MS) {
        lastSample = now;

        PressureReading r = sensorRead();
        if (r.fault) {
            Serial.printf("[SENSOR] FAULT: %.3f V at the pin, check wiring\n", r.volts);
        } else {
            Serial.printf("[SENSOR] %.1f psi | %.3f V | raw %d\n", r.psi, r.volts, r.raw);
        }

        mqttPublish(r);
    }
}
