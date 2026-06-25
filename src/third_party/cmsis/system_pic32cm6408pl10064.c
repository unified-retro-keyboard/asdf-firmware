/*
 * System configuration file for PIC32CM6408PL10064
 *
 * Copyright (c) 2026 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include "pic32cm6408pl10064.h"
#include "system_pic32cm6408pl10064.h"

/*----------------------------------------------------------------------------
  Define clocks
 *----------------------------------------------------------------------------*/
#define SYSTEM_CLOCK    (4000000UL)

/*----------------------------------------------------------------------------
  System Core Clock Variable
 *----------------------------------------------------------------------------*/
uint32_t SystemCoreClock = (uint32_t)SYSTEM_CLOCK;  /* System Core Clock Frequency */


/*----------------------------------------------------------------------------
  Configuration macros
 *----------------------------------------------------------------------------*/

/* Default internal clock frequency after reset */
#define __SYSTEM_CLOCK_DEFAULT   (4000000UL)    /* 4 MHz default on reset */
#define __SYSTEM_OSCHF_FREQ      (4000000UL)    /* 4 MHz default internal oscillator */
#define __SYSTEM_OSC32K_FREQ     (32768UL)
#define __SYSTEM_XOSC32K_FREQ    (32768UL)

/* Optional user-defined external source frequency (if used) */
#define __EXTERNAL_CLOCK_FREQ    (12000000UL)   /* Example: 12 MHz external clock */

/*----------------------------------------------------------------------------
  Internal helper functions
 *----------------------------------------------------------------------------*/

static inline uint32_t GetGenericClockSource(uint32_t generator_id)
{
    uint32_t masked = GCLK_REGS->GCLK_GENCTRL[generator_id] & GCLK_GENCTRL_SRC_Msk;
    return masked >> GCLK_GENCTRL_SRC_Pos;
}

static inline uint32_t GetDivideSelection(uint32_t generator_id)
{
    uint32_t masked = GCLK_REGS->GCLK_GENCTRL[generator_id] & GCLK_GENCTRL_DIVSEL_Msk;
    return masked >> GCLK_GENCTRL_DIVSEL_Pos;
}

static inline uint32_t GetDivisionValue(uint32_t generator_id)
{
    uint32_t masked = GCLK_REGS->GCLK_GENCTRL[generator_id] & GCLK_GENCTRL_DIV_Msk;
    return masked >> GCLK_GENCTRL_DIV_Pos;
}

static inline uint32_t GetOSCCTRLFreq(void)
{
    uint32_t frequency_hz;
    uint32_t masked = OSCCTRL_REGS->OSCCTRL_OSCHFCTRL & OSCCTRL_OSCHFCTRL_FRQSEL_Msk;
    uint32_t frqsel = masked >> OSCCTRL_OSCHFCTRL_FRQSEL_Pos;
    switch (frqsel)
    {
        case OSCCTRL_OSCHFCTRL_FRQSEL_1M_Val:  frequency_hz = 1000000UL;  break;
        case OSCCTRL_OSCHFCTRL_FRQSEL_2M_Val:  frequency_hz = 2000000UL;  break;
        case OSCCTRL_OSCHFCTRL_FRQSEL_3M_Val:  frequency_hz = 3000000UL;  break;
        case OSCCTRL_OSCHFCTRL_FRQSEL_4M_Val:  frequency_hz = 4000000UL;  break;
        case OSCCTRL_OSCHFCTRL_FRQSEL_8M_Val:  frequency_hz = 8000000UL;  break;
        case OSCCTRL_OSCHFCTRL_FRQSEL_12M_Val: frequency_hz = 12000000UL; break;
        case OSCCTRL_OSCHFCTRL_FRQSEL_16M_Val: frequency_hz = 16000000UL; break;
        case OSCCTRL_OSCHFCTRL_FRQSEL_20M_Val: frequency_hz = 20000000UL; break;
        case OSCCTRL_OSCHFCTRL_FRQSEL_24M_Val: frequency_hz = 24000000UL; break;
        default:                               frequency_hz = 0UL;         break;
    }
    return frequency_hz;
}

/*----------------------------------------------------------------------------
  Internal function for detecting Generator 1 clock frequency
 *----------------------------------------------------------------------------*/

