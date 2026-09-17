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
 * Thread-safe snapshots for tasks that include activity data in their output.
 */
ActivityState SystemState_GetActivityState();
bool SystemState_IsMotionDetected();

#endif
