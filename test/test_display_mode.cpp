/*
 * test/test_display_mode.cpp
 *
 * Unit tests for NextDisplayMode() and PreviousDisplayMode() — M6 requirement.
 *
 * These are pure functions defined in src/input.cpp with no hardware
 * dependencies.  They navigate through the four DisplayMode pages
 * (TEMPERATURE → HUMIDITY → LIGHT → MOTION) with wraparound.
 */
#include <unity.h>
#include "input.h"

void setup() {
    UNITY_BEGIN();
}

void loop() {
    /* NextDisplayMode tests */
    RUN_TEST(test_next_temperature_to_humidity);
    RUN_TEST(test_next_motion_wraps_to_temperature);

    /* PreviousDisplayMode tests */
    RUN_TEST(test_previous_humidity_to_temperature);
    RUN_TEST(test_previous_temperature_wraps_to_motion);

    UNITY_END();
}

/* ── 6. TEMPERATURE → HUMIDITY ───────────────────────── */
void test_next_temperature_to_humidity(void) {
    TEST_ASSERT_EQUAL(DisplayMode::HUMIDITY,
                      NextDisplayMode(DisplayMode::TEMPERATURE));
}

/* ── 7. MOTION → TEMPERATURE (wraparound) ────────────── */
void test_next_motion_wraps_to_temperature(void) {
    TEST_ASSERT_EQUAL(DisplayMode::TEMPERATURE,
                      NextDisplayMode(DisplayMode::MOTION));
}

/* ── 8. HUMIDITY → TEMPERATURE ───────────────────────── */
void test_previous_humidity_to_temperature(void) {
    TEST_ASSERT_EQUAL(DisplayMode::TEMPERATURE,
                      PreviousDisplayMode(DisplayMode::HUMIDITY));
}

/* ── 9. TEMPERATURE → MOTION (wraparound) ────────────── */
void test_previous_temperature_wraps_to_motion(void) {
    TEST_ASSERT_EQUAL(DisplayMode::MOTION,
                      PreviousDisplayMode(DisplayMode::TEMPERATURE));
}
