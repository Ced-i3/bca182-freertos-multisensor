/*
 * test/test_temperature.cpp
 *
 * Unit tests for EvaluateTemperature() — M6 requirement.
 *
 * The function is the pure decision logic extracted from AlarmTask.
 * It returns true when temperature is outside [18, 30] AND state is ACTIVE.
 */
#include <unity.h>
#include "alarm.h"

void setup() {
    UNITY_BEGIN();
}

void loop() {
    RUN_TEST(test_below_threshold);
    RUN_TEST(test_at_lower_bound);
    RUN_TEST(test_in_normal_range);
    RUN_TEST(test_at_upper_bound);
    RUN_TEST(test_above_threshold);
    UNITY_END();
}

/* ── 1. Below 18 °C → alarm ON ──────────────────────── */
void test_below_threshold(void) {
    TEST_ASSERT_TRUE(EvaluateTemperature(15.0f, ActivityState::ACTIVE));
}

/* ── 2. Exactly 18 °C → alarm OFF (boundary) ────────── */
void test_at_lower_bound(void) {
    TEST_ASSERT_FALSE(EvaluateTemperature(18.0f, ActivityState::ACTIVE));
}

/* ── 3. Normal temperature (24 °C) → alarm OFF ──────── */
void test_in_normal_range(void) {
    TEST_ASSERT_FALSE(EvaluateTemperature(24.0f, ActivityState::ACTIVE));
}

/* ── 4. Exactly 30 °C → alarm OFF (boundary) ────────── */
void test_at_upper_bound(void) {
    TEST_ASSERT_FALSE(EvaluateTemperature(30.0f, ActivityState::ACTIVE));
}

/* ── 5. Above 30 °C → alarm ON ──────────────────────── */
void test_above_threshold(void) {
    TEST_ASSERT_TRUE(EvaluateTemperature(35.0f, ActivityState::ACTIVE));
}
