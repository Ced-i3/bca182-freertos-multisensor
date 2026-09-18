#include "display.h"
#include "system_state.h"

extern "C"
{
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
}

#include <cstring>

/* ============================================================
 * SSD1306 I2C constants
 * ============================================================ */

static constexpr uint8_t SSD1306_I2C_ADDR = 0x3C;
static constexpr uint8_t SSD1306_CMD      = 0x00;
static constexpr uint8_t SSD1306_DATA     = 0x40;

static constexpr int SSD1306_WIDTH  = 128;
static constexpr int SSD1306_HEIGHT = 64;
static constexpr int SSD1306_PAGES  = SSD1306_HEIGHT / 8;

/* ============================================================
 * 5x7 ASCII font — characters 32 (space) through 126 (~).
 * 5 bytes per glyph, column-major, MSB = top row.
 * ============================================================ */

static const uint8_t Font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* 32 (space) */
    {0x00,0x00,0x5F,0x00,0x00}, /* 33 ! */
    {0x00,0x07,0x00,0x07,0x00}, /* 34 " */
    {0x14,0x7F,0x14,0x7F,0x14}, /* 35 # */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* 36 $ */
    {0x23,0x13,0x08,0x64,0x62}, /* 37 % */
    {0x36,0x49,0x55,0x22,0x50}, /* 38 & */
    {0x00,0x05,0x03,0x00,0x00}, /* 39 ' */
    {0x00,0x1C,0x22,0x41,0x00}, /* 40 ( */
    {0x00,0x41,0x22,0x1C,0x00}, /* 41 ) */
    {0x14,0x08,0x3E,0x08,0x14}, /* 42 * */
    {0x08,0x08,0x3E,0x08,0x08}, /* 43 + */
    {0x00,0x50,0x30,0x00,0x00}, /* 44 , */
    {0x08,0x08,0x08,0x08,0x08}, /* 45 - */
    {0x00,0x60,0x60,0x00,0x00}, /* 46 . */
    {0x20,0x10,0x08,0x04,0x02}, /* 47 / */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 48 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 49 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 50 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 51 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 52 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 53 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 54 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 55 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 56 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 57 9 */
    {0x00,0x36,0x36,0x00,0x00}, /* 58 : */
    {0x00,0x56,0x36,0x00,0x00}, /* 59 ; */
    {0x08,0x14,0x22,0x41,0x00}, /* 60 < */
    {0x14,0x14,0x14,0x14,0x14}, /* 61 = */
    {0x00,0x41,0x22,0x14,0x08}, /* 62 > */
    {0x02,0x01,0x51,0x09,0x06}, /* 63 ? */
    {0x32,0x49,0x79,0x41,0x3E}, /* 64 @ */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 65 A */
    {0x7F,0x49,0x49,0x49,0x36}, /* 66 B */
    {0x3E,0x41,0x41,0x41,0x22}, /* 67 C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 68 D */
    {0x7F,0x49,0x49,0x49,0x41}, /* 69 E */
    {0x7F,0x09,0x09,0x09,0x01}, /* 70 F */
    {0x3E,0x41,0x49,0x49,0x7A}, /* 71 G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 72 H */
    {0x00,0x41,0x7F,0x41,0x00}, /* 73 I */
    {0x20,0x40,0x41,0x3F,0x01}, /* 74 J */
    {0x7F,0x08,0x14,0x22,0x41}, /* 75 K */
    {0x7F,0x40,0x40,0x40,0x40}, /* 76 L */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* 77 M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 78 N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 79 O */
    {0x7F,0x09,0x09,0x09,0x06}, /* 80 P */
    {0x3E,0x41,0x51,0x21,0x5E}, /* 81 Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* 82 R */
    {0x46,0x49,0x49,0x49,0x31}, /* 83 S */
    {0x01,0x01,0x7F,0x01,0x01}, /* 84 T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 85 U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* 86 V */
    {0x3F,0x40,0x38,0x40,0x3F}, /* 87 W */
    {0x63,0x14,0x08,0x14,0x63}, /* 88 X */
    {0x07,0x08,0x70,0x08,0x07}, /* 89 Y */
    {0x61,0x51,0x49,0x45,0x43}, /* 90 Z */
    {0x00,0x7F,0x41,0x41,0x00}, /* 91 [ */
    {0x02,0x04,0x08,0x10,0x20}, /* 92 backslash */
    {0x00,0x41,0x41,0x7F,0x00}, /* 93 ] */
    {0x04,0x02,0x01,0x02,0x04}, /* 94 ^ */
    {0x40,0x40,0x40,0x40,0x40}, /* 95 _ */
    {0x00,0x01,0x02,0x04,0x00}, /* 96 ` */
    {0x20,0x54,0x54,0x54,0x78}, /* 97 a */
    {0x7F,0x48,0x44,0x44,0x38}, /* 98 b */
    {0x38,0x44,0x44,0x44,0x20}, /* 99 c */
    {0x38,0x44,0x44,0x48,0x7F}, /* 100 d */
    {0x38,0x54,0x54,0x54,0x18}, /* 101 e */
    {0x08,0x7E,0x09,0x01,0x02}, /* 102 f */
    {0x0C,0x52,0x52,0x52,0x3E}, /* 103 g */
    {0x7F,0x08,0x04,0x04,0x78}, /* 104 h */
    {0x00,0x44,0x7D,0x40,0x00}, /* 105 i */
    {0x20,0x40,0x44,0x3D,0x00}, /* 106 j */
    {0x7F,0x10,0x28,0x44,0x00}, /* 107 k */
    {0x00,0x41,0x7F,0x40,0x00}, /* 108 l */
    {0x7C,0x04,0x18,0x04,0x78}, /* 109 m */
    {0x7C,0x08,0x04,0x04,0x78}, /* 110 n */
    {0x38,0x44,0x44,0x44,0x38}, /* 111 o */
    {0x7C,0x14,0x14,0x14,0x08}, /* 112 p */
    {0x08,0x14,0x14,0x18,0x7C}, /* 113 q */
    {0x7C,0x08,0x04,0x04,0x08}, /* 114 r */
    {0x48,0x54,0x54,0x54,0x20}, /* 115 s */
    {0x04,0x3F,0x44,0x40,0x20}, /* 116 t */
    {0x3C,0x40,0x40,0x20,0x7C}, /* 117 u */
    {0x1C,0x20,0x40,0x20,0x1C}, /* 118 v */
    {0x3C,0x40,0x30,0x40,0x3C}, /* 119 w */
    {0x44,0x28,0x10,0x28,0x44}, /* 120 x */
    {0x0C,0x50,0x50,0x50,0x3C}, /* 121 y */
    {0x44,0x64,0x54,0x4C,0x44}, /* 122 z */
    {0x00,0x08,0x36,0x41,0x00}, /* 123 { */
    {0x00,0x00,0x7F,0x00,0x00}, /* 124 | */
    {0x00,0x41,0x36,0x08,0x00}, /* 125 } */
    {0x10,0x08,0x08,0x10,0x08}, /* 126 ~ */
};

