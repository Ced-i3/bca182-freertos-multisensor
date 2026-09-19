/*
 * hal_mock.cpp — Test-only mock implementations for native builds.
 * Defines GPIO port stubs, FreeRTOS stubs, and main() for Unity tests.
 *
 * This file compiles to empty on STM32 builds thanks to the UNIT_TEST guard.
 */
#ifdef UNIT_TEST

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"

GPIO_TypeDef _mock_gpioA;
GPIO_TypeDef _mock_gpioB;
GPIO_TypeDef _mock_gpioC;

/* Stub for the global sensor queue referenced by alarm.cpp */
QueueHandle_t sensorQueue = NULL;

/*
 * PlatformIO's Unity test framework expects the test file to define
 * setup() and loop() (Arduino convention). Unity does NOT provide main().
 * We provide main() here so the linker finds it and the test runner
 * can execute.
 */
void setup();
void loop();

int main(void)
{
    setup();
    loop();
    return 0;
}

#endif /* UNIT_TEST */
