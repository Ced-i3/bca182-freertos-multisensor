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
#define GPIO_PIN_8   ((uint16_t)0x0100)
#define GPIO_PIN_9   ((uint16_t)0x0200)
#define GPIO_PIN_10  ((uint16_t)0x0400)
#define GPIO_PIN_13  ((uint16_t)0x2000)

#define GPIO_MODE_INPUT       0x00
#define GPIO_MODE_OUTPUT_PP   0x01
#define GPIO_MODE_ANALOG      0x03
#define GPIO_MODE_AF_OD       0x0B
#define GPIO_MODE_AF_PP       0x02

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

typedef struct { uint32_t CFGR; } RCC_TypeDef;
static RCC_TypeDef _mock_rcc;
#define RCC (&_mock_rcc)
#define RCC_CFGR_PPRE2        0U
#define RCC_CFGR_PPRE2_DIV1   0U

typedef struct {
    uint32_t Prescaler;
    uint32_t CounterMode;
    uint32_t Period;
    uint32_t ClockDivision;
    uint32_t RepetitionCounter;
    uint32_t AutoReloadPreload;
} TIM_InitTypeDef;

typedef struct {
    void *Instance;
    TIM_InitTypeDef Init;
} TIM_HandleTypeDef;

static inline uint32_t HAL_RCC_GetHCLKFreq(void) { return 72000000UL; }
static inline uint32_t HAL_RCC_GetPCLK2Freq(void) { return 72000000UL; }
static inline void __HAL_RCC_GPIOA_CLK_ENABLE(void) {}
static inline void __HAL_RCC_GPIOB_CLK_ENABLE(void) {}
static inline void __HAL_RCC_GPIOC_CLK_ENABLE(void) {}
static inline void __HAL_RCC_USART1_CLK_ENABLE(void) {}
static inline void __HAL_RCC_ADC1_CLK_ENABLE(void) {}
static inline void __HAL_RCC_I2C1_CLK_ENABLE(void) {}
static inline void __HAL_RCC_TIM1_CLK_ENABLE(void) {}

/* ── Delay ────────────────────────────────────────────── */

static inline void HAL_Delay(uint32_t) {}

/* ── TIM (minimal stubs for native tests) ─────────────── */

typedef struct {
    uint32_t OCMode;
    uint32_t Pulse;
    uint32_t OCPolarity;
    uint32_t OCNPolarity;
    uint32_t OCFastMode;
    uint32_t OCIdleState;
    uint32_t OCNIdleState;
} TIM_OC_InitTypeDef;

#define __HAL_RCC_TIM1_CLK_ENABLE()  ((void)0)
#define TIM_COUNTERMODE_UP           0U
#define TIM_CLOCKDIVISION_DIV1       0U
#define TIM_AUTORELOAD_PRELOAD_DISABLE 0U
#define TIM_OCMODE_PWM1              0U
#define TIM_OCPOLARITY_HIGH          0U
#define TIM_OCNPOLARITY_HIGH         0U
#define TIM_OCFAST_DISABLE           0U
#define TIM_OCIDLESTATE_RESET        0U
#define TIM_OCNIDLESTATE_RESET       0U
#define TIM_CHANNEL_1                0U

static TIM_HandleTypeDef _mock_htim1;
#define TIM1 (&_mock_htim1)

static inline uint32_t HAL_TIM_PWM_Init(TIM_HandleTypeDef *) { return HAL_OK; }
static inline uint32_t HAL_TIM_PWM_ConfigChannel(TIM_HandleTypeDef *, void *, uint32_t) { return HAL_OK; }
static inline uint32_t HAL_TIM_PWM_Start(TIM_HandleTypeDef *, uint32_t) { return HAL_OK; }
static inline uint32_t HAL_TIM_PWM_Stop(TIM_HandleTypeDef *, uint32_t) { return HAL_OK; }

#endif /* STM32F1XX_HAL_H_MOCK */
