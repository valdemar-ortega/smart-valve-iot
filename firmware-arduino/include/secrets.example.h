// Copy this file to include/secrets.h and fill in your values.
// secrets.h is git-ignored, so your credentials never reach the repo.
#pragma once

// --- Wi-Fi (2.4 GHz) ---
#define WIFI_SSID     "my-network"
#define WIFI_PASSWORD "my-password"

// --- MQTT broker (TLS) ---
#define MQTT_HOST     "your-cluster.s1.eu.hivemq.cloud"
#define MQTT_PORT     8883
#define MQTT_USER     "your-mqtt-user"
#define MQTT_PASS     "your-mqtt-password"

// Same topics as the ESP-IDF firmware and the app (see docs/protocol.md).
#define MQTT_TOPIC_PRESSURE "smartvalve/pressure"
#define MQTT_TOPIC_STATUS   "smartvalve/status"
