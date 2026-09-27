#ifndef SENSORS_H
#define SENSORS_H

#include "stm32f1xx_hal.h"

struct SensorData
{
    float temperature;
    float humidity;
    int lightLevel;
    bool motionDetected;
    bool dhtValid;   /* true only when DHT22_Read succeeded */
};

/*
 * Initialize LED (PC13), ADC1 for LDR (PA1), and DHT22 (PA0).
 * Call once before creating SensorTask.
 */
void sensors_init(void);

/*
 * Initialize the DHT22 sensor.
 */
void DHT22_Init(GPIO_TypeDef *port, uint16_t pin);

/*
 * Read temperature and humidity from the DHT22.
 *
 * Returns true when a valid reading was received.
 */
bool DHT22_Read(float *temperature, float *humidity);

/*
 * Read ADC1 channel 1 and convert its 12-bit value to a relative
 * ambient-light level from 0 to 100. This is not calibrated lux.
 */
bool LDR_Read(int *lightLevel);

#endif