static I2C_HandleTypeDef *oledI2C = nullptr;
static uint8_t displayBuffer[SSD1306_WIDTH * SSD1306_PAGES];

/* ============================================================
 * Low-level I2C helpers
 * ============================================================ */

static void ssd1306_Command(uint8_t cmd)
{
    uint8_t buf[2] = { SSD1306_CMD, cmd };
    HAL_I2C_Master_Transmit(oledI2C, SSD1306_I2C_ADDR << 1, buf, 2, 100);
}

static void ssd1306_Data(const uint8_t *data, uint16_t len)
{
    uint8_t header = SSD1306_DATA;
    HAL_I2C_Master_Transmit(oledI2C, SSD1306_I2C_ADDR << 1, &header, 1, 100);
    HAL_I2C_Master_Transmit(oledI2C, SSD1306_I2C_ADDR << 1, const_cast<uint8_t *>(data), len, 100);
}

/* ============================================================
 * SSD1306 initialization
 * ============================================================ */

static void ssd1306_Init()
{
    HAL_Delay(100);

    ssd1306_Command(0xAE);
    ssd1306_Command(0x20);
    ssd1306_Command(0x10);
    ssd1306_Command(0xB0);
    ssd1306_Command(0xC8);
    ssd1306_Command(0x00);
    ssd1306_Command(0x10);
    ssd1306_Command(0x40);
    ssd1306_Command(0x81);
    ssd1306_Command(0xCF);
    ssd1306_Command(0xA1);
    ssd1306_Command(0xA6);
    ssd1306_Command(0xA8);
    ssd1306_Command(0x3F);
    ssd1306_Command(0xA4);
    ssd1306_Command(0xD3);
    ssd1306_Command(0x00);
    ssd1306_Command(0xD5);
    ssd1306_Command(0xF0);
    ssd1306_Command(0xD9);
    ssd1306_Command(0x22);
    ssd1306_Command(0xDA);
    ssd1306_Command(0x12);
    ssd1306_Command(0xDB);
    ssd1306_Command(0x20);
    ssd1306_Command(0x8D);
    ssd1306_Command(0x14);
    ssd1306_Command(0xAF);

    HAL_Delay(100);
}

/* ============================================================
 * Buffer operations
 * ============================================================ */

void Display_Clear()
{
    memset(displayBuffer, 0, sizeof(displayBuffer));
}

static void Display_Flush()
{
    ssd1306_Command(0x21);
    ssd1306_Command(0x00);
    ssd1306_Command(0x7F);
    ssd1306_Command(0x22);
    ssd1306_Command(0x00);
    ssd1306_Command(0x07);

    for (uint8_t page = 0; page < SSD1306_PAGES; page++)
    {
        ssd1306_Data(&displayBuffer[page * SSD1306_WIDTH], SSD1306_WIDTH);
    }
}

