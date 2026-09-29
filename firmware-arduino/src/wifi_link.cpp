#include <Arduino.h>
#include <WiFi.h>
#include "wifi_link.h"
#include "secrets.h"

void wifiBegin() {
    Serial.printf("[WiFi] Connecting to '%s'", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Wait up to 20 s but do NOT reboot on failure: the sensor keeps being
    // read and wifiKeep() retries in the background.
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
        delay(400);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi] Connected, IP %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n[WiFi] Timeout; will keep retrying in the background");
    }
}

void wifiKeep() {
    static unsigned long lastAttempt = 0;

    if (WiFi.status() == WL_CONNECTED) return;

    unsigned long now = millis();
    if (now - lastAttempt >= 5000) {
        lastAttempt = now;
        Serial.println("[WiFi] Link down, retrying...");
        WiFi.disconnect();
        WiFi.reconnect();
    }
}
