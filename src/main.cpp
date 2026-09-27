#include "stm32f1xx_hal.h"
#include "alarm.h"
#include "display.h"
#include "input.h"
#include "motion.h"
#include "sensors.h"
#include "system_state.h"
#include <cstring>
#include <cstdio>

extern "C" {
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
}

UART_HandleTypeDef huart1;

SemaphoreHandle_t serialMutex;
QueueHandle_t sensorQueue;

namespace
{
    constexpr UBaseType_t sensorTaskPriority = 2;
    constexpr uint16_t sensorTaskStackSize = 256;
    constexpr UBaseType_t sensorQueueLength = 4;
    constexpr uint32_t sensorPeriodMs = 2000;
}

/* Function prototypes */
void SystemClock_Config();
void TaskA(void *pvParameters);
void TaskB(void *pvParameters);
void SensorTask(void *pvParameters);
void Error_Handler();

/* ---------------------------------------------------------
 * Serial output
 * --------------------------------------------------------- */
void Serial_Print(const char *message)
{
    if (serialMutex == nullptr)
    {
        return;
    }

    xSemaphoreTake(serialMutex, portMAX_DELAY);

    HAL_UART_Transmit(
        &huart1,
        reinterpret_cast<uint8_t *>(const_cast<char *>(message)),
        strlen(message),
        100
    );

    xSemaphoreGive(serialMutex);
}

/* ---------------------------------------------------------
 * Task A
 * --------------------------------------------------------- */
