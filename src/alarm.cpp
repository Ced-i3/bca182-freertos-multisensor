#include "alarm.h"
#include "sensors.h"
#include "system_state.h"

extern "C"
{
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
}

/* ============================================================
 * Alarm configuration
 * ============================================================ */

static constexpr float TEMP_LOW  = 18.0f;
static constexpr float TEMP_HIGH = 30.0f;

/* ============================================================
 * Pure decision function
 * ============================================================ */

bool EvaluateTemperature(float temperature, ActivityState state)
{
    return (temperature < TEMP_LOW || temperature > TEMP_HIGH)
           && state == ActivityState::ACTIVE;
}

/* ============================================================
 * Buzzer GPIO
 * ============================================================ */

static GPIO_TypeDef *buzzerPort = nullptr;
static uint16_t      buzzerPin  = 0;

void Buzzer_Init(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    buzzerPort = port;
    buzzerPin  = pin;

    if (port == GPIOA)
        __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB)
        __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC)
        __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin   = pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
}

/* ============================================================
 * AlarmTask
 *
 * Priority: 2
 * Stack:    256 words
 *
 * Uses xQueuePeek to non-destructively read the latest sensor
 * sample.  When the temperature is outside 18-30 C and the
 * system is ACTIVE, the buzzer is toggled at ~2 Hz to produce
 * an audible alarm.  The buzzer is silenced when the system is
 * INACTIVE or when the temperature returns to range.
 *
 * The task notification is used for optional external alarm
 * suppression (a future milestone can xTaskNotifyGive to force
 * the alarm off).
 * ============================================================ */

static void AlarmTask(void *pvParameters)
{
    (void)pvParameters;

    extern QueueHandle_t sensorQueue;
    SensorData sensorData = {};

    for (;;)
    {
        /* Peek the latest sensor sample without consuming it. */
        (void)xQueuePeek(sensorQueue, &sensorData, 0);

        bool alarmActive = EvaluateTemperature(
            sensorData.temperature,
            SystemState_GetActivityState());

        if (alarmActive)
        {
            HAL_GPIO_TogglePin(buzzerPort, buzzerPin);
        }
        else
        {
            HAL_GPIO_WritePin(buzzerPort, buzzerPin, GPIO_PIN_RESET);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

bool AlarmTask_Create()
{
    return xTaskCreate(
               AlarmTask,
               "AlarmTask",
               256,
               nullptr,
               2,
               nullptr) == pdPASS;
}
