#include "mqtt_link.h"
#include "secrets.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

static WiFiClientSecure tls;
static PubSubClient mqtt(tls);

static unsigned long lastAttempt = 0;

// ONE connection attempt, never a loop: an unreachable broker must not stop
// the sensor readings (that was the bug in the very first version).
static bool tryConnect() {
    if (WiFi.status() != WL_CONNECTED) return false;

    String id = "esp32-" + String((uint32_t)esp_random(), HEX);
    Serial.print("[MQTT] Connecting... ");

    // Last Will: if the board drops off, the broker publishes "offline"
    // (retained) on the status topic so the app can tell.
    if (mqtt.connect(id.c_str(), MQTT_USER, MQTT_PASS,
                     MQTT_TOPIC_STATUS, 0, true, "offline")) {
        Serial.println("OK");
        mqtt.publish(MQTT_TOPIC_STATUS, "online", true);
        return true;
    }

    Serial.printf("failed, rc=%d\n", mqtt.state());
    return false;
}

void mqttBegin() {
    // setInsecure(): TLS encryption WITHOUT checking the broker certificate.
    // Simple for a prototype; the ESP-IDF version verifies it against a CA
    // bundle. Here that would be tls.setCACert(<ISRG Root X1 PEM>).
    tls.setInsecure();
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setBufferSize(512);
}

void mqttKeep() {
    if (!mqtt.connected()) {
        unsigned long now = millis();
        if (now - lastAttempt >= 3000) {   // at most one attempt every 3 s
            lastAttempt = now;
            tryConnect();
        }
        return;
    }
    mqtt.loop();
}

void mqttPublish(const PressureReading& r) {
    if (!mqtt.connected()) return;

    // Same JSON contract as the ESP-IDF firmware (docs/protocol.md).
    JsonDocument doc;
    doc["psi"]       = roundf(r.psi * 10.0f) / 10.0f;
    doc["voltage"]   = roundf(r.volts * 1000.0f) / 1000.0f;
    doc["raw"]       = r.raw;
    doc["fault"]     = r.fault;
    doc["uptime_ms"] = millis();

    char buf[160];
    size_t n = serializeJson(doc, buf);
    // Retained: a client that subscribes later gets the last value at once.
    bool ok = mqtt.publish(MQTT_TOPIC_PRESSURE, (const uint8_t*)buf, n, true);
    Serial.printf("[MQTT] publish -> %s\n", ok ? "ok" : "FAILED");
}
