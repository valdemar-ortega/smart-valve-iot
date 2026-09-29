/**
 * @file mqtt_link.h
 * @brief TLS MQTT connection that publishes pressure readings.
 */
#ifndef MQTT_LINK_H
#define MQTT_LINK_H

#include <stdbool.h>

#include "esp_err.h"
#include "pressure_sensor.h"

/**
 * @brief Create the client. It connects on its own once the network is up
 *        and reconnects after any drop.
 */
esp_err_t mqtt_link_start(void);

/** @return true while the broker session is open. */
bool mqtt_link_is_connected(void);

/**
 * @brief Publish one reading as retained JSON on the pressure topic.
 * @return ESP_ERR_INVALID_STATE when offline (the reading is dropped: the
 *         next one supersedes it, so there is no point in queueing).
 */
esp_err_t mqtt_link_publish_reading(const pressure_reading_t *reading);

#endif /* MQTT_LINK_H */
