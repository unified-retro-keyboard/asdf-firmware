// -*- mode: C; tab-width: 4 ; indent-tabs-mode: nil -*-
/**
 * @file asdf_arch_atmega2560.h
 *
 * Contains architecture-specific definitions for the atmega 2560
 *
 * Part of the Unified Keyboard Project ASDF keyboard firmware.
 *
 * @copyright Copyright 2019 David F. MIT License; see LICENSE.
 */
// SPDX-License-Identifier: MIT

#if !defined(ASDF_ARCH_H)
#define ASDF_ARCH_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>

#include "asdf.h"
#include "asdf_platform.h"

// ASDF keyboard definitions:

#define F_CPU 16000000L

#define ASDF_STROBE_LENGTH_US 10 // strobe length in microseconds
#define ASDF_KEYBOARD_ROW_SETTLING_TIME_US 4 // time for keyboard capacitance to
                                             // settle before sampling

// Timer 0 definitions
// Define fields for register A, B, interrupt mask as 8-bit masks, and
// as masks offset into a combined config word
#define TMR0A_POS 0u
#define TMR0B_POS 8u
#define TMR0IMSK_POS 16u
#define TMR0A (1UL << TMR0A_POS)
#define TMR0B (1UL << TMR0B_POS)
#define TMR0IMSK (1UL << TMR0IMSK_POS)

#define TIMER0_COM_A_DISCONNECTED 0
#define TIMER0_COM_B_DISCONNECTED 0
#define TIMER0_WFM_CTC (TMR0A << WGM01)
#define TIMER0_DIV64 ((TMR0B << CS01) | (TMR0B << CS00))
#define TIMER0_INT_ON_COMA (TMR0IMSK << OCIE0A)
#define TIMER0_INT_ON_COMB (TMR0IMSK << OCIE0B)
#define TIMER0_INT_OV_ENABLE (TMR0IMSK << TOIE0)

// Macros for 16-bit timers 1, 3, 4, 5. Datasheet section 17, p. 133
//
// Define fields for Registers A, B, C, INT Mask registers as 8-bit
// masks, and as masks offset into a 32-bit combined config word.
//
// Macro definitions for individual registers are named TMRXA_*,
// TMRXB_*, and TMRXC_*.
//
// Macros for the one-step combined timer config functions are named
// TMRX_*.
//
//
// Examples:
//   // Use TMRXB_* definition to set TCCR4B register
//   TCCR4B |= TMRXB_DIV1;
//
//   // Use TMRX_* definitions to configure timer with config function
//   processor_timer4_config(TMRX_WFM_CTC | TMRX_INT_ON_CMPA);
//
#define TMRXA_POS 0u
#define TMRXB_POS 8u
#define TMRXC_POS 16u
#define TMRXIMSK_POS 24u
#define TMRXA (1UL << TMRXA_POS)
#define TMRXB (1UL << TMRXB_POS)
#define TMRXC (1UL << TMRXC_POS)
#define TMRXIMSK (1UL << TMRXIMSK_POS)

// 16-bit timer reg A - Datasheet 17.11.1, p. 154
#define TMRXA_CMPA_CLR_MATCH_SET_BOTTOM (1 << COM1A1)
#define TMRXA_CMPB_CLR_MATCH_SET_BOTTOM (1 << COM1B1)
#define TMRXA_CMPC_CLR_MATCH_SET_BOTTOM (1 << COM1C1)
#define TMRX_CMPA_CLR_MATCH_SET_BOTTOM (TMRXA << COM1A1)
#define TMRX_CMPB_CLR_MATCH_SET_BOTTOM (TMRXA << COM1B1)
#define TMRX_CMPC_CLR_MATCH_SET_BOTTOM (TMRXA << COM1C1)

// 16-bit timer reg B - Datasheet 17.11.6, p. 156

#define TMRXB_IN_CAP_POS 0x40L
#define TMRXB_IN_CAP_NEG 0L
#define TMRXB_IN_CAP_NOISE_CANCEL 0x80L

