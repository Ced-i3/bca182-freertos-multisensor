#ifndef MOTION_H
#define MOTION_H

#include "stm32f1xx_hal.h"

/* Configure the GPIO used by the PIR output. */
void PIR_Init(GPIO_TypeDef *port, uint16_t pin);

/* Return the current digital level from the PIR output. */
bool PIR_IsMotionDetected();

/* Create the dedicated MotionTask. */
bool MotionTask_Create();

#endif
