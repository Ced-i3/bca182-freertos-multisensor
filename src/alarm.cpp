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
 * Buzzer — TIM1 PWM on PA8 (friend's proven approach)
 *
 * PA8 is TIM1_CH1. The buzzer is driven by hardware PWM at
 * ~1 kHz, 50% duty. The prescaler is computed at runtime from
 * the actual TIM1 clock so it stays correct at any system
 * frequency (8 MHz or 72 MHz).
 * ============================================================ */

static TIM_HandleTypeDef htim1;

static uint32_t tim1_clock_hz(void)
{
    uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();
    bool apb2_undivided = (RCC->CFGR & RCC_CFGR_PPRE2) == RCC_CFGR_PPRE2_DIV1;
    return apb2_undivided ? pclk2 : 2U * pclk2;
}

void Buzzer_Init(GPIO_TypeDef *port, uint16_t pin)
{
    (void)port;
    (void)pin;  /* PA8 is used directly via TIM1_CH1 */

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();

    /* PA8 — TIM1_CH1 alternate function push-pull */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Compute prescaler from actual timer clock */
    uint32_t timer_hz = tim1_clock_hz();
    uint32_t prescaler = timer_hz / 1000000U - 1;   /* 1 MHz tick */
    uint32_t period = 1000000U / 1000U - 1;          /* 1 kHz PWM */

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = prescaler;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = period;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim1);

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = (period + 1) / 2;                  /* 50 % duty */
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    oc.OCIdleState = TIM_OCIDLESTATE_RESET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    HAL_TIM_PWM_ConfigChannel(&htim1, &oc, TIM_CHANNEL_1);
}

static void buzzer_set(bool on)
{
    if (on)
    {
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    }
    else
    {
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    }
}

/* ============================================================
 * AlarmTask
 *
 * Priority: 2
 * Stack:    256 words
 *
 * Uses xQueuePeek to non-destructively read the latest sensor
 * sample. When the temperature is outside 18-30 C and the
 * system is ACTIVE, the buzzer is enabled via PWM.
 * ============================================================ */

static void AlarmTask(void *pvParameters)
{
    (void)pvParameters;

    extern QueueHandle_t sensorQueue;
    SensorData sensorData = {};
    bool dataReceived = false;
    bool sounding = false;

    for (;;)
    {
        if (xQueuePeek(sensorQueue, &sensorData, 0) == pdTRUE)
        {
            dataReceived = true;
        }

        if (dataReceived)
        {
            bool alarmActive = sensorData.dhtValid &&
                               EvaluateTemperature(
                                   sensorData.temperature,
                                   SystemState_GetActivityState());

            if (alarmActive && !sounding)
            {
                sounding = true;
                buzzer_set(true);
            }
            else if (!alarmActive && sounding)
            {
                sounding = false;
                buzzer_set(false);
            }
        }
        else
        {
            if (sounding)
            {
                sounding = false;
                buzzer_set(false);
            }
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
