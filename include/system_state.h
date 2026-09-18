#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

enum class ActivityState
{
    ACTIVE,
    INACTIVE
};

/*
 * Create the task that owns the ACTIVE/INACTIVE state machine.
 */
bool SystemState_CreateTask();

/*
 * Notify the state task that the PIR reported motion.
 */
void SystemState_NotifyMotionDetected();

/*
 * Pure decision function: returns ACTIVE when a motion notification
 * has arrived, or INACTIVE on timeout.
 */
ActivityState EvaluateSystemState(bool notificationReceived);

/*
 * Thread-safe snapshots for tasks that include activity data in their output.
 */
ActivityState SystemState_GetActivityState();
bool SystemState_IsMotionDetected();

#endif
