/**
 * @file app_main.c
 * @brief Smart Valve pressure node: sample the sensor, publish over MQTT.
 *
 * Start-up order: NVS (Wi-Fi calibration data) -> event loop -> sensor ->
 * Wi-Fi -> MQTT. After that a single task samples at a fixed rate with
 * vTaskDelayUntil, so network hiccups never shift or stop the readings.
 */
#include <stdint.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#include "mqtt_link.h"
#include "pressure_sensor.h"
#include "wifi_sta.h"

#define SAMPLER_STACK_BYTES (4096U)
#define SAMPLER_PRIORITY    (5U)

static const char *TAG = "app";

static void sampler_task(void *arg)
{
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(CONFIG_SV_SAMPLE_PERIOD_MS);
    pressure_reading_t r;

    for (;;)
    {
        if (pressure_sensor_read(&r) == ESP_OK)
        {
            if (r.fault)
            {
                ESP_LOGW(TAG, "SENSOR FAULT: %ld mV at the pin (raw %ld), check wiring",
                         (long)r.pin_mv, (long)r.raw);
            }
            else
            {
                ESP_LOGI(TAG, "%ld.%ld psi | %ld mV | raw %ld",
                         (long)(r.deci_psi / 10), (long)(r.deci_psi % 10),
                         (long)r.pin_mv, (long)r.raw);
            }

            /* Offline is normal while Wi-Fi or the broker reconnects. */
            (void)mqtt_link_publish_reading(&r);
        }

        vTaskDelayUntil(&last_wake, period);
    }
}

static void on_link_change(bool connected)
{
    ESP_LOGI(TAG, "network %s", connected ? "up" : "down");
}

static esp_err_t init_nvs(void)
{
    esp_err_t err = nvs_flash_init();

    if ((err == ESP_ERR_NVS_NO_FREE_PAGES) || (err == ESP_ERR_NVS_NEW_VERSION_FOUND))
    {
        /* Partition layout changed: wipe and start clean. */
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "nvs erase");
        err = nvs_flash_init();
    }

    return err;
}

void app_main(void)
{
    ESP_ERROR_CHECK(init_nvs());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(pressure_sensor_init());
    ESP_ERROR_CHECK(wifi_sta_start(on_link_change));
    ESP_ERROR_CHECK(mqtt_link_start());

    const BaseType_t ok = xTaskCreate(sampler_task, "sampler", SAMPLER_STACK_BYTES, NULL,
                                      SAMPLER_PRIORITY, NULL);
    configASSERT(ok == pdPASS);

    ESP_LOGI(TAG, "running: one reading every %d ms", CONFIG_SV_SAMPLE_PERIOD_MS);
}