// 16-bit timer reg C -- see datasheet, 17.11.9, p. 157
#define TMRXC_FOCA 0x80L
#define TMRXC_FOCB 0x40L
#define TMRXC_FOCC 0x20L

// 16-bit timer int mask -- see datasheet 17.11.33, p. 161.
#define TMRXIM_INT_CMP_MATCH_A (1L << OCIE1A)
#define TMRXIM_INT_CMP_MATCH_B (1L << OCIE1B)
#define TMRXIM_INT_CMP_MATCH_C (1L << OCIE1C)

// 16-bit timer all registers:
#define TMRX_CMPA_DISCONNECTED 0L
#define TMRX_CMPB_DISCONNECTED 0L
#define TMRX_CMPC_DISCONNECTED 0L
#define TMRX_INT_ON_CMPA (TMRXIM_INT_CMP_MATCH_A << TMRXIMSK_POS)

// 16-bit timer clock modes - see Datasheet table 17-6,  p. 157
#define TMRXB_OFF 0
#define TMRX_OFF 0
#define TMRXB_DIV1 (0x01L << CS10)
#define TMRXB_DIV8 (0x02L << CS10)
#define TMRXB_DIV64 (0x03L << CS10)
#define TMRXB_DIV256 (0x04L << CS10)
#define TMRXB_DIV1024 (0x05L << CS10)
#define TMRXB_EXT_FALLING_EDGE (0x06L << CS10)
#define TMRXB_EXT_RISING_EDGE (0x07L << CS10)
#define TMRXB_CLK_MASK 0x07L
#define TMRX_DIV1 (TMRXB_DIV1 << TMRXB_POS)
#define TMRX_DIV8 (TMRXB_DIV8 << TMRXB_POS)
#define TMRX_DIV64 (TMRXB_DIV64 << TMRXB_POS)
#define TMRX_DIV256 (TMRXB_DIV256 << TMRXB_POS)
#define TMRX_DIV1024 (TMRXB_DIV1024 << TMRXB_POS)
#define TMRX_EXT_FALLING_EDGE (TMRXB_EXT_FALLING_EDGE << TMRXB_POS)
#define TMRX_EXT_RISING_EDGE (TMRXB_EXT_RISING_EDGE << TMRXB_POS)

#define TMRXB_EDGE_SEL_POSITIVE (1 << ICES1)
#define TMRXB_EDGE_SEL_NEGATIVE 0L
#define TMRX_EDGE_SEL_POSITIVE (TMRXB << ICES1)
#define TMRX_EDGE_SEL_NEGATIVE 0L

// 16-bit waveform modes (across reg A and B) Datasheet Table 17.2, p 145
#define TMRX_WFM_NORMAL 0L
#define TMRX_WFM_PWM_PC8 (TMRXA << WGM10)                        // PWM Phase Correct 8-bit
#define TMRX_WFM_PWM_PC9 (TMRXA << WGM11)                        // PWM Phase COrrect 9-bit
#define TMRX_WFM_PWM_PC10 ((TMRXA << WGM11) | (TMRXA << WGM10))  // PWM Phase Correct 10-bit
#define TMRX_WFM_CTC (TMRXB << WGM12)                            // CTC
#define TMRX_WFM_PWM_FAST8 ((TMRXB << WGM12) | (TMRXA << WGM10)) // PWM Fast 8-bit
#define TMRX_WFM_PWM_FAST9 ((TMRXB << WGM12) | (TMRXA << WGM11)) // PWM Fast 9-bit
#define TMRX_WFM_PWM_FAST10                                                                        \
  ((TMRXB << WGM12) | (TMRXA << WGM11) | (TMRXA << WGM10)) // PWM Fast 10-bit
#define TMRX_WFM_PWM_PFC_ICR (TMRXB << WGM13)              // PWM Phase and Freq Correct, TpOP=ICR
#define TMRX_WFM_PWM_PFC_OCRA                                                                      \
  ((TMRXB << WGM13) | (TMRXA << WGM10)) // PWM Phase and Freq Correct, TOP = OCRA
