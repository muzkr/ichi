
#include "board.h"
#include "main.h"
#include "log.h"

// PC13: Flashlight
#define FLASHLIGHT_GPIOx GPIOC
#define FLASHLIGHT_PIN LL_GPIO_PIN_13
// PF8: Backlight
#define BACKLIGHT_GPIOx GPIOF
#define BACKLIGHT_PIN LL_GPIO_PIN_8

// PA3: SPI flash CS
#define SPI_FLASH_CS_GPIOx GPIOA
#define SPI_FLASH_CS_PIN LL_GPIO_PIN_3
// PA6: LCD A0
#define LCD_A0_GPIOx GPIOA
#define LCD_A0_PIN LL_GPIO_PIN_6
// PB2: LCD CS
#define LCD_CS_GPIOx GPIOB
#define LCD_CS_PIN LL_GPIO_PIN_2

void board_init()
{
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA | LL_IOP_GRP1_PERIPH_GPIOB | LL_IOP_GRP1_PERIPH_GPIOC | LL_IOP_GRP1_PERIPH_GPIOF);

    LL_GPIO_ResetOutputPin(BACKLIGHT_GPIOx, BACKLIGHT_PIN);
    LL_GPIO_ResetOutputPin(FLASHLIGHT_GPIOx, FLASHLIGHT_PIN);
    LL_GPIO_SetOutputPin(SPI_FLASH_CS_GPIOx, SPI_FLASH_CS_PIN);
    LL_GPIO_SetOutputPin(LCD_CS_GPIOx, LCD_CS_PIN);

    // Output ----

    LL_GPIO_InitTypeDef InitStruct = {0};
    InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    InitStruct.Pull = LL_GPIO_PULL_UP;

    // PA ----

    // SPI flash CS
    // LCD A0
    InitStruct.Pin = SPI_FLASH_CS_PIN | LCD_A0_PIN;
    LL_GPIO_Init(GPIOA, &InitStruct);

    // PB ---

    // LCD CS
    InitStruct.Pin = LCD_CS_PIN;
    LL_GPIO_Init(GPIOB, &InitStruct);

    // PC ------

    // Flashlight
    InitStruct.Pin = FLASHLIGHT_PIN;
    LL_GPIO_Init(GPIOC, &InitStruct);

    // PF ------

    // Backlight
    InitStruct.Pin = BACKLIGHT_PIN;
    LL_GPIO_Init(GPIOF, &InitStruct);
}

// Backlight ----------

enum
{
    BL_STATIC,
    BL_DELAY,
    BL_FLASH,
};

static struct
{
    uint8_t mode;
    uint32_t delay;
    uint32_t time;
} backlight_state = {0};

void board_backlight_on(uint32_t delay)
{
    LL_GPIO_SetOutputPin(FLASHLIGHT_GPIOx, FLASHLIGHT_PIN);
    LL_GPIO_SetOutputPin(BACKLIGHT_GPIOx, BACKLIGHT_PIN);
    if (0 == delay)
    {
        backlight_state.mode = BL_STATIC;
    }
    else
    {
        backlight_state.mode = BL_DELAY;
        backlight_state.delay = delay;
        backlight_state.time = main_timestamp();
    }
}

void board_backlight_off()
{
    LL_GPIO_ResetOutputPin(FLASHLIGHT_GPIOx, FLASHLIGHT_PIN);
    LL_GPIO_ResetOutputPin(BACKLIGHT_GPIOx, BACKLIGHT_PIN);
    backlight_state.mode = BL_STATIC;
}

void board_backlight_flash(uint32_t delay)
{
    backlight_state.mode = BL_FLASH;
    backlight_state.time = main_timestamp();
    backlight_state.delay = delay / 2;
}

void board_backlight_update()
{
    if (BL_STATIC == backlight_state.mode)
    {
        return;
    }

    uint32_t t = main_timestamp();
    uint32_t dt = t - backlight_state.time;
    if (dt >= backlight_state.delay)
    {
        if (BL_DELAY == backlight_state.mode)
        {
            LL_GPIO_ResetOutputPin(BACKLIGHT_GPIOx, BACKLIGHT_PIN);
            backlight_state.mode = BL_STATIC;
        }
        else // Flash
        {
            LL_GPIO_TogglePin(BACKLIGHT_GPIOx, BACKLIGHT_PIN);
            backlight_state.time = t;
        }
    }
}
