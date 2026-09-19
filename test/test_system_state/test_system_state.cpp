/*
 * test/test_system_state/test_system_state.cpp
 *
 * Unit tests for EvaluateSystemState() — M6 requirement.
 *
 * This is the pure decision function extracted from StateTask.
 * It maps a boolean notification flag to the correct ActivityState:
 *   true  → ACTIVE   (motion notification arrived)
 *   false → INACTIVE (timeout, no motion)
 */
#include <unity.h>
#include "system_state.h"

/* Forward declarations */
void test_notification_sets_active(void);
void test_timeout_sets_inactive(void);
void test_repeated_notification_stays_active(void);
void test_consecutive_timeouts_stay_inactive(void);

void setup() {
    UNITY_BEGIN();
}

void loop() {
    RUN_TEST(test_notification_sets_active);
    RUN_TEST(test_timeout_sets_inactive);
    RUN_TEST(test_repeated_notification_stays_active);
    RUN_TEST(test_consecutive_timeouts_stay_inactive);

    UNITY_END();
}

/* ── 10. Motion notification → ACTIVE ─────────────────── */
void test_notification_sets_active(void) {
    TEST_ASSERT_EQUAL(ActivityState::ACTIVE,
                      EvaluateSystemState(true));
}

/* ── 11. Timeout / no notification → INACTIVE ────────── */
void test_timeout_sets_inactive(void) {
    TEST_ASSERT_EQUAL(ActivityState::INACTIVE,
                      EvaluateSystemState(false));
}

/* ── 12. Repeated notification → ACTIVE ──────────────── */
void test_repeated_notification_stays_active(void) {
    /* Simulates consecutive motion events keeping the system alive. */
    TEST_ASSERT_EQUAL(ActivityState::ACTIVE,
                      EvaluateSystemState(true));
    TEST_ASSERT_EQUAL(ActivityState::ACTIVE,
                      EvaluateSystemState(true));
}

/* ── 13. Consecutive timeouts → INACTIVE ─────────────── */
void test_consecutive_timeouts_stay_inactive(void) {
    /* Simulates multiple 15-second windows with no PIR event. */
    TEST_ASSERT_EQUAL(ActivityState::INACTIVE,
                      EvaluateSystemState(false));
    TEST_ASSERT_EQUAL(ActivityState::INACTIVE,
                      EvaluateSystemState(false));
}
