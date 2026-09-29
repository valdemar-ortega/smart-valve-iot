/**
 * @file pressure_sensor.h
 * @brief Analog pressure sensor read through the ESP32-S3 ADC1.
 */
#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/** One averaged reading. */
typedef struct
{
    int32_t deci_psi;  /**< Pressure, tenths of psi (0 when faulted). */
    int32_t pin_mv;    /**< Calibrated voltage at the GPIO, millivolts. */
    int32_t raw;       /**< Averaged raw ADC code (0..4095). */
    bool    fault;     /**< Open wire, short or over-range. */
} pressure_reading_t;

/**
 * @brief Configure the ADC channel and its calibration scheme.
 *
 * Settings come from menuconfig ("Smart Valve configuration").
 */
esp_err_t pressure_sensor_init(void);

/**
 * @brief Take CONFIG_SV_SENSOR_SAMPLES samples, average them and convert.
 * @param[out] out Filled on success.
 */
esp_err_t pressure_sensor_read(pressure_reading_t *out);

#endif /* PRESSURE_SENSOR_H */
