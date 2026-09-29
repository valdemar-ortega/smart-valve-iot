/**
 * @file test_pressure_math.c
 * @brief Host unit tests for pressure_math.c (no hardware needed).
 *
 * Build and run:  make -C firmware/test
 */
#include <stdio.h>
#include <stdlib.h>

#include "pressure_math.h"

static int s_failures = 0;
static int s_checks = 0;

#define CHECK_EQ(expected, actual)                                                  \
    do                                                                              \
    {                                                                               \
        s_checks++;                                                                 \
        const long e_ = (long)(expected);                                           \
        const long a_ = (long)(actual);                                             \
        if (e_ != a_)                                                               \
        {                                                                           \
            s_failures++;                                                           \
            (void)printf("  FAIL %s:%d: expected %ld, got %ld\n", __FILE__, __LINE__, \
                         e_, a_);                                                   \
        }                                                                           \
    } while (0)

/* The default calibration from Kconfig. */
static const pressure_cal_t CAL = {
    .zero_mv = 318,
    .full_mv = 3000,
    .full_deci_psi = 1740,
    .fault_low_mv = 150,
    .fault_high_mv = 3100,
};

static void test_zero_voltage_is_zero_psi(void)
{
    const pressure_result_t r = pressure_from_mv(&CAL, 318);
    CHECK_EQ(0, r.deci_psi);
    CHECK_EQ(0, r.fault);
}

static void test_full_voltage_is_full_scale(void)
{
    const pressure_result_t r = pressure_from_mv(&CAL, 3000);
    CHECK_EQ(1740, r.deci_psi);
    CHECK_EQ(0, r.fault);
}

static void test_midpoint_is_half_scale(void)
{
    /* (318 + 3000) / 2 = 1659 mV -> 87.0 psi */
    const pressure_result_t r = pressure_from_mv(&CAL, 1659);
    CHECK_EQ(870, r.deci_psi);
}

static void test_rounds_to_nearest_tenth(void)
{
    /* 1000 mV -> (682 * 1740) / 2682 = 442.46 -> 44.2 psi */
    CHECK_EQ(442, pressure_from_mv(&CAL, 1000).deci_psi);
    /* 1001 mV -> 443.11 -> 44.3 psi */
    CHECK_EQ(443, pressure_from_mv(&CAL, 1001).deci_psi);
}

static void test_noise_below_zero_clamps_to_zero(void)
{
    const pressure_result_t r = pressure_from_mv(&CAL, 250);
    CHECK_EQ(0, r.deci_psi);
    CHECK_EQ(0, r.fault);
}

static void test_above_full_scale_clamps(void)
{
    const pressure_result_t r = pressure_from_mv(&CAL, 3050);
    CHECK_EQ(1740, r.deci_psi);
    CHECK_EQ(0, r.fault);
}

static void test_open_wire_is_fault(void)
{
    const pressure_result_t r = pressure_from_mv(&CAL, 20);
    CHECK_EQ(1, r.fault);
    CHECK_EQ(0, r.deci_psi);
}

static void test_short_to_supply_is_fault(void)
{
    const pressure_result_t r = pressure_from_mv(&CAL, 3250);
    CHECK_EQ(1, r.fault);
    CHECK_EQ(0, r.deci_psi);
}

static void test_fault_window_edges_are_valid(void)
{
    CHECK_EQ(0, pressure_from_mv(&CAL, 150).fault);
    CHECK_EQ(0, pressure_from_mv(&CAL, 3100).fault);
    CHECK_EQ(1, pressure_from_mv(&CAL, 149).fault);
    CHECK_EQ(1, pressure_from_mv(&CAL, 3101).fault);
}

static void test_invalid_calibration_is_rejected(void)
{
    pressure_cal_t bad = CAL;

    bad.full_mv = bad.zero_mv; /* zero span would divide by zero */
    CHECK_EQ(0, pressure_cal_is_valid(&bad));
    CHECK_EQ(1, pressure_from_mv(&bad, 1000).fault);

    CHECK_EQ(0, pressure_cal_is_valid(NULL));
    CHECK_EQ(1, pressure_from_mv(NULL, 1000).fault);
}

static void test_large_scale_does_not_overflow(void)
{
    /* 10 000 psi sensor: offset * scale exceeds INT32_MAX without int64. */
    const pressure_cal_t big = {
        .zero_mv = 0,
        .full_mv = 3000,
        .full_deci_psi = 1000000,
        .fault_low_mv = 0,
        .fault_high_mv = 3100,
    };
    CHECK_EQ(1000000, pressure_from_mv(&big, 3000).deci_psi);
}

int main(void)
{
    test_zero_voltage_is_zero_psi();
    test_full_voltage_is_full_scale();
    test_midpoint_is_half_scale();
    test_rounds_to_nearest_tenth();
    test_noise_below_zero_clamps_to_zero();
    test_above_full_scale_clamps();
    test_open_wire_is_fault();
    test_short_to_supply_is_fault();
    test_fault_window_edges_are_valid();
    test_invalid_calibration_is_rejected();
    test_large_scale_does_not_overflow();

    (void)printf("%d checks, %d failures\n", s_checks, s_failures);
    return (s_failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
