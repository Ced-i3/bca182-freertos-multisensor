#include "sensors.h"

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

#include <cstdio>

/* ============================================================
 * DHT22 on PA0 — DWT cycle-counter timing
 *
 * Adapted from the friend's proven Wokwi implementation.
 * Uses direct register access for GPIO mode switching
 * (HAL_GPIO_Init is too slow for the release-then-listen step).
 * The DWT cycle counter provides clock-independent microsecond
 * timing that works at any SystemCoreClock frequency.
 * ============================================================ */

static bool s_cycle_counter_ok = false;
static uint32_t s_cycles_per_us = 1;

static void dht_release(void)
{
    GPIOA->BSRR = GPIO_PIN_0;
    GPIOA->CRL = (GPIOA->CRL & ~0xFUL) | 0x8UL;   /* input, pull-up/down */
}

static void dht_drive_low(void)
{
    GPIOA->BRR = GPIO_PIN_0;
    GPIOA->CRL = (GPIOA->CRL & ~0xFUL) | 0x6UL;   /* open-drain out, 2 MHz */
}

static inline bool dht_is_high(void)
{
    return (GPIOA->IDR & GPIO_PIN_0) != 0;
}

static inline uint32_t us_to_cycles(uint32_t us)
{
    return us * s_cycles_per_us;
}

static bool wait_level(bool high, uint32_t timeout_cycles)
{
    uint32_t start = DWT->CYCCNT;
    for (uint32_t guard = 0; guard < timeout_cycles; guard++)
    {
        if (dht_is_high() == high) return true;
        if (DWT->CYCCNT - start > timeout_cycles) return false;
    }
    return false;
}

/* ============================================================
 * DHT22 Initialization
 * ============================================================ */
void DHT22_Init(GPIO_TypeDef *port, uint16_t pin)
{
    (void)port;
    (void)pin;  /* PA0 is used directly via registers */

    __HAL_RCC_GPIOA_CLK_ENABLE();
    dht_release();

    /* Enable DWT cycle counter */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    s_cycles_per_us = SystemCoreClock / 1000000U;

    /* Verify DWT is counting */
    uint32_t before = DWT->CYCCNT;
    for (volatile uint32_t i = 0; i < 16; i++) {}
    s_cycle_counter_ok = (DWT->CYCCNT != before);
}

/* ============================================================
 * DHT22 Read
 *
 * The 40-bit read runs inside a critical section to prevent
 * SysTick from interrupting mid-bit and corrupting timing.
 * The patched FreeRTOS port gates SysTick during critical
 * sections and recovers missed ticks on exit.
 * ============================================================ */
bool DHT22_Read(float *temperature, float *humidity)
{
    if (!s_cycle_counter_ok) return false;
    if (!temperature || !humidity) return false;

    uint16_t high_us[40];

    /* Start signal: hold low for 2 ms */
    dht_drive_low();
    vTaskDelay(pdMS_TO_TICKS(2));

    /* Critical section: every edge must be seen within a few
     * microseconds. A '0' and '1' differ only in high-pulse
     * width (26-28 us vs 70 us). */
    taskENTER_CRITICAL();

    dht_release();

    /* Wait for sensor response: HIGH, LOW, HIGH, LOW */
    if (!wait_level(true,  us_to_cycles(100)) ||
        !wait_level(false, us_to_cycles(100)) ||
        !wait_level(true,  us_to_cycles(100)) ||
        !wait_level(false, us_to_cycles(100)))
    {
        taskEXIT_CRITICAL();
        return false;
    }

    /* Read 40 bits */
    for (uint8_t i = 0; i < 40; i++)
    {
        if (!wait_level(true, us_to_cycles(80)))
        {
            taskEXIT_CRITICAL();
            return false;
        }
        uint32_t rise = DWT->CYCCNT;
        if (!wait_level(false, us_to_cycles(100)))
        {
            taskEXIT_CRITICAL();
            return false;
        }
        high_us[i] = (uint16_t)((DWT->CYCCNT - rise) / s_cycles_per_us);
    }

    taskEXIT_CRITICAL();

    /* Decode 40 high-pulse widths into 5 bytes */
    uint8_t data[5] = {0};
    for (uint8_t i = 0; i < 40; i++)
    {
        data[i / 8] = (data[i / 8] << 1) | (high_us[i] > 48 ? 1 : 0);
    }

    /* Verify checksum */
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) return false;

    /* Convert humidity */
    uint16_t rawHumidity = (data[0] << 8) | data[1];
    *humidity = rawHumidity / 10.0f;

    /* Convert temperature */
    uint16_t rawTemperature = (data[2] << 8) | data[3];
    if (rawTemperature & 0x8000)
    {
        rawTemperature &= 0x7FFF;
        *temperature = -(rawTemperature / 10.0f);
    }
    else
    {
        *temperature = rawTemperature / 10.0f;
    }

    /* Return line to idle state */
    dht_release();
    GPIOA->BSRR = GPIO_PIN_0;

    return true;
}

/* ============================================================
 * LDR Read
 *
 * The LDR module output (AO) is connected to PA1 / ADC1
 * channel 1. The raw 12-bit value is scaled to 0-100.
 * ============================================================ */
static ADC_HandleTypeDef hadc1;

bool LDR_Read(int *lightLevel)
{
    if (!lightLevel) return false;

    if (HAL_ADC_Start(&hadc1) != HAL_OK) return false;

    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK)
    {
        (void)HAL_ADC_Stop(&hadc1);
        return false;
    }

    uint32_t rawValue = HAL_ADC_GetValue(&hadc1);

    if (HAL_ADC_Stop(&hadc1) != HAL_OK)
    {
        return false;
    }

    *lightLevel = static_cast<int>((rawValue * 100U + 2047U) / 4095U);
    return true;
}

/* ============================================================
 * sensors_init — LED, ADC1 for LDR, DHT22 DWT setup
 *
 * Matches the friend's initialization approach.
 * ============================================================ */
void sensors_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();

    /* PC13 — heartbeat LED (active-low on Blue Pill) */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);  /* LED OFF */

    /* PA1 — LDR analog input (ADC1 channel 1) */
    GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /* PA0 — DHT22 (DWT init is inside DHT22_Init) */
    DHT22_Init(GPIOA, GPIO_PIN_0);
}