#define TMRX_WFM_PWM_PC_ICR ((TMRXB << WGM13) | (TMRXA << WGM11)) // PWM PhaseCorrect, TOP = ICR
#define TMRX_WFM_PWM_PC_OCRA                                                                       \
  ((TMRXB << WGM13) | (TMRXA << WGM11) | (TMRXA << WGM12))     // PWM PhaseCorrect, TOP=OCRA
#define TMRX_WFM_CTC_ICR ((TMRXB << WGM13) | (TMRXB << WGM12)) // CTC, TOP = ICR
#define TMRX_WFM_PWM_FAST_ICR                                                                      \
  ((TMRXB << WGM13) | (TMRXB << WGM12) | (TMRXA << WGM11)) // PWM Fast, TOP = ICR
#define TMRX_WFM_PWM_FAST_OCRA                                                                     \
  ((TMRXB << WGM13) | (TMRXB << WGM12) | (TMRXA << WGM11)                                          \
   | (TMRXA << WGM10)) // PWM Fast, TOP = OCRA

// USART configuration (Datasheet section 22, p. 200)
//
// Macro definitions for individual registers are named USARTA_*,
// USARTB_*, and USARTC_*.
//
// Macros for the one-step combined USART config functions are named
// USART_*.
//
//
// Examples:
//   // Use USARTB_* definition to set UCSR1B register
//     UCSR1B |= USARTB_DATA_REG_EMPTY_INT_EN; // enable interrupt on tx reg empty
//
//   // Use USART_* definitions to configure usart with config function
//   processor_usart3_config(USART_SIZE_8 | USART_PARITY_NONE | USART_STOP_1 | USART_DATA_TXEN |
//                           USART_DATA_RXEN | USART_RX_COMPLETE_INT_EN);
//
#define USARTA_POS 0
#define USARTB_POS 8
#define USARTC_POS 16
#define USARTA (1L << USARTA_POS)
#define USARTB (1L << USARTB_POS)
#define USARTC (1L << USARTC_POS)

// USART Register A, Datasheet 22.10.2, p. 219

#define USARTA_DATA_REG_EMPTY (1 << UDR0)
#define USARTA_FRAME_ERROR (1 << FE0)
#define USARTA_DATA_OVERRUN (1 << DOR0)
#define USARTA_PARITY_ERROR (1 << UPE0)
#define USARTA_DOUBLE_SPEED (1 << U2X0)
#define USARTA_MULTI_PROCESSOR (1 << MPCM0)
#define USART_DATA_REG_EMPTY (USARTA << UDR0)
#define USART_FRAME_ERROR (USARTA << FE0)
#define USART_DATA_OVERRUN (USARTA << DOR0)
#define USART_PARITY_ERROR (USARTA << UPE0)
#define USART_DOUBLE_SPEED (USARTA << U2X0)
#define USART_MULTI_PROCESSOR (USARTA << MPCM0)

// USART Register B, Datasheet 22.10.3, p. 220

#define USARTB_RX_COMPLETE_INT_EN (1 << RXCIE0)
#define USARTB_TX_COMPLETE_INT_EN (1 << TXCIE0)
#define USARTB_DATA_REG_EMPTY_INT_EN (1 << UDRIE0)
#define USARTB_DATA_RXEN (1 << RXEN0)
#define USARTB_DATA_TXEN (1 << TXEN0)
#define USARTB_SIZE_9 (1 << UCSZ02)
#define USARTB_RX_BIT_8 (1 << RXB80)
#define USARTB_TX_BIT_8 (1 << TXB80)
#define USART_RX_COMPLETE_INT_EN (USARTB << RXCIE0)
#define USART_TX_COMPLETE_INT_EN (USARTB << TXCIE0)
#define USART_DATA_REG_EMPTY_INT_EN (USARTB << UDRIE0)
#define USART_DATA_RXEN (USARTB << RXEN0)
#define USART_DATA_TXEN (USARTB << TXEN0)
#define USART_RX_BIT_8 (USARTB << RXB80)
#define USART_TX_BIT_8 (USARTB << TXB80)

