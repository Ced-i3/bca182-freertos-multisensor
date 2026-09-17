#include "motion.h"
#include "system_state.h"

extern "C"
{
#include "FreeRTOS.h"
#include "task.h"
}

namespace
{
    constexpr UBaseType_t motionTaskPriority = 3;
    constexpr uint16_t motionTaskStackSize = 256;
    constexpr uint32_t motionPollPeriodMs = 100;

    GPIO_TypeDef *pirPort = nullptr;
    uint16_t pirPin = 0;

    void MotionTask(void *pvParameters)
    {
        (void)pvParameters;

        for (;;)
        {
            if (PIR_IsMotionDetected())
            {
                /* MotionTask is the producer; StateTask is the consumer. */
                SystemState_NotifyMotionDetected();
            }

            vTaskDelay(pdMS_TO_TICKS(motionPollPeriodMs));
        }
    }
}

void PIR_Init(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    pirPort = port;
    pirPin = pin;

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

    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;

    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

bool PIR_IsMotionDetected()
{
    if (pirPort == nullptr || pirPin == 0)
    {
        return false;
    }

    return HAL_GPIO_ReadPin(pirPort, pirPin) == GPIO_PIN_SET;
}

bool MotionTask_Create()
{
    return xTaskCreate(
               MotionTask,
               "MotionTask",
               motionTaskStackSize,
               nullptr,
               motionTaskPriority,
               nullptr) == pdPASS;
}
