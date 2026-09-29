/**
 * @file pressure_math.c
 * @brief Pure conversion from sensor voltage to pressure.
 */
#include "pressure_math.h"

#include <stddef.h>

bool pressure_cal_is_valid(const pressure_cal_t *cal)
{
    bool valid = false;

    if (cal != NULL)
    {
        valid = (cal->zero_mv >= 0) &&
                (cal->full_mv > cal->zero_mv) &&
                (cal->full_deci_psi > 0) &&
                (cal->fault_low_mv < cal->fault_high_mv);
    }

    return valid;
}

pressure_result_t pressure_from_mv(const pressure_cal_t *cal, int32_t mv)
{
    pressure_result_t result = { .deci_psi = 0, .fault = true };

    if (pressure_cal_is_valid(cal) &&
        (mv >= cal->fault_low_mv) &&
        (mv <= cal->fault_high_mv))
    {
        const int32_t span_mv = cal->full_mv - cal->zero_mv;
        int32_t offset_mv = mv - cal->zero_mv;

        /* Clamp to the calibrated span: small noise below zero reads as 0. */
        if (offset_mv < 0)
        {
            offset_mv = 0;
        }
        else if (offset_mv > span_mv)
        {
            offset_mv = span_mv;
        }
        else
        {
            /* Inside the span: nothing to clamp. */
        }

        /* 64-bit intermediate: offset * full scale can exceed INT32_MAX
         * with unusual calibrations. Adding half the divisor rounds to
         * the nearest tenth instead of truncating. */
        const int64_t scaled = ((int64_t)offset_mv * cal->full_deci_psi) + (span_mv / 2);

        result.deci_psi = (int32_t)(scaled / span_mv);
        result.fault = false;
    }

    return result;
}
