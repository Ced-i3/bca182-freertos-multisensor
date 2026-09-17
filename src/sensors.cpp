#include "sensors.h"

namespace
{
    GPIO_TypeDef *dhtPort = nullptr;
    uint16_t dhtPin = 0;

    /*
     * DWT cycle counter gives us microsecond timing.
     * STM32F103 is running at 72 MHz.
     */
    void DWT_Init()
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }

    void delay_us(uint32_t microseconds)
    {
        uint32_t start = DWT->CYCCNT;
        uint32_t cycles = microseconds * (HAL_RCC_GetHCLKFreq() / 1000000U);

        while ((DWT->CYCCNT - start) < cycles)
        {
        }
    }

    void SetPinOutput()
    {
        GPIO_InitTypeDef GPIO_InitStruct = {0};

        GPIO_InitStruct.Pin = dhtPin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

        HAL_GPIO_Init(dhtPort, &GPIO_InitStruct);
    }

    void SetPinInput()
    {
        GPIO_InitTypeDef GPIO_InitStruct = {0};

        GPIO_InitStruct.Pin = dhtPin;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;

        HAL_GPIO_Init(dhtPort, &GPIO_InitStruct);
    }

    bool WaitForPinState(
        GPIO_PinState state,
        uint32_t timeout_us)
    {
        uint32_t start = DWT->CYCCNT;
        uint32_t timeout_cycles =
            timeout_us * (HAL_RCC_GetHCLKFreq() / 1000000U);

        while (HAL_GPIO_ReadPin(dhtPort, dhtPin) != state)
        {
            if ((DWT->CYCCNT - start) >= timeout_cycles)
            {
                return false;
            }
        }

        return true;
    }

    bool ReadBit(uint8_t &bit)
    {
        /*
         * Each DHT22 bit starts with approximately
         * 50 us LOW followed by a HIGH pulse.
         *
         * The HIGH pulse duration determines:
         *
         * ~26-28 us -> 0
         * ~70 us    -> 1
         */

        if (!WaitForPinState(GPIO_PIN_RESET, 100))
        {
            return false;
        }

        if (!WaitForPinState(GPIO_PIN_SET, 100))
        {
            return false;
        }

        /*
         * Sample around 40 us after the HIGH begins.
         */
        delay_us(40);

        if (HAL_GPIO_ReadPin(dhtPort, dhtPin) == GPIO_PIN_SET)
        {
            bit = 1;
        }
        else
        {
            bit = 0;
        }

        /*
         * Wait for the HIGH pulse to finish.
         */
        if (!WaitForPinState(GPIO_PIN_RESET, 100))
        {
            return false;
        }

        return true;
    }
}

/* ---------------------------------------------------------
 * DHT22 Initialization
 * --------------------------------------------------------- */
void DHT22_Init(GPIO_TypeDef *port, uint16_t pin)
{
    dhtPort = port;
    dhtPin = pin;

    /*
     * Enable the GPIO clock for the selected port.
     */
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

    DWT_Init();

    SetPinOutput();

    /*
     * DHT22 data line normally stays HIGH.
     */
    HAL_GPIO_WritePin(dhtPort, dhtPin, GPIO_PIN_SET);
}

/* ---------------------------------------------------------
 * DHT22 Read
 * --------------------------------------------------------- */
bool DHT22_Read(float *temperature, float *humidity)
{
    if (dhtPort == nullptr ||
        dhtPin == 0 ||
        temperature == nullptr ||
        humidity == nullptr)
    {
        return false;
    }

    uint8_t data[5] = {0};

    /*
     * -----------------------------------------------------
     * 1. Send start signal
     * -----------------------------------------------------
     */

    SetPinOutput();

    /*
     * Pull the line LOW for at least 1 ms.
     */
    HAL_GPIO_WritePin(dhtPort, dhtPin, GPIO_PIN_RESET);
    HAL_Delay(2);

    /*
     * Release the line.
     */
    HAL_GPIO_WritePin(dhtPort, dhtPin, GPIO_PIN_SET);
    delay_us(30);

    /*
     * -----------------------------------------------------
     * 2. Wait for DHT22 response
     * -----------------------------------------------------
     */

    SetPinInput();

    /*
     * DHT22 responds with:
     *
     * LOW  ~80 us
     * HIGH ~80 us
     * LOW  ~50 us
     */

    if (!WaitForPinState(GPIO_PIN_RESET, 100))
    {
        return false;
    }

    if (!WaitForPinState(GPIO_PIN_SET, 100))
    {
        return false;
    }

    if (!WaitForPinState(GPIO_PIN_RESET, 100))
    {
        return false;
    }

    /*
     * -----------------------------------------------------
     * 3. Read 40 bits
     * -----------------------------------------------------
     */

    for (uint8_t byteIndex = 0; byteIndex < 5; byteIndex++)
    {
        for (uint8_t bitIndex = 0; bitIndex < 8; bitIndex++)
        {
            uint8_t bit = 0;

            if (!ReadBit(bit))
            {
                return false;
            }

            data[byteIndex] <<= 1;
            data[byteIndex] |= bit;
        }
    }

    /*
     * -----------------------------------------------------
     * 4. Verify checksum
     * -----------------------------------------------------
     */

    uint8_t checksum =
        static_cast<uint8_t>(
            data[0] +
            data[1] +
            data[2] +
            data[3]);

    if (checksum != data[4])
    {
        return false;
    }

    /*
     * -----------------------------------------------------
     * 5. Convert humidity
     * -----------------------------------------------------
     *
     * DHT22 humidity:
     *
     * data[0:1] / 10
     */

    uint16_t rawHumidity =
        (static_cast<uint16_t>(data[0]) << 8) |
        data[1];

    *humidity = rawHumidity / 10.0f;

    /*
     * -----------------------------------------------------
     * 6. Convert temperature
     * -----------------------------------------------------
     *
     * Bit 15 indicates negative temperature.
     */

    uint16_t rawTemperature =
        (static_cast<uint16_t>(data[2]) << 8) |
        data[3];

    if (rawTemperature & 0x8000)
    {
        rawTemperature &= 0x7FFF;

        *temperature =
            -(rawTemperature / 10.0f);
    }
    else
    {
        *temperature =
            rawTemperature / 10.0f;
    }

    /*
     * Return the line to its normal idle state.
     */
    SetPinOutput();
    HAL_GPIO_WritePin(dhtPort, dhtPin, GPIO_PIN_SET);

    return true;
}

/* ---------------------------------------------------------
 * LDR Read
 *
 * The LDR is connected to ADC1 channel 0 (PA0). The raw 12-bit
 * conversion is represented as a relative 0-100 level; it is not
 * presented as calibrated lux.
 * --------------------------------------------------------- */
bool LDR_Read(ADC_HandleTypeDef *hadc, int *lightLevel)
{
    if (hadc == nullptr || lightLevel == nullptr)
    {
        return false;
    }

    if (HAL_ADC_Start(hadc) != HAL_OK)
    {
        return false;
    }

    if (HAL_ADC_PollForConversion(hadc, 10) != HAL_OK)
    {
        (void)HAL_ADC_Stop(hadc);
        return false;
    }

    uint32_t rawValue = HAL_ADC_GetValue(hadc);

    if (HAL_ADC_Stop(hadc) != HAL_OK)
    {
        return false;
    }

    *lightLevel = static_cast<int>((rawValue * 100U + 2047U) / 4095U);
    return true;
}
