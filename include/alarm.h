#ifndef ALARM_H
#define ALARM_H

#include "stm32f1xx_hal.h"
#include "system_state.h"

/*
 * Initialize the buzzer GPIO output.
 */
void Buzzer_Init(GPIO_TypeDef *port, uint16_t pin);

/*
 * Pure decision function: returns true when the temperature is outside
 * the 18-30 C comfort range AND the system is ACTIVE.
 */
bool EvaluateTemperature(float temperature, ActivityState state);

/*
 * Create the dedicated AlarmTask.
 *
 * The task monitors the sensor queue for temperature readings
 * outside the 18-30 C comfort range and activates the buzzer.
 * Buzzer is silenced during INACTIVE periods.
 */
bool AlarmTask_Create();

#endif
