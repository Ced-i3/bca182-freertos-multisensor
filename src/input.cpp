#include "input.h"
#include "system_state.h"

extern "C"
{
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
}

namespace
{
    constexpr UBaseType_t inputTaskPriority = 3;
    constexpr uint16_t inputTaskStackSize = 256;
    constexpr uint32_t inputPollPeriodMs = 1;
    constexpr uint32_t encoderDebounceMs = 10;

    GPIO_TypeDef *clockPort = nullptr;
    uint16_t clockPin = 0;
    GPIO_TypeDef *dataPort = nullptr;
    uint16_t dataPin = 0;

    SemaphoreHandle_t displayModeMutex = nullptr;
    DisplayMode selectedDisplayMode = DisplayMode::TEMPERATURE;

    void EnablePortClock(GPIO_TypeDef *port)
    {
        if (port == GPIOA)
        {
            __HAL_RCC_GPIOA_CLK_ENABLE();
        }
        else if (port == GPIOB)
        {
            __HAL_RCC_GPIOB_CLK_ENABLE();
        }
        else if (port == GPIOC)
        {
            __HAL_RCC_GPIOC_CLK_ENABLE();
        }
    }

    GPIO_PinState ReadClock()
    {
        return HAL_GPIO_ReadPin(clockPort, clockPin);
    }

    GPIO_PinState ReadData()
    {
        return HAL_GPIO_ReadPin(dataPort, dataPin);
    }

    void SetDisplayMode(DisplayMode newMode)
    {
        if (xSemaphoreTake(displayModeMutex, portMAX_DELAY) == pdTRUE)
        {
            selectedDisplayMode = newMode;
            xSemaphoreGive(displayModeMutex);
        }
    }

    void InputTask(void *pvParameters)
    {
        (void)pvParameters;

        GPIO_PinState previousClock = ReadClock();
        TickType_t lastNavigationTick = 0;

        for (;;)
        {
            GPIO_PinState currentClock = ReadClock();

            /* The encoder is active only while the system is ACTIVE. */
            if (SystemState_GetActivityState() == ActivityState::ACTIVE)
            {
                /*
                 * The Wokwi KY-040 and a conventional quadrature encoder
                 * present the direction on DT when CLK falls.
                 */
                if (previousClock == GPIO_PIN_SET &&
                    currentClock == GPIO_PIN_RESET)
                {
                    TickType_t now = xTaskGetTickCount();

                    if ((now - lastNavigationTick) >=
                        pdMS_TO_TICKS(encoderDebounceMs))
                    {
                        DisplayMode currentMode = Input_GetDisplayMode();

                        if (ReadData() == GPIO_PIN_SET)
                        {
                            SetDisplayMode(NextDisplayMode(currentMode));
                        }
                        else
                        {
                            SetDisplayMode(PreviousDisplayMode(currentMode));
                        }

                        lastNavigationTick = now;
                    }
                }
            }

            previousClock = currentClock;
            vTaskDelay(pdMS_TO_TICKS(inputPollPeriodMs));
        }
    }
}

void Encoder_Init(
    GPIO_TypeDef *newClockPort,
    uint16_t newClockPin,
    GPIO_TypeDef *newDataPort,
    uint16_t newDataPin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    clockPort = newClockPort;
    clockPin = newClockPin;
    dataPort = newDataPort;
    dataPin = newDataPin;

    EnablePortClock(clockPort);
    EnablePortClock(dataPort);

    GPIO_InitStruct.Pin = clockPin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(clockPort, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = dataPin;
    HAL_GPIO_Init(dataPort, &GPIO_InitStruct);
}

DisplayMode NextDisplayMode(DisplayMode currentMode)
{
    switch (currentMode)
    {
        case DisplayMode::TEMPERATURE:
            return DisplayMode::HUMIDITY;
        case DisplayMode::HUMIDITY:
            return DisplayMode::LIGHT;
        case DisplayMode::LIGHT:
            return DisplayMode::MOTION;
        case DisplayMode::MOTION:
        default:
            return DisplayMode::TEMPERATURE;
    }
}

DisplayMode PreviousDisplayMode(DisplayMode currentMode)
{
    switch (currentMode)
    {
        case DisplayMode::TEMPERATURE:
        default:
            return DisplayMode::MOTION;
        case DisplayMode::HUMIDITY:
            return DisplayMode::TEMPERATURE;
        case DisplayMode::LIGHT:
            return DisplayMode::HUMIDITY;
        case DisplayMode::MOTION:
            return DisplayMode::LIGHT;
    }
}

bool InputTask_Create()
{
    displayModeMutex = xSemaphoreCreateMutex();

    if (displayModeMutex == nullptr)
    {
        return false;
    }

    return xTaskCreate(
               InputTask,
               "InputTask",
               inputTaskStackSize,
               nullptr,
               inputTaskPriority,
               nullptr) == pdPASS;
}

DisplayMode Input_GetDisplayMode()
{
    if (displayModeMutex == nullptr)
    {
        return DisplayMode::TEMPERATURE;
    }

    if (xSemaphoreTake(displayModeMutex, portMAX_DELAY) == pdTRUE)
    {
        DisplayMode mode = selectedDisplayMode;
        xSemaphoreGive(displayModeMutex);
        return mode;
    }

    return DisplayMode::TEMPERATURE;
}