// USART Register C, Datasheet 22.10.4, p.221
#define USARTC_MODE_ASYNC_USART (0x00L << UMSEL00)
#define USARTC_MODE_SYNC_USART (0x01L << UMSEL00)
#define USARTC_MODE_MSPIM (0x03L << UMSEL00)
#define USART_MODE_ASYNC_USART (USARTC_MODE_ASYNC_USART << USARTC_POS)
#define USART_MODE_SYNC_USART (USARTC_MODE_SYNC_USART << USARTC_POS)
#define USART_MODE_MSPIM (USARTC_MODE_MSPIM << USARTC_POS)

#define USARTC_PARITY_NONE 0
#define USARTC_PARITY_EVEN (0x01L << UPM00)
#define USARTC_PARITY_ODD (0x03 << UPM00)
#define USART_PARITY_NONE 0
#define USART_PARITY_EVEN (USARTC_PARITY_EVEN << USARTC_POS)
#define USART_PARITY_ODD (USARTC_PARITY_ODD << USARTC_POS)

#define USARTC_STOP_1 0
#define USARTC_STOP_2 (1 << USBS0)
#define USART_STOP_1 0
#define USART_STOP_2 (USARTC << USBS0)

#define USARTC_SIZE_5 0
#define USARTC_SIZE_6 (0x01L << UCSZ00)
#define USARTC_SIZE_7 (0x02L << UCSZ00)
#define USARTC_SIZE_8 (0x03L << UCSZ00)
#define USARTC_SIZE_9 USARTC_SIZE_8
#define USART_SIZE_5 0
#define USART_SIZE_6 (USARTC_SIZE_6 << USARTC_POS)
#define USART_SIZE_7 (USARTC_SIZE_7 << USARTC_POS)
#define USART_SIZE_8 (USARTC_SIZE_8 << USARTC_POS)
#define USART_SIZE_9 ((USARTB_SIZE_9 << USARTB_POS) | (USARTC_SIZE_9 << USARTC_POS))

#define USARTC_CLK_TX_RISING 0
#define USART_CLK_TX_RISING 0
#define USARTC_CLK_TX_FALLING (1 << UCPOL0)
#define USART_CLK_TX_FALLING (USARTC << UCPOL0)



// I/O port definitions:

#define PIN_INPUT 0
#define PIN_OUTPUT 1
#define ALL_INPUTS 0x00u
#define ALL_OUTPUTS 0xFFu
#define ALL_PULLUPS 0xff

#define ASDF_HIROW_PORT PORTA
#define ASDF_HIROW_DDR DDRA
#define ASDF_HIROW_PIN PINA

#define ASDF_LOROW_PORT PORTJ
#define ASDF_LOROW_DDR DDRJ
#define ASDF_LOROW_PIN PINJ

#define ASDF_COLUMNS_PORT PORTC
#define ASDF_COLUMNS_PIN PINC
#define ASDF_COLUMNS_DDR DDRC

#define ASDF_ASCII_PORT PORTH
#define ASDF_ASCII_DDR DDRH

#define ASDF_LED1_PORT PORTD
#define ASDF_LED1_DDR DDRD
#define ASDF_LED1_BIT 5

#define ASDF_LED2_PORT PORTD
#define ASDF_LED2_DDR DDRD
#define ASDF_LED2_BIT 6

#define ASDF_LED3_PORT PORTD
#define ASDF_LED3_DDR DDRD
#define ASDF_LED3_BIT 7

#define ASDF_OUT1_PORT PORTB
#define ASDF_OUT1_PIN PINB
#define ASDF_OUT1_DDR DDRB
#define ASDF_OUT1_BIT 5

#define ASDF_OUT2_PORT PORTB
#define ASDF_OUT2_PIN PINB
#define ASDF_OUT2_DDR DDRB
#define ASDF_OUT2_BIT 6

#define ASDF_OUT3_PORT PORTB
#define ASDF_OUT3_PIN PINB
#define ASDF_OUT3_DDR DDRB
#define ASDF_OUT3_BIT 7

