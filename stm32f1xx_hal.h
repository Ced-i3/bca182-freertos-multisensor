/*
 * Minimal STM32 HAL mock for native PlatformIO unit tests.
 *
 * This header provides only the type definitions and macros required
 * by the project headers (alarm.h, sensors.h, input.h, display.h)
 * so they can compile under a host gcc without the real STM32 HAL.
 *
 * This file is NOT part of the firmware and is only used when
 * platformio builds with  platform = native.
 */
#ifndef STM32F1XX_HAL_H_MOCK
#define STM32F1XX_HAL_H_MOCK

#include <stdint.h>

/* ── GPIO ─────────────────────────────────────────────── */

typedef struct { int dummy; } GPIO_TypeDef;

extern GPIO_TypeDef _mock_gpioA;
extern GPIO_TypeDef _mock_gpioB;
extern GPIO_TypeDef _mock_gpioC;
#define GPIOA (&_mock_gpioA)
#define GPIOB (&_mock_gpioB)
#define GPIOC (&_mock_gpioC)

typedef enum {
    GPIO_PIN_RESET = 0U,
    GPIO_PIN_SET   = 1U
} GPIO_PinState;

#define GPIO_PIN_0   ((uint16_t)0x0001)
#define GPIO_PIN_1   ((uint16_t)0x0002)
#define GPIO_PIN_13  ((uint16_t)0x2000)

#define GPIO_MODE_INPUT       0x00
#define GPIO_MODE_OUTPUT_PP   0x01
#define GPIO_MODE_ANALOG      0x03
#define GPIO_MODE_AF_OD       0x0B

#define GPIO_NOPULL     0x00
#define GPIO_PULLUP     0x01
#define GPIO_PULLDOWN   0x02

#define GPIO_SPEED_FREQ_LOW   0x02
#define GPIO_SPEED_FREQ_HIGH  0x03

typedef struct {
    uint32_t Pin;
    uint32_t Mode;
    uint32_t Pull;
    uint32_t Speed;
} GPIO_InitTypeDef;

static inline void HAL_GPIO_Init(GPIO_TypeDef *, GPIO_InitTypeDef *) {}
static inline void HAL_GPIO_WritePin(GPIO_TypeDef *, uint16_t, GPIO_PinState) {}
static inline void HAL_GPIO_TogglePin(GPIO_TypeDef *, uint16_t) {}
static inline GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *, uint16_t) { return GPIO_PIN_RESET; }

/* ── I2C ──────────────────────────────────────────────── */

typedef struct { int dummy; } I2C_HandleTypeDef;

#define HAL_OK   0U
#define HAL_MAX_DELAY 0xFFFFFFFFU

static inline uint32_t HAL_I2C_Master_Transmit(
    I2C_HandleTypeDef *, uint16_t, uint8_t *, uint16_t, uint32_t)
{ return HAL_OK; }

/* ── ADC ──────────────────────────────────────────────── */

typedef struct { int dummy; } ADC_HandleTypeDef;
typedef struct {
    uint32_t Channel;
    uint32_t Rank;
    uint32_t SamplingTime;
} ADC_ChannelConfTypeDef;

#define ADC_CHANNEL_0          0U
#define ADC_REGULAR_RANK_1     1U
#define ADC_SAMPLETIME_55CYCLES_5 5U

static inline uint32_t HAL_ADC_Start(ADC_HandleTypeDef *) { return HAL_OK; }
static inline uint32_t HAL_ADC_PollForConversion(ADC_HandleTypeDef *, uint32_t) { return HAL_OK; }
static inline uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *) { return 0; }
static inline uint32_t HAL_ADC_Stop(ADC_HandleTypeDef *) { return HAL_OK; }
static inline uint32_t HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *) { return HAL_OK; }

/* ── Clock ────────────────────────────────────────────── */

static inline uint32_t HAL_RCC_GetHCLKFreq(void) { return 72000000UL; }
static inline void __HAL_RCC_GPIOA_CLK_ENABLE(void) {}
static inline void __HAL_RCC_GPIOB_CLK_ENABLE(void) {}
static inline void __HAL_RCC_GPIOC_CLK_ENABLE(void) {}
static inline void __HAL_RCC_USART1_CLK_ENABLE(void) {}
static inline void __HAL_RCC_ADC1_CLK_ENABLE(void) {}
static inline void __HAL_RCC_I2C1_CLK_ENABLE(void) {}

/* ── Delay ────────────────────────────────────────────── */

static inline void HAL_Delay(uint32_t) {}

#endif /* STM32F1XX_HAL_H_MOCK */