void TaskA(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        Serial_Print("Task A running\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ---------------------------------------------------------
 * Task B
 * --------------------------------------------------------- */
void TaskB(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        Serial_Print("Task B running\r\n");
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

/* ---------------------------------------------------------
 * SensorTask
 * --------------------------------------------------------- */
void SensorTask(void *pvParameters)
{
    (void)pvParameters;

    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        SensorData sensorData = {};

        bool dhtReadOk = DHT22_Read(
            &sensorData.temperature,
            &sensorData.humidity);
        bool ldrReadOk = LDR_Read(&sensorData.lightLevel);

        sensorData.dhtValid = dhtReadOk;

        if (dhtReadOk || ldrReadOk)
        {
            sensorData.motionDetected = SystemState_IsMotionDetected();

            {
                char buf[80];
                int len = snprintf(buf, sizeof(buf),
                    "Temp: %.2f C  Hum: %.2f %%  Light: %d\r\n",
                    sensorData.temperature,
                    sensorData.humidity,
                    sensorData.lightLevel);
                if (len > 0)
                {
                    Serial_Print(buf);
                }
            }

            (void)xQueueSend(sensorQueue, &sensorData, 0);
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(sensorPeriodMs));
    }
}

/* ---------------------------------------------------------
 * HAL tick hook — called by the patched port's SysTick_Handler
 * to keep HAL_GetTick() running.
 * --------------------------------------------------------- */
extern "C" void vApplicationTickHook(void)
{
    HAL_IncTick();
}

/* ---------------------------------------------------------
 * Application entry point
 * --------------------------------------------------------- */
void app_main()
{
    /* Initialize STM32 HAL */
    HAL_Init();

    /* Configure system clock (8 MHz HSI — empty config, matching friend) */
    SystemClock_Config();

    /* USART1 TX only — PA9, 115200 8N1 */
    {
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_USART1_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        huart1.Instance = USART1;
        huart1.Init.BaudRate = 115200;
        huart1.Init.WordLength = UART_WORDLENGTH_8B;
        huart1.Init.StopBits = UART_STOPBITS_1;
        huart1.Init.Parity = UART_PARITY_NONE;
        huart1.Init.Mode = UART_MODE_TX;
        huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
        huart1.Init.OverSampling = UART_OVERSAMPLING_16;
        HAL_UART_Init(&huart1);
    }

    /*
     * Create the mutex before using Serial_Print().
     */
    serialMutex = xSemaphoreCreateMutex();
    if (serialMutex == nullptr)
    {
        Error_Handler();
    }

    sensorQueue = xQueueCreate(sensorQueueLength, sizeof(SensorData));
    if (sensorQueue == nullptr)
    {
        Error_Handler();
    }

    /*
     * Initial diagnostic messages.
     */
    Serial_Print("BCA182 FreeRTOS Multisensor\r\n");
    Serial_Print("System starting...\r\n");

    /*
     * Initialize peripherals — matching the friend's init order.
     * sensors_init() sets up LED (PC13), ADC1 for LDR (PA1),
     * and DHT22 with DWT timing (PA0).
     */
    sensors_init();

    /* PIR on PA2 */
    PIR_Init(GPIOA, GPIO_PIN_2);

    /* Buzzer on PA8 via TIM1 PWM */
    Buzzer_Init(GPIOA, GPIO_PIN_8);

    /* Encoder CLK=PA3, DT=PA4 */
    Encoder_Init(GPIOA, GPIO_PIN_3, GPIOA, GPIO_PIN_4);

    /* I2C1 for OLED (PB6=SCL, PB7=SDA) — hardware only */
    display_init();

    /*
     * Create Task A (priority 2, LED toggle + serial)
     */
    if (xTaskCreate(
            TaskA,
            "TaskA",
            256,
            nullptr,
            2,
            nullptr) != pdPASS)
    {
        Error_Handler();
    }

    /*
     * Create Task B (priority 1, serial diagnostic)
     */
    if (xTaskCreate(
            TaskB,
            "TaskB",
            256,
            nullptr,
            1,
            nullptr) != pdPASS)
    {
        Error_Handler();
    }

    /*
     * SensorTask — priority 2, samples every 2 s using vTaskDelayUntil
     */
    if (xTaskCreate(
            SensorTask,
            "SensorTask",
            sensorTaskStackSize,
            nullptr,
            sensorTaskPriority,
            nullptr) != pdPASS)
    {
        Error_Handler();
    }

    /*
     * StateTask owns the ACTIVE/INACTIVE state machine.
     * MotionTask is created after it.
     */
    if (!SystemState_CreateTask() ||
        !MotionTask_Create() ||
        !InputTask_Create())
    {
        Error_Handler();
    }

    /*
     * DisplayTask and AlarmTask.
     * DisplayTask will initialise the OLED panel itself.
     */
    if (!DisplayTask_Create() || !AlarmTask_Create())
    {
        Error_Handler();
    }

    /*
     * Start the FreeRTOS scheduler.
     */
    vTaskStartScheduler();

    Error_Handler();
}

/* ---------------------------------------------------------
 * C/C++ program entry
 * --------------------------------------------------------- */
int main()
{
    /* SystemInit() leaves VTOR at its reset value 0. Point it at flash
     * so FreeRTOS can read the initial MSP from the vector table. */
    SCB->VTOR = FLASH_BASE;

    app_main();

    while (1)
    {
    }
}

/* ---------------------------------------------------------
 * STM32F103 Clock Configuration
 *
 * Empty — runs on HSI 8 MHz (reset default).
 * Matching the friend's proven Wokwi configuration.
 * The DWT cycle counter provides clock-independent timing,
 * and the PWM prescaler is computed at runtime from
 * HAL_RCC_GetPCLK2Freq().
 * --------------------------------------------------------- */
void SystemClock_Config()
{
}

/* ---------------------------------------------------------
 * Error Handler
 * --------------------------------------------------------- */
void Error_Handler()
{
    __disable_irq();

    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        for (volatile uint32_t i = 0; i < 300000; i++)
        {
        }
    }
}

/* ---------------------------------------------------------
 * FreeRTOS Stack Overflow Hook
 * --------------------------------------------------------- */
extern "C" void vApplicationStackOverflowHook(
    TaskHandle_t xTask,
    char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;

    __disable_irq();

    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        for (volatile uint32_t i = 0; i < 100000; i++)
        {
        }
    }
}