#define ASDF_STROBE_PORT PORTB
#define ASDF_STROBE_PIN PINB
#define ASDF_STROBE_DDR DDRB
#define ASDF_STROBE_BIT 4

#define ASDF_ARCH_DIP_SWITCH_ROW 8

#define FUSE_INTERNAL_8MHZ_OSC_4MS (FUSE_CKSEL1 | FUSE_SUT0)
#define FUSE_INTERNAL_8MHZ_OSC_65MS (FUSE_CKSEL1 | FUSE_SUT1)
#define FUSE_XTAL_16MHZ_4MS (FUSE_CKSEL2 | FUSE_CKSEL1 | CKSEL0 | FUSE_SUT1)
#define FUSE_XTAL_16MHZ_65MS (FUSE_CKSEL2 | FUSE_CKSEL1 | CKSEL0 | FUSE_SUT1 | FUSE_SUT0)
#define FLASH PROGMEM

// not implemented with do-while(0) because this is a function call that returns
// a value, and parameters are expanded inside the parameter list, so this will
// be valid when substituting for function-like syntax.
#define FLASH_READ(a) pgm_read_byte((a))
#define FLASH_READ_PTR(a) pgm_read_ptr((a))
#define FLASH_MEMCPY(dst, src, n) memcpy_P((dst), (src), (n))
#define FLASH_READ_MATRIX_ELEMENT(matrix, row, col) pgm_read_byte(&((matrix)[(row)][(col)]))

// Places a string literal in flash; read it back with FLASH_READ.
#define FLASH_STRING(s) PSTR(s)

// Timer 0 TOP for a 1 ms tick: (16000000 / 64 prescale) / 1000 Hz - 1 = 249
#define TICK_COUNT 249

// DIP switch is on row 8
#define ASDF_ARCH_DIPSWITCH_ROW 8


// State of one keyboard's hardware. The embedded platform passes this state to
// each of its operations. The board layer (main.c) owns the state and the tick
// interrupt vector (ASDF_ARCH_TICK_ISR), which calls asdf_arch_count_tick().
typedef struct {
  asdf_platform_t platform; // the keyboard's platform; its user pointer is this state
  volatile uint8_t ticks;   // ticks counted by the interrupt, not yet collected
  uint8_t data_polarity;    // XORed with each code sent
} asdf_arch_t;

// The tick interrupt: Timer 0 compare match, every 1 ms. The application
// defines the interrupt handler with this macro.
#define ASDF_ARCH_TICK_ISR ISR(TIMER0_COMPA_vect)

/**
 * Counts one elapsed 1 ms tick.
 *
 * The count saturates at 255, so a long stall cannot wrap it. Call only from
 * the tick interrupt. Increments the tick count in arch.
 *
 * @param arch  Hardware state of the keyboard whose tick is counted.
 */
static inline void asdf_arch_count_tick(asdf_arch_t *arch)
{
  if (arch->ticks < 0xFFu) {
    arch->ticks++;
  }
}

/**
 * Collects the ticks counted since the last call.
 *
 * Reads and clears the tick count as one step with respect to the tick
 * interrupt, so no tick is lost. Clears the tick count in arch.
 *
 * @param arch  Hardware state of the keyboard.
 * @return The number of 1 ms ticks since the last call, saturating at 255.
 */
uint8_t asdf_arch_tick(asdf_arch_t *arch);

/**
 * Sets up the keyboard hardware and the platform embedded in arch.
 *
 * Call once, before the keyboard runs. Sets the system clock and the 1 ms tick
 * timer, the ASCII output port with the default data and strobe polarity, the
 * LED outputs, the row outputs and the column inputs; fills in the platform
 * operations, clears the tick count and enables interrupts, so the tick
 * interrupt starts running.
 *
 * @param arch  Hardware state to initialize.
 */
void asdf_arch_init(asdf_arch_t *arch);

#endif /* !defined (ASDF_ARCH_H) */

//-------|---------|---------+---------+---------+---------+---------+---------+
// Above line is 80 columns, and should display completely in the editor.
