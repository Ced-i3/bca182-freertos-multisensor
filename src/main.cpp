#include "stm32f1xx_hal.h"
#include <cstring>

extern "C" {
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
}

UART_HandleTypeDef huart1;

SemaphoreHandle_t serialMutex;

/* Function prototypes */
void SystemClock_Config();
static void MX_GPIO_Init();
static void MX_USART1_UART_Init();

void TaskA(void *pvParameters);
void TaskB(void *pvParameters);

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
        HAL_MAX_DELAY
    );

    xSemaphoreGive(serialMutex);
}

/* ---------------------------------------------------------
 * Task A
 *
 * Priority: 2
 * Delay: 1000 ms
 * --------------------------------------------------------- */
void TaskA(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        /*
         * Toggle the onboard LED whenever Task A executes.
         */
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        Serial_Print("Task A running\r\n");

        /*
         * Block Task A for 1 second.
         */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ---------------------------------------------------------
 * Task B
 *
 * Priority: 1
 * Delay: 1500 ms
 * --------------------------------------------------------- */
void TaskB(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        Serial_Print("Task B running\r\n");

        /*
         * Block Task B for 1.5 seconds.
         */
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

/* ---------------------------------------------------------
 * Application entry point
 * --------------------------------------------------------- */
void app_main()
{
    /* Initialize STM32 HAL */
    HAL_Init();

    /* Configure system clock */
    SystemClock_Config();

    /* Initialize GPIO */
    MX_GPIO_Init();

    /* Initialize USART1 */
    MX_USART1_UART_Init();

    /*
     * Create the mutex before using Serial_Print().
     */
    serialMutex = xSemaphoreCreateMutex();

    if (serialMutex == nullptr)
    {
        Error_Handler();
    }

    /*
     * Initial diagnostic messages.
     */
    Serial_Print("BCA182 FreeRTOS Multisensor\r\n");
    Serial_Print("System starting...\r\n");

    /*
     * Create Task A.
     *
     * Priority = 2
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
     * Create Task B.
     *
     * Priority = 1
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
     * Start the FreeRTOS scheduler.
     */
    vTaskStartScheduler();

    /*
     * We should never reach this point.
     */
    Error_Handler();
}

/* ---------------------------------------------------------
 * C/C++ program entry
 * --------------------------------------------------------- */
int main()
{
    app_main();

    while (1)
    {
    }
}

/* ---------------------------------------------------------
 * STM32F103 Clock Configuration
 *
 * HSE = 8 MHz
 * PLL x9 = 72 MHz
 * --------------------------------------------------------- */
void SystemClock_Config()
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;

    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ---------------------------------------------------------
 * GPIO
 *
 * PC13 = Blue Pill onboard LED
 * --------------------------------------------------------- */
static void MX_GPIO_Init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*
     * Blue Pill onboard LED is active-low.
     * SET = LED OFF.
     */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

/* ---------------------------------------------------------
 * USART1
 *
 * PA9  = TX
 * PA10 = RX
 * Baud = 115200
 * --------------------------------------------------------- */
static void MX_USART1_UART_Init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    /* PA9 - USART1 TX */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA10 - USART1 RX */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;

    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
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

    /*
     * Rapid LED blinking indicates a stack overflow.
     */
    __disable_irq();

    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        for (volatile uint32_t i = 0; i < 100000; i++)
        {
        }
    }
}

/* ---------------------------------------------------------
 * Error Handler
 * --------------------------------------------------------- */
void Error_Handler()
{
    __disable_irq();

    while (1)
    {
        /*
         * Rapid LED blinking indicates an initialization
         * or FreeRTOS object creation failure.
         */
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        for (volatile uint32_t i = 0; i < 300000; i++)
        {
        }
    }
}