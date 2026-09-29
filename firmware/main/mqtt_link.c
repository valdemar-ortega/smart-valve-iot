/**
 * @file mqtt_link.c
 * @brief TLS MQTT connection that publishes pressure readings.
 *
 * - The broker certificate is verified with the ESP-IDF CA bundle.
 * - A retained Last Will ("offline") on the status topic lets the app know
 *   the board disappeared; "online" is published on every (re)connect.
 * - esp-mqtt runs in its own task and reconnects by itself, so nothing
 *   here ever blocks the sampling loop.
 */
#include "mqtt_link.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_check.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "mqtt_client.h"
#include "sdkconfig.h"

#define STATUS_ONLINE   "online"
#define STATUS_OFFLINE  "offline"
#define QOS_AT_LEAST_ONCE (1)
#define RETAIN            (1)

static const char *TAG = "mqtt";

static esp_mqtt_client_handle_t s_client = NULL;
static volatile bool s_connected = false;

static void on_mqtt_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;
    const esp_mqtt_event_handle_t ev = (esp_mqtt_event_handle_t)data;

    switch ((esp_mqtt_event_id_t)id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "connected to broker");
            s_connected = true;
            (void)esp_mqtt_client_publish(s_client, CONFIG_SV_MQTT_TOPIC_STATUS, STATUS_ONLINE,
                                          0, QOS_AT_LEAST_ONCE, RETAIN);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "disconnected, esp-mqtt will retry");
            s_connected = false;
            break;

        case MQTT_EVENT_ERROR:
            if ((ev != NULL) && (ev->error_handle != NULL) &&
                (ev->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED))
            {
                ESP_LOGE(TAG, "broker refused the connection (code %d): check username/password",
                         (int)ev->error_handle->connect_return_code);
            }
            else
            {
                ESP_LOGE(TAG, "transport error");
            }
            break;

        default:
            /* Subscribe/publish acknowledgements are not needed. */
            break;
    }
}

esp_err_t mqtt_link_start(void)
{
    const esp_mqtt_client_config_t cfg = {
        .broker = {
            .address.uri = CONFIG_SV_MQTT_BROKER_URI,
            .verification.crt_bundle_attach = esp_crt_bundle_attach,
        },
        .credentials = {
            .username = CONFIG_SV_MQTT_USERNAME,
            .authentication.password = CONFIG_SV_MQTT_PASSWORD,
        },
        .session.last_will = {
            .topic = CONFIG_SV_MQTT_TOPIC_STATUS,
            .msg = STATUS_OFFLINE,
            .qos = QOS_AT_LEAST_ONCE,
            .retain = RETAIN,
        },
    };

    s_client = esp_mqtt_client_init(&cfg);
    ESP_RETURN_ON_FALSE(s_client != NULL, ESP_ERR_NO_MEM, TAG, "client init");

    ESP_RETURN_ON_ERROR(esp_mqtt_client_register_event(s_client, MQTT_EVENT_ANY, on_mqtt_event, NULL),
                        TAG, "event handler");
    ESP_RETURN_ON_ERROR(esp_mqtt_client_start(s_client), TAG, "start");

    return ESP_OK;
}

bool mqtt_link_is_connected(void)
{
    return s_connected;
}

esp_err_t mqtt_link_publish_reading(const pressure_reading_t *reading)
{
    char payload[128];

    ESP_RETURN_ON_FALSE(reading != NULL, ESP_ERR_INVALID_ARG, TAG, "null reading");
    if (!s_connected)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* Integer formatting keeps the float printf code out of the image. */
    const int32_t psi = reading->deci_psi;
    const int32_t mv = reading->pin_mv;
    const int len = snprintf(payload, sizeof(payload),
                             "{\"psi\":%ld.%01ld,\"voltage\":%ld.%03ld,\"raw\":%ld,"
                             "\"fault\":%s,\"uptime_ms\":%lld}",
                             (long)(psi / 10), (long)(psi % 10),
                             (long)(mv / 1000), (long)(mv % 1000),
                             (long)reading->raw,
                             reading->fault ? "true" : "false",
                             (long long)(esp_timer_get_time() / 1000));
    ESP_RETURN_ON_FALSE((len > 0) && ((size_t)len < sizeof(payload)), ESP_ERR_INVALID_SIZE, TAG,
                        "payload truncated");

    /* Retained: an app that subscribes later gets the last value at once. */
    const int msg_id = esp_mqtt_client_publish(s_client, CONFIG_SV_MQTT_TOPIC_PRESSURE, payload,
                                               len, QOS_AT_LEAST_ONCE, RETAIN);
    ESP_RETURN_ON_FALSE(msg_id >= 0, ESP_FAIL, TAG, "publish failed");

    return ESP_OK;
}