void Display_DrawString(
    uint8_t page,
    uint8_t col,
    const char *str)
{
    while (*str && col < SSD1306_WIDTH - 5)
    {
        char c = *str++;
        if (c < 32 || c > 126)
        {
            c = ' ';
        }
        uint8_t idx = c - 32;
        for (uint8_t i = 0; i < 5; i++)
        {
            displayBuffer[page * SSD1306_WIDTH + col + i] =
                Font5x7[idx][i];
        }
        col += 6;
    }
}

static void Display_DrawInt(uint8_t page, uint8_t col, int value)
{
    char buf[12];
    int i = 0;
    if (value < 0)
    {
        buf[i++] = '-';
        value = -value;
    }
    char tmp[10];
    int j = 0;
    if (value == 0)
    {
        tmp[j++] = '0';
    }
    else
    {
        while (value > 0)
        {
            tmp[j++] = '0' + (value % 10);
            value /= 10;
        }
    }
    for (int k = j - 1; k >= 0; k--)
    {
        buf[i++] = tmp[k];
    }
    buf[i] = '\0';
    Display_DrawString(page, col, buf);
}

static void Display_DrawFloat(uint8_t page, uint8_t col, float value)
{
    int whole = static_cast<int>(value);
    int frac = static_cast<int>((value - static_cast<float>(whole)) * 10.0f);
    if (frac < 0) frac = -frac;
    Display_DrawInt(page, col, whole);
    /* Count digits in integer part to find column after it. */
    int idx = 0;
    {
        int tmp = whole < 0 ? -whole : whole;
        if (tmp == 0)
            idx = 1;
        while (tmp > 0) { idx++; tmp /= 10; }
        if (whole < 0) idx++; /* for the minus sign */
    }
    int endCol = col + idx * 6;
    Display_DrawString(page, endCol, ".");
    Display_DrawInt(page, endCol + 6, frac);
}

/* ============================================================
 * Public interface
 * ============================================================ */

void Display_Init(I2C_HandleTypeDef *hi2c)
{
    oledI2C = hi2c;
    ssd1306_Init();
    Display_Clear();
    Display_Flush();
}

/* ============================================================
 * DisplayTask
 *
 * Priority: 2
 * Stack:    512 words (2048 bytes — includes 1024-byte display buffer)
 * Period:   200 ms via vTaskDelayUntil()
 *
 * Uses xQueuePeek (non-destructive read) so SensorTask always
 * owns the queue data.  The display mode is read through the
 * mutex-protected Input_GetDisplayMode().
 * ============================================================ */

static void DisplayTask(void *pvParameters)
{
    (void)pvParameters;

    extern QueueHandle_t sensorQueue;
    SensorData sensorData = {};
    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        /* Peek latest sample without removing it from the queue. */
        (void)xQueuePeek(sensorQueue, &sensorData, 0);

        DisplayMode mode = Input_GetDisplayMode();
        ActivityState activity = SystemState_GetActivityState();

        Display_Clear();

        /* Row 0 — mode header */
        switch (mode)
        {
            case DisplayMode::TEMPERATURE:
                Display_DrawString(0, 0, "-- Temperature --");
                break;
            case DisplayMode::HUMIDITY:
                Display_DrawString(0, 0, "--- Humidity  ---");
                break;
            case DisplayMode::LIGHT:
                Display_DrawString(0, 0, "--- Light     ---");
                break;
            case DisplayMode::MOTION:
                Display_DrawString(0, 0, "--- Motion    ---");
                break;
        }

        /* Row 2 — value */
        switch (mode)
        {
            case DisplayMode::TEMPERATURE:
                Display_DrawString(2, 0, "Temp: ");
                Display_DrawFloat(2, 36, sensorData.temperature);
                Display_DrawString(2, 84, " C");
                break;
            case DisplayMode::HUMIDITY:
                Display_DrawString(2, 0, "Hum:  ");
                Display_DrawFloat(2, 36, sensorData.humidity);
                Display_DrawString(2, 84, " %");
                break;
            case DisplayMode::LIGHT:
                Display_DrawString(2, 0, "Light: ");
                Display_DrawInt(2, 42, sensorData.lightLevel);
                Display_DrawString(2, 78, " /100");
                break;
            case DisplayMode::MOTION:
                Display_DrawString(2, 0, "Motion: ");
                if (sensorData.motionDetected)
                    Display_DrawString(2, 48, "YES");
                else
                    Display_DrawString(2, 48, "NO");
                break;
        }

        /* Row 5 — activity state */
        if (activity == ActivityState::ACTIVE)
            Display_DrawString(5, 0, "State: ACTIVE");
        else
            Display_DrawString(5, 0, "State: INACTIVE");

        Display_Flush();

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(200));
    }
}

bool DisplayTask_Create()
{
    return xTaskCreate(
               DisplayTask,
               "DisplayTask",
               352,
               nullptr,
               2,
               nullptr) == pdPASS;
}
