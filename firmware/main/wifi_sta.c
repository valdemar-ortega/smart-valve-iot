/**
 * @file wifi_sta.c
 * @brief Wi-Fi station with automatic reconnection.
 *
 * Event driven: the default event loop tells us when the link drops and a
 * one-shot esp_timer schedules the next attempt, doubling the delay each
 * time (1 s, 2 s, 4 s ... up to CONFIG_SV_WIFI_RETRY_MAX_DELAY_MS). The
 * delay resets once an IP is obtained.
 */
#include "wifi_sta.h"

#include <stddef.h>
#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"

#define RETRY_FIRST_DELAY_MS (1000U)

static const char *TAG = "wifi";

static esp_timer_handle_t s_retry_timer = NULL;
static wifi_sta_link_cb_t s_on_link = NULL;
static uint32_t s_retry_delay_ms = RETRY_FIRST_DELAY_MS;
static volatile bool s_connected = false;

static void retry_timer_cb(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "reconnecting...");
    (void)esp_wifi_connect();
}

static void schedule_retry(void)
{
    ESP_LOGW(TAG, "link down, next attempt in %u ms", (unsigned)s_retry_delay_ms);
    (void)esp_timer_start_once(s_retry_timer, (uint64_t)s_retry_delay_ms * 1000U);

    s_retry_delay_ms *= 2U;
    if (s_retry_delay_ms > (uint32_t)CONFIG_SV_WIFI_RETRY_MAX_DELAY_MS)
    {
        s_retry_delay_ms = (uint32_t)CONFIG_SV_WIFI_RETRY_MAX_DELAY_MS;
    }
}

static void set_link(bool connected)
{
    if (s_connected != connected)
    {
        s_connected = connected;
        if (s_on_link != NULL)
        {
            s_on_link(connected);
        }
    }
}

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;

    if (id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "connecting to \"%s\"", CONFIG_SV_WIFI_SSID);
        (void)esp_wifi_connect();
    }
    else if (id == WIFI_EVENT_STA_DISCONNECTED)
    {
        const wifi_event_sta_disconnected_t *ev = (const wifi_event_sta_disconnected_t *)data;
        ESP_LOGW(TAG, "disconnected (reason %d)", (int)ev->reason);
        set_link(false);
        schedule_retry();
    }
    else
    {
        /* Other Wi-Fi events are not needed. */
    }
}

static void on_ip_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;

    if (id == IP_EVENT_STA_GOT_IP)
    {
        const ip_event_got_ip_t *ev = (const ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "got IP " IPSTR, IP2STR(&ev->ip_info.ip));
        s_retry_delay_ms = RETRY_FIRST_DELAY_MS;
        set_link(true);
    }
}

esp_err_t wifi_sta_start(wifi_sta_link_cb_t on_link)
{
    s_on_link = on_link;

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "netif");
    (void)esp_netif_create_default_wifi_sta();

    const wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&init_cfg), TAG, "wifi init");

    const esp_timer_create_args_t timer_args = {
        .callback = retry_timer_cb,
        .name = "wifi_retry",
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&timer_args, &s_retry_timer), TAG, "retry timer");

    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                            on_wifi_event, NULL, NULL),
                        TAG, "wifi handler");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                            on_ip_event, NULL, NULL),
                        TAG, "ip handler");

    wifi_config_t cfg = { 0 };
    (void)strncpy((char *)cfg.sta.ssid, CONFIG_SV_WIFI_SSID, sizeof(cfg.sta.ssid));
    (void)strncpy((char *)cfg.sta.password, CONFIG_SV_WIFI_PASSWORD, sizeof(cfg.sta.password));
    cfg.sta.threshold.authmode = (strlen(CONFIG_SV_WIFI_PASSWORD) == 0U) ? WIFI_AUTH_OPEN
                                                                         : WIFI_AUTH_WPA2_PSK;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &cfg), TAG, "config");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "start");

    return ESP_OK;
}

bool wifi_sta_is_connected(void)
{
    return s_connected;
}
