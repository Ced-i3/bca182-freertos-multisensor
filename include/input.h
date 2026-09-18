#ifndef INPUT_H
#define INPUT_H

#include "stm32f1xx_hal.h"

enum class DisplayMode
{
    TEMPERATURE,
    HUMIDITY,
    LIGHT,
    MOTION
};

/* Configure the CLK and DT GPIO inputs used by the rotary encoder. */
void Encoder_Init(
    GPIO_TypeDef *clockPort,
    uint16_t clockPin,
    GPIO_TypeDef *dataPort,
    uint16_t dataPin);

/* Pure navigation helpers for the four laboratory display pages. */
DisplayMode NextDisplayMode(DisplayMode currentMode);
DisplayMode PreviousDisplayMode(DisplayMode currentMode);

/* Create the dedicated FreeRTOS InputTask. */
bool InputTask_Create();

/* Thread-safe selection snapshot for the future DisplayTask. */
DisplayMode Input_GetDisplayMode();

#endif
