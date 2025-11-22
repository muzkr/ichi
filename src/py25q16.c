/* Copyright 2025 muzkr
 * https://github.com/muzkr
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include <string.h>

#include "py25q16.h"
#include "py32f071_ll_gpio.h"
#include "py32f071_ll_bus.h"
#include "py32f071_ll_system.h"
#include "py32f071_ll_spi.h"
#include "py32f071_ll_utils.h"

#define SPIx SPI2

#define CS_PORT GPIOA
#define CS_PIN LL_GPIO_PIN_3

#define CS_ASSERT() LL_GPIO_ResetOutputPin(CS_PORT, CS_PIN)
#define CS_RELEASE() LL_GPIO_SetOutputPin(CS_PORT, CS_PIN)

#define SECTOR_SIZE 0x1000

static uint32_t SectorCacheAddr = 0x1000000;
static uint8_t SectorCache[SECTOR_SIZE];
// static uint8_t BlackHole[1];

static void SPI_Init()
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI2);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);

    do
    {
        // SCK: PA0
        // MOSI: PA1
        // MISO: PA2

        LL_GPIO_InitTypeDef InitStruct;
        LL_GPIO_StructInit(&InitStruct);
        InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
        InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
        InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        InitStruct.Pull = LL_GPIO_PULL_UP;

        InitStruct.Pin = LL_GPIO_PIN_0;
        InitStruct.Alternate = LL_GPIO_AF8_SPI2;
        LL_GPIO_Init(GPIOA, &InitStruct);

        InitStruct.Pin = LL_GPIO_PIN_1 | LL_GPIO_PIN_2;
        InitStruct.Alternate = LL_GPIO_AF9_SPI2;
        LL_GPIO_Init(GPIOA, &InitStruct);

    } while (0);

    LL_SPI_InitTypeDef InitStruct;
    LL_SPI_StructInit(&InitStruct);
    InitStruct.Mode = LL_SPI_MODE_MASTER;
    InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
    InitStruct.ClockPhase = LL_SPI_PHASE_2EDGE;
    InitStruct.ClockPolarity = LL_SPI_POLARITY_HIGH;
    InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV2;
    InitStruct.BitOrder = LL_SPI_MSB_FIRST;
    InitStruct.NSS = LL_SPI_NSS_SOFT;
    InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
    LL_SPI_Init(SPIx, &InitStruct);

    LL_SPI_Enable(SPIx);
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

static void WriteAddr(uint32_t Addr);
static uint8_t ReadStatusReg_0();
static void WaitWIP();
static void delay_us(uint32_t n);
static void WriteEnable();
static void SectorErase(uint32_t Addr);
static void SectorProgram(uint32_t Addr, const uint8_t *Buf, uint32_t Size);
static void PageProgram(uint32_t Addr, const uint8_t *Buf);

void py25q16_init()
{
    CS_RELEASE();
    SPI_Init();
}

static void py25q16_read(uint32_t Address, uint8_t *pBuffer, uint32_t Size)
{
#ifdef DEBUG
    printf("spi flash read: %06x %ld\n", Address, Size);
#endif
    CS_ASSERT();

    SPI_WriteByte(0x03); // Fast read
    WriteAddr(Address);

    for (uint32_t i = 0; i < Size; i++)
    {
        pBuffer[i] = SPI_WriteByte(0xff);
    }

    CS_RELEASE();
}

void py25q16_read_page(uint32_t addr, uint8_t *buf)
{
    py25q16_read(addr, buf, PY25Q16_PAGE_SIZE);
}

void py25q16_write_page(uint32_t Address, const uint8_t *pBuffer)
{
    uint32_t SecIndex = Address / SECTOR_SIZE;
    uint32_t SecAddr = SecIndex * SECTOR_SIZE;
    uint32_t SecOffset = Address % SECTOR_SIZE;
    uint32_t SecSize = PY25Q16_PAGE_SIZE;

    if (SecAddr != SectorCacheAddr)
    {
        py25q16_read(SecAddr, SectorCache, SECTOR_SIZE);
        SectorCacheAddr = SecAddr;
    }

    if (0 != memcmp(pBuffer, SectorCache + SecOffset, SecSize))
    {
        bool Erase = false;
        for (uint32_t i = 0; i < SecSize; i++)
        {
            if (0xff != SectorCache[SecOffset + i])
            {
                Erase = true;
                break;
            }
        }

        memcpy(SectorCache + SecOffset, pBuffer, SecSize);

        if (Erase)
        {
            SectorErase(SecAddr);
            SectorProgram(SecAddr, SectorCache, SECTOR_SIZE);
        }
        else
        {
            SectorProgram(Address, pBuffer, SecSize);
        }
    }
}

static inline void WriteAddr(uint32_t Addr)
{
    SPI_WriteByte(0xff & (Addr >> 16));
    SPI_WriteByte(0xff & (Addr >> 8));
    SPI_WriteByte(0xff & Addr);
}

static uint8_t ReadStatusReg_0()
{
    uint8_t Cmd = 0x5;

    CS_ASSERT();
    SPI_WriteByte(Cmd);
    uint8_t Value = SPI_WriteByte(0xff);
    CS_RELEASE();

    return Value;
}

static void delay_us(uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
    {
        for (uint32_t j = 0; j < 16; j++)
        {
            __NOP();
        }
    }
}

static void WaitWIP()
{
    // for (int i = 0; i < 1000000; i++)
    while (1)
    {
        uint8_t Status = ReadStatusReg_0();
        if (1 & Status) // WIP
        {
            // delay_us(10);
            continue;
        }
        break;
    }
}

static void WriteEnable()
{
    CS_ASSERT();
    SPI_WriteByte(0x6);
    CS_RELEASE();
}

static void SectorErase(uint32_t Addr)
{
    WriteEnable();
    WaitWIP();

    CS_ASSERT();
    SPI_WriteByte(0x20);
    WriteAddr(Addr);
    CS_RELEASE();

    WaitWIP();
}

static void SectorProgram(uint32_t Addr, const uint8_t *Buf, uint32_t Size)
{
    while (Size)
    {
        PageProgram(Addr, Buf);
        Addr += PY25Q16_PAGE_SIZE;
        Buf += PY25Q16_PAGE_SIZE;
        Size -= PY25Q16_PAGE_SIZE;
    }
}

static void PageProgram(uint32_t Addr, const uint8_t *Buf)
{
    WriteEnable();
    WaitWIP();

    CS_ASSERT();

    SPI_WriteByte(0x2);
    WriteAddr(Addr);

    for (uint32_t i = 0; i < PY25Q16_PAGE_SIZE; i++)
    {
        SPI_WriteByte(Buf[i]);
    }

    CS_RELEASE();

    WaitWIP();
}
