#ifndef DISPLAY_H
#define DISPLAY_H

#include "stm32f1xx_hal.h"
#include "sensors.h"
#include "input.h"

/*
 * Initialize I2C1 hardware (PB6=SCL, PB7=SDA).
 * The OLED panel itself is initialised by DisplayTask.
 */
void display_init(void);

/*
 * Clear the display buffer and push it to the OLED.
 */
void Display_Clear();

/*
 * Render a string at the given page (0-7) and column (0-127).
 */
void Display_DrawString(
    uint8_t page,
    uint8_t col,
    const char *str);

/*
 * Create the dedicated DisplayTask.
 *
 * The task receives SensorData from the sensor queue and reads the
 * current DisplayMode from Input_GetDisplayMode() to decide what
 * to render on the OLED.
 */
bool DisplayTask_Create();

#endif
