/**
 * @file wifi_sta.h
 * @brief Wi-Fi station with automatic reconnection.
 */
#ifndef WIFI_STA_H
#define WIFI_STA_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/** Called from the event task whenever the link goes up or down. */
typedef void (*wifi_sta_link_cb_t)(bool connected);

/**
 * @brief Start the station and connect in the background.
 *
 * Never blocks waiting for the network: the rest of the firmware keeps
 * running while the link is down. Reconnection uses exponential backoff.
 *
 * @param on_link Optional callback for link changes (may be NULL).
 */
esp_err_t wifi_sta_start(wifi_sta_link_cb_t on_link);

/** @return true while the station has an IP address. */
bool wifi_sta_is_connected(void);

#endif /* WIFI_STA_H */
