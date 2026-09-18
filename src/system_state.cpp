#include "system_state.h"

extern "C"
{
#include "FreeRTOS.h"
#include "task.h"
}

namespace
{
    constexpr UBaseType_t stateTaskPriority = 2;
    constexpr uint16_t stateTaskStackSize = 256;
    constexpr uint32_t inactivityTimeoutMs = 15000;

    TaskHandle_t stateTaskHandle = nullptr;
    ActivityState activityState = ActivityState::ACTIVE;
    bool motionDetected = false;

    void SetState(ActivityState newState, bool newMotionDetected)
    {
        taskENTER_CRITICAL();
        activityState = newState;
        motionDetected = newMotionDetected;
        taskEXIT_CRITICAL();
    }

    void StateTask(void *pvParameters)
    {
        (void)pvParameters;

        /* The room begins ACTIVE and becomes inactive after no PIR events. */
        SetState(ActivityState::ACTIVE, false);

        for (;;)
        {
            /*
             * A task notification represents one or more motion samples
             * reported by MotionTask. Waiting here blocks StateTask without
             * polling or busy-waiting.
             */
            bool notified = ulTaskNotifyTake(
                    pdTRUE,
                    pdMS_TO_TICKS(inactivityTimeoutMs)) > 0;

            ActivityState newState = EvaluateSystemState(notified);
            SetState(newState, notified);
        }
    }
}

/* ============================================================
 * Pure decision function
 * ============================================================ */

ActivityState EvaluateSystemState(bool notificationReceived)
{
    return notificationReceived
        ? ActivityState::ACTIVE
        : ActivityState::INACTIVE;
}

bool SystemState_CreateTask()
{
    activityState = ActivityState::ACTIVE;
    motionDetected = false;

    return xTaskCreate(
               StateTask,
               "StateTask",
               stateTaskStackSize,
               nullptr,
               stateTaskPriority,
               &stateTaskHandle) == pdPASS;
}

void SystemState_NotifyMotionDetected()
{
    if (stateTaskHandle != nullptr)
    {
        xTaskNotifyGive(stateTaskHandle);
    }
}

ActivityState SystemState_GetActivityState()
{
    taskENTER_CRITICAL();
    ActivityState state = activityState;
    taskEXIT_CRITICAL();

    return state;
}

bool SystemState_IsMotionDetected()
{
    taskENTER_CRITICAL();
    bool detected = motionDetected;
    taskEXIT_CRITICAL();

    return detected;
}
