/**
 * @file pressure_sensor.c
 * @brief Analog pressure sensor read through the ESP32-S3 ADC1.
 *
 * The sensor outputs 0.5-4.5 V. A 1 kOhm / 2 kOhm divider scales it to
 * roughly 0.33-3.0 V, inside the range of the ADC at 12 dB attenuation.
 * Raw codes are converted to millivolts with the chip's eFuse calibration
 * (curve fitting), which is far more accurate than raw * 3.3 / 4095.
 */
#include "pressure_sensor.h"

#include <stddef.h>

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "pressure_math.h"

static const char *TAG = "sensor";

static adc_oneshot_unit_handle_t s_adc = NULL;
static adc_cali_handle_t s_cali = NULL;
static adc_channel_t s_channel;

static const pressure_cal_t s_cal = {
    .zero_mv = CONFIG_SV_SENSOR_ZERO_MV,
    .full_mv = CONFIG_SV_SENSOR_FULL_MV,
    .full_deci_psi = CONFIG_SV_SENSOR_FULL_SCALE_DECI_PSI,
    .fault_low_mv = CONFIG_SV_SENSOR_FAULT_LOW_MV,
    .fault_high_mv = CONFIG_SV_SENSOR_FAULT_HIGH_MV,
};

esp_err_t pressure_sensor_init(void)
{
    adc_unit_t unit;

    ESP_RETURN_ON_FALSE(pressure_cal_is_valid(&s_cal), ESP_ERR_INVALID_ARG, TAG,
                        "invalid sensor calibration in menuconfig");

    ESP_RETURN_ON_ERROR(adc_oneshot_io_to_channel(CONFIG_SV_SENSOR_GPIO, &unit, &s_channel),
                        TAG, "GPIO%d is not an ADC pin", CONFIG_SV_SENSOR_GPIO);
    ESP_RETURN_ON_FALSE(unit == ADC_UNIT_1, ESP_ERR_INVALID_ARG, TAG,
                        "GPIO%d is on ADC2, which Wi-Fi uses; pick an ADC1 pin",
                        CONFIG_SV_SENSOR_GPIO);

    const adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit_cfg, &s_adc), TAG, "ADC unit");

    const adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(s_adc, s_channel, &chan_cfg), TAG, "ADC channel");

    const adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .chan = s_channel,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_RETURN_ON_ERROR(adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali), TAG,
                        "ADC calibration (eFuse not burned?)");

    ESP_LOGI(TAG, "ADC1 channel %d on GPIO%d ready", (int)s_channel, CONFIG_SV_SENSOR_GPIO);
    return ESP_OK;
}

esp_err_t pressure_sensor_read(pressure_reading_t *out)
{
    int32_t sum = 0;
    int raw = 0;
    int mv = 0;

    ESP_RETURN_ON_FALSE(out != NULL, ESP_ERR_INVALID_ARG, TAG, "null output");
    ESP_RETURN_ON_FALSE(s_adc != NULL, ESP_ERR_INVALID_STATE, TAG, "not initialised");

    for (int32_t i = 0; i < CONFIG_SV_SENSOR_SAMPLES; i++)
    {
        ESP_RETURN_ON_ERROR(adc_oneshot_read(s_adc, s_channel, &raw), TAG, "ADC read");
        sum += raw;
    }

    const int32_t avg_raw = sum / CONFIG_SV_SENSOR_SAMPLES;
    ESP_RETURN_ON_ERROR(adc_cali_raw_to_voltage(s_cali, (int)avg_raw, &mv), TAG, "raw to mV");

    const pressure_result_t p = pressure_from_mv(&s_cal, (int32_t)mv);

    out->deci_psi = p.deci_psi;
    out->pin_mv = (int32_t)mv;
    out->raw = avg_raw;
    out->fault = p.fault;

    return ESP_OK;
}
