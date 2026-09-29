/**
 * @file pressure_math.h
 * @brief Pure conversion from sensor voltage to pressure.
 *
 * No ESP-IDF dependencies: this module is compiled and unit-tested on the
 * host (see firmware/test).
 */
#ifndef PRESSURE_MATH_H
#define PRESSURE_MATH_H

#include <stdbool.h>
#include <stdint.h>

/** Linear calibration of a ratiometric 0.5-4.5 V sensor behind a divider. */
typedef struct
{
    int32_t zero_mv;        /**< Pin voltage at 0 psi.                  */
    int32_t full_mv;        /**< Pin voltage at full scale.             */
    int32_t full_deci_psi;  /**< Full-scale pressure, tenths of psi.    */
    int32_t fault_low_mv;   /**< Below this: open wire / no sensor.     */
    int32_t fault_high_mv;  /**< Above this: short / over-range.        */
} pressure_cal_t;

/** Result of converting one averaged voltage sample. */
typedef struct
{
    int32_t deci_psi;  /**< Pressure in tenths of psi, clamped to [0, full scale]. */
    bool    fault;     /**< True when the voltage is outside the fault window.     */
} pressure_result_t;

/**
 * @brief Check that a calibration is usable.
 * @return true if zero < full, full scale > 0 and the fault window is ordered.
 */
bool pressure_cal_is_valid(const pressure_cal_t *cal);

/**
 * @brief Convert a pin voltage to pressure.
 *
 * Uses integer arithmetic only. A faulted reading reports 0 psi so a
 * consumer that ignores the flag never shows a stale or bogus value.
 *
 * @param cal Calibration (must satisfy pressure_cal_is_valid()).
 * @param mv  Averaged pin voltage in millivolts.
 */
pressure_result_t pressure_from_mv(const pressure_cal_t *cal, int32_t mv);

#endif /* PRESSURE_MATH_H */
