#include "lcd.h"
#include <stdint.h>
#include <stddef.h>
#include "py32f071_ll_gpio.h"
#include "py32f071_ll_spi.h"
#include "py32f071_ll_bus.h"
#include "py32f071_ll_utils.h"

#define SPIx SPI1
#define SYSTEM_DelayMs LL_mDelay

#define LCD_WIDTH 128
#define LCD_HEIGHT 64

static void SPI_Init()
{
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SPI1);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);

    do
    {
        LL_GPIO_InitTypeDef InitStruct;
        LL_GPIO_StructInit(&InitStruct);
        InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
        InitStruct.Alternate = LL_GPIO_AF0_SPI1;
        InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;

        // SCK: PA5
        InitStruct.Pin = LL_GPIO_PIN_5;
        InitStruct.Pull = LL_GPIO_PULL_UP;
        LL_GPIO_Init(GPIOA, &InitStruct);

        // SDA: PA7
        InitStruct.Pin = LL_GPIO_PIN_7;
        InitStruct.Pull = LL_GPIO_PULL_NO;
        LL_GPIO_Init(GPIOA, &InitStruct);
    } while (0);

    LL_SPI_InitTypeDef InitStruct;
    LL_SPI_StructInit(&InitStruct);
    InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
    InitStruct.Mode = LL_SPI_MODE_MASTER;
    InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
    InitStruct.ClockPolarity = LL_SPI_POLARITY_HIGH;
    InitStruct.ClockPhase = LL_SPI_PHASE_2EDGE;
    InitStruct.NSS = LL_SPI_NSS_SOFT;
    InitStruct.BitOrder = LL_SPI_MSB_FIRST;
    InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
    InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV64;
    LL_SPI_Init(SPIx, &InitStruct);

    LL_SPI_Enable(SPIx);
}

static inline void CS_Assert()
{
    // PB2
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_2);
}

static inline void CS_Release()
{
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_2);
}

static inline void A0_Set()
{
    // PA6
    LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_6);
}

static inline void A0_Reset()
{
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_6);
}

static uint8_t SPI_WriteByte(uint8_t Value)
{
    while (!LL_SPI_IsActiveFlag_TXE(SPIx))
        ;

    LL_SPI_TransmitData8(SPIx, Value);

    while (!LL_SPI_IsActiveFlag_RXNE(SPIx))
        ;

    return LL_SPI_ReceiveData8(SPIx);
}

// Software reset
#define ST7565_CMD_SOFTWARE_RESET 0xE2
// Bias Select
// 1 0 1 0 0 0 1 BS
// Select bias setting 0=1/9  1=1/7 (at 1/65 duty)
#define ST7565_CMD_BIAS_SELECT 0xA2
// COM Direction
// 1 1 0 0 MY - - -
// Set output direction of COM
// MY=1, reverse direction
// MY=0, normal direction
#define ST7565_CMD_COM_DIRECTION 0xC0
// SEG Direction
// 1 0 1 0 0 0 0 MX
// Set scan direction of SEG
// MX=1, reverse direction
// MX=0, normal direction
#define ST7565_CMD_SEG_DIRECTION 0xA0
// Inverse Display
// 1 0 1 0 0 1 1 INV
// INV =1, inverse display
// INV =0, normal display
#define ST7565_CMD_INVERSE_DISPLAY 0xA6
// All Pixel ON
// 1 0 1 0 0 1 0 AP
// AP=1, set all pixel ON
// AP=0, normal display
#define ST7565_CMD_ALL_PIXEL_ON 0xA4
// Regulation Ratio
// 0 0 1 0 0 RR2 RR1 RR0
// This instruction controls the regulation ratio of the built-in regulator
#define ST7565_CMD_REGULATION_RATIO 0x20
// Double command!! Set electronic volume (EV) level
// Send next: 0 0 EV5 EV4 EV3 EV2 EV1 EV0  contrast 0-63
#define ST7565_CMD_SET_EV 0x81
// Control built-in power circuit ON/OFF - 0 0 1 0 1 VB VR VF
// VB: Built-in Booster
// VR: Built-in Regulator
// VF: Built-in Follower
#define ST7565_CMD_POWER_CIRCUIT 0x28
// Set display start line 0-63
// 0 0 0 1 S5 S4 S3 S2 S1 S0
#define ST7565_CMD_SET_START_LINE 0x40
// Display ON/OFF
// 0 0 1 0 1 0 1 1 1 D
// D=1, display ON
// D=0, display OFF
#define ST7565_CMD_DISPLAY_ON_OFF 0xAE

