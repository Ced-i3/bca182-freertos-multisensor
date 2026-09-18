#ifndef DISPLAY_H
#define DISPLAY_H

#include "stm32f1xx_hal.h"
#include "sensors.h"
#include "input.h"

/* Shared I2C handle — defined in main.cpp. */
extern I2C_HandleTypeDef hi2c1;

/*
 * Initialize the SSD1306 OLED over I2C1 (PB6=SCL, PB7=SDA).
 */
void Display_Init(I2C_HandleTypeDef *hi2c);

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