static uint32_t GetGEN1freq(void)
{
    uint32_t gen1_freq    = 0U; /* Default value for error cases */
    uint32_t gen1_src     = GetGenericClockSource(1U);
    uint32_t gen1_div     = GetDivideSelection(1U);
    uint32_t gen1_div_val = GetDivisionValue(1U);
    uint32_t oschf_freq   = GetOSCCTRLFreq();

    /* Select clock source frequency for Generator 1 */
    switch (gen1_src)
    {
        case GCLK_SOURCE_OSCHF  : gen1_freq = oschf_freq;            break;
        case GCLK_SOURCE_OSC32K : gen1_freq = __SYSTEM_OSC32K_FREQ;  break;
        case GCLK_SOURCE_XOSC32K: gen1_freq = __SYSTEM_XOSC32K_FREQ; break;
        case GCLK_SOURCE_GCLKIN : gen1_freq = __EXTERNAL_CLOCK_FREQ; break;
        default                 : gen1_freq = 0U;                    break;
    }

    if (gen1_freq != 0U)
    {
        switch (gen1_div)
        {
            case GCLK_GENCTRL_DIVSEL_DIV1_Val:
                if (gen1_div_val != 0U)
                {
                    gen1_freq /= gen1_div_val;
                }
                break;
            case GCLK_GENCTRL_DIVSEL_DIV2_Val:
                gen1_freq /= (1UL << (gen1_div_val + 1U));
                break;
            default:
                /* No other divider modes defined. */
                break;
        }
    }

    return gen1_freq;
}

/*----------------------------------------------------------------------------
  System Core Clock update function
 *----------------------------------------------------------------------------*/
bool SystemCoreClockUpdate_PreHook(void);

#if defined(__ICCARM__)
#pragma weak SystemCoreClockUpdate_PreHook
#else
__attribute__((weak))
#endif
bool SystemCoreClockUpdate_PreHook(void)
{
    /* Default implementation: do nothing. */
    return false;
}

void SystemCoreClockUpdate(void)
{
    if (SystemCoreClockUpdate_PreHook())
    {
        return;
    }

    uint32_t src_freq     = 0U;
    uint32_t gen0_src     = GetGenericClockSource(0U);
    uint32_t gen0_div     = GetDivideSelection(0U);
    uint32_t gen0_div_val = GetDivisionValue(0U);
    uint32_t oschf_freq   = GetOSCCTRLFreq();
    uint32_t mclk_cpu_div = (uint32_t)MCLK_REGS->MCLK_CPUDIV;
    uint32_t result_freq  = 0U; /* Default value for error cases */

    /* Select clock source frequency for Generator 0 */
    switch (gen0_src)
    {
        case GCLK_SOURCE_OSCHF   : src_freq = oschf_freq;            break;
        case GCLK_SOURCE_OSC32K  : src_freq = __SYSTEM_OSC32K_FREQ;  break;
        case GCLK_SOURCE_XOSC32K : src_freq = __SYSTEM_XOSC32K_FREQ; break;
        case GCLK_SOURCE_GCLKGEN1: src_freq = GetGEN1freq();         break;
        case GCLK_SOURCE_GCLKIN  : src_freq = __EXTERNAL_CLOCK_FREQ; break;
        default                  : src_freq = 0U;                    break; /* Unsupported clock source */
    }

    /* Only proceed with valid source frequency and divider */
    if ((src_freq != 0U) && (mclk_cpu_div != 0U))
    {
        switch (gen0_div)
        {
            case GCLK_GENCTRL_DIVSEL_DIV1_Val:
                if (gen0_div_val != 0U)
                {
                    src_freq /= gen0_div_val;
                }
                break;
            case GCLK_GENCTRL_DIVSEL_DIV2_Val:
                src_freq /= (1UL << (gen0_div_val + 1U));
                break;
            default:
                src_freq = 0U; /* Invalid divisor config, will keep result == 0U */
                break;
        }

        if (src_freq != 0U)
        {
            result_freq = src_freq / mclk_cpu_div;
        }
    }

    SystemCoreClock = result_freq;
}

/*----------------------------------------------------------------------------
  System initialization function
 *----------------------------------------------------------------------------*/
void SystemInit (void)
{
    SystemCoreClock = (uint32_t)SYSTEM_CLOCK;
}
