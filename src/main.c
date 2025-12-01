/**
 ******************************************************************************
 * @file    main.c
 * @author  MCU Application Team
 * @brief   Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2023 Puya Semiconductor Co.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by Puya under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

#include "main.h"
#include "py32f071_it.h"
#include "usb_config.h"
#include "log.h"
#include "board.h"
#include "py25q16.h"
#include "lcd.h"

static void APP_SystemClockConfig();
static void APP_SysTick_Init();
static void APP_USB_Init();

#if defined(ENABLE_LOGGING)
#define USARTx USART1
static void APP_USART_Init();
static void APP_DumpLog();
#endif

static volatile uint32_t timestamp;
static volatile uint32_t schedule_reset_delay = 0;

uint32_t main_timestamp()
{
    return timestamp;
}

void main_schedule_reset(uint32_t delay)
{
    schedule_reset_delay = delay;
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void)
{
    timestamp++;
    board_backlight_update();
}

/**
 * @brief  Main program.
 * @retval int
 */
int main()
{
    /* System clock configuration */
    APP_SystemClockConfig();

    /* Enable SYSCFG and PWR clocks */
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    APP_SysTick_Init();

    board_init();

#if defined(ENABLE_LOGGING)
    APP_USART_Init();
    log_init();
#endif

    log("start\n");

    lcd_init();
    lcd_display_logo();

    board_backlight_on(BOARD_DEFAULT_BACKLIGHT_DELAY);
    py25q16_init();
    APP_USB_Init();

    while (1)
    {
        if (schedule_reset_delay)
        {
            LL_mDelay(schedule_reset_delay);
            board_backlight_off();
            lcd_clear();
            NVIC_SystemReset();
            while (1)
            {
            }
        }

#if defined(ENABLE_LOGGING)
        APP_DumpLog();
#endif
    }
}

static void APP_SysTick_Init()
{
    NVIC_SetPriority(SysTick_IRQn, 0);
    SysTick_Config(SystemCoreClock / 1000);
    // NVIC_EnableIRQ(SysTick_IRQn);
}

#if defined(ENABLE_LOGGING)
static void APP_USART_Init()
{
    // TX: PA9

    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_USART1);

    do
    {
        LL_GPIO_InitTypeDef InitStruct;
        InitStruct.Pin = LL_GPIO_PIN_9;
        InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
        InitStruct.Alternate = LL_GPIO_AF1_USART1;
        InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
        InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        InitStruct.Pull = LL_GPIO_PULL_UP;
        LL_GPIO_Init(GPIOA, &InitStruct);
    } while (0);

    LL_USART_Disable(USARTx);

    do
    {
        LL_USART_InitTypeDef InitStruct;
        LL_USART_StructInit(&InitStruct);
        InitStruct.BaudRate = 38400;
        InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
        InitStruct.StopBits = LL_USART_STOPBITS_1;
        InitStruct.Parity = LL_USART_PARITY_NONE;
        InitStruct.TransferDirection = LL_USART_DIRECTION_TX;
        LL_USART_Init(USARTx, &InitStruct);
    } while (0);

    LL_USART_Enable(USARTx);
    LL_USART_TransmitData8(USARTx, 0);
}

static void APP_DumpLog()
{
    static uint8_t buf[80] = {0};

    const uint32_t size = log_fetch(buf, sizeof(buf));

    for (uint32_t i = 0; i < size; i++)
    {
        while (!LL_USART_IsActiveFlag_TXE(USARTx))
        {
        }
        LL_USART_TransmitData8(USARTx, buf[i]);
    }
}
#endif // ENABLE_LOGGING

/**
 * @brief  USB peripheral initialization function
 * @param  None
 * @retval None
 */
static void APP_USB_Init()
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USBD);
    // LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);

    msc_ram_init();

    /* Enable USB interrupt */
    NVIC_SetPriority(USBD_IRQn, 3);
    NVIC_EnableIRQ(USBD_IRQn);
}

/**
 * @brief  System clock configuration function
 * @param  None
 * @retval None
 */
static void APP_SystemClockConfig()
{
    /* Enable and initialize HSI */
    LL_RCC_HSI_Enable();
    LL_RCC_HSI_SetCalibFreq(LL_RCC_HSICALIBRATION_16MHz);
    while (LL_RCC_HSI_IsReady() != 1)
    {
    }

    LL_RCC_SetHSIDiv(LL_RCC_HSI_DIV_1);

    /* Configure HSISYS as system clock */
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSISYS);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSISYS)
    {
    }

    /* PLL multiplication factor of HSI */
    LL_RCC_PLL_Disable();
    while (LL_RCC_PLL_IsReady() != 0)
    {
    }
    LL_RCC_PLL_SetMainSource(LL_RCC_PLLSOURCE_HSI);
    LL_RCC_PLL_SetMulFactor(LL_RCC_PLLMUL_3);
    LL_RCC_PLL_Enable();
    while (LL_RCC_PLL_IsReady() != 1)
    {
    }

    /* Configure AHB prescaler */
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

    /* Set flash latency */
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1)
    {
    }

    /* Configure PLL as system clock and initialize */
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
    {
    }

    /* Configure APB1 prescaler and initialize */
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    // LL_Init1msTick(48000000);

    /* Update system clock global variable SystemCoreClock (can also be updated by calling SystemCoreClockUpdate function) */
    LL_SetSystemCoreClock(48000000);

    LL_FLASH_TIMMING_SEQUENCE_CONFIG_16M();
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
void APP_ErrorHandler()
{
    /* Infinite loop */
    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT Puya *****END OF FILE******************/