static const uint8_t cmds[] = {
    ST7565_CMD_BIAS_SELECT | 0,             // Select bias setting: 1/9
    ST7565_CMD_COM_DIRECTION | (0 << 3),    // Set output direction of COM: normal
    ST7565_CMD_SEG_DIRECTION | 1,           // Set scan direction of SEG: reverse
    ST7565_CMD_INVERSE_DISPLAY | 0,         // Inverse Display: false
    ST7565_CMD_ALL_PIXEL_ON | 0,            // All Pixel ON: false - normal display
    ST7565_CMD_REGULATION_RATIO | (4 << 0), // Regulation Ratio 5.0

    ST7565_CMD_SET_EV, // Set contrast
    31,

    ST7565_CMD_POWER_CIRCUIT | 0b111, // Built-in power circuit ON/OFF: VB=1 VR=1 VF=1
    ST7565_CMD_SET_START_LINE | 0,    // Set Start Line: 0
    ST7565_CMD_DISPLAY_ON_OFF | 1,    // Display ON/OFF: ON
};

/**
 *  Write a command (rather than pixel data)
 */
static void ST7565_WriteByte(uint8_t Value)
{
    A0_Reset();
    SPI_WriteByte(Value);
}

static void ST7565_SelectColumnAndLine(uint8_t Column, uint8_t Line)
{
    A0_Reset();
    SPI_WriteByte(0xb0 | Line);
    SPI_WriteByte(0x10 | (Column >> 4));
    SPI_WriteByte(0x0F & Column);
}

static void DrawLine(uint8_t column, uint8_t line, const uint8_t *lineBuffer, uint32_t size_defVal)
{
    ST7565_SelectColumnAndLine(column + 4, line);
    A0_Set();
    for (unsigned i = 0; i < size_defVal; i++)
    {
        SPI_WriteByte(lineBuffer[i]);
    }
}

static void ST7565_FillScreen(uint8_t value)
{
    CS_Assert();
    for (unsigned i = 0; i < LCD_HEIGHT; i++)
    {
        ST7565_SelectColumnAndLine(4, i);
        A0_Set();
        for (uint32_t x = 0; x < LCD_WIDTH; x++)
        {
            SPI_WriteByte(value);
        }
    }
    CS_Release();
}

void ST7565_Init(void)
{
    SPI_Init();
    CS_Assert();
    ST7565_WriteByte(ST7565_CMD_SOFTWARE_RESET); // software reset
    SYSTEM_DelayMs(120);

    for (uint8_t i = 0; i < 8; i++)
    {
        ST7565_WriteByte(cmds[i]);
    }

    ST7565_WriteByte(ST7565_CMD_POWER_CIRCUIT | 0b011); // VB=0 VR=1 VF=1
    SYSTEM_DelayMs(1);
    ST7565_WriteByte(ST7565_CMD_POWER_CIRCUIT | 0b110); // VB=1 VR=1 VF=0
    SYSTEM_DelayMs(1);

    for (uint8_t i = 0; i < 4; i++)                         // why 4 times?
        ST7565_WriteByte(ST7565_CMD_POWER_CIRCUIT | 0b111); // VB=1 VR=1 VF=1

    SYSTEM_DelayMs(40);

    ST7565_WriteByte(ST7565_CMD_SET_START_LINE | 0); // line 0
    ST7565_WriteByte(ST7565_CMD_DISPLAY_ON_OFF | 1); // D=1

    CS_Release();

    ST7565_FillScreen(0x00);
}

static const uint8_t C[] = {0xE0, 0xF0, 0x18, 0x08, 0x08, 0x18, 0x30, /*0x00,*/ 0x07, 0x0F, 0x18, 0x10, 0x10, 0x18, 0x0C}; // 'C'
static const uint8_t H[] = {0xF8, 0xF8, 0x80, 0x80, 0x80, 0xF8, 0xF8, /*0x00,*/ 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x1F, 0x1F}; // 'H'
static const uint8_t I[] = {0x00, 0x00, 0x08, 0xF8, 0xF8, 0x08, 0x00, /*0x00,*/ 0x00, 0x00, 0x10, 0x1F, 0x1F, 0x10, 0x00}; // 'I'

#define FONT_WIDTH 7

static const uint8_t *LOGO[] = {I, C, H, I};

#define LOGO_LEN (sizeof(LOGO) / sizeof(uint8_t *))

#define LOGO_TOP 3
#define LOGO_FONT_WIDTH 10
#define LOGO_LEFT ((LCD_WIDTH - LOGO_LEN * LOGO_FONT_WIDTH) / 2)

void lcd_init()
{
    ST7565_Init();
}

void lcd_clear()
{
    ST7565_FillScreen(0);
}

void lcd_display_logo()
{
    ST7565_FillScreen(0);

    CS_Assert();

    for (uint32_t y = 0; y < 2; y++)
    {
        uint32_t y1 = y + LOGO_TOP;
        uint32_t off = FONT_WIDTH * y;
        for (uint32_t x = 0; x < 4; x++)
        {
            uint32_t x1 = LOGO_LEFT + x * LOGO_FONT_WIDTH;
            DrawLine(x1, y1, LOGO[x] + off, FONT_WIDTH);
        }
    }

    CS_Release();
}
