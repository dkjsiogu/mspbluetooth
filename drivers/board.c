/*
 * board.c
 * Board support implementation for clock setup, millisecond timing, blocking
 * delays, and the LaunchPad/Pocket Kit status LED.
 */
#include "board.h"

#include <msp430.h>

#include "board_pins.h"
#include "platform_config.h"

/* g_millis: 1 ms uptime counter incremented by TIMER0_A0 ISR. */
static volatile uint32_t g_millis = 0;

/* pmm_set_vcore_up: raises PMM core voltage by one level; level is target VCore. */
static uint8_t pmm_set_vcore_up(uint8_t level)
{
    uint16_t pmmrie_backup;
    uint16_t svsmhctl_backup;
    uint16_t svsmlctl_backup;

    PMMCTL0_H = PMMPW_H;

    pmmrie_backup = PMMRIE;
    PMMRIE &= (uint16_t)~(SVMHVLRPE | SVSHPE | SVMLVLRPE | SVSLPE |
                          SVMHVLRIE | SVMHIE | SVSMHDLYIE |
                          SVMLVLRIE | SVMLIE | SVSMLDLYIE);
    svsmhctl_backup = SVSMHCTL;
    svsmlctl_backup = SVSMLCTL;

    PMMIFG = 0;
    SVSMHCTL = SVMHE | SVSHE | (uint16_t)(SVSMHRRL0 * level);
    while ((PMMIFG & SVSMHDLYIFG) == 0u) {
    }
    PMMIFG &= (uint16_t)~SVSMHDLYIFG;

    if ((PMMIFG & SVMHIFG) != 0u) {
        PMMIFG &= (uint16_t)~SVSMHDLYIFG;
        SVSMHCTL = svsmhctl_backup;
        while ((PMMIFG & SVSMHDLYIFG) == 0u) {
        }
        PMMIFG &= (uint16_t)~(SVMHVLRIFG | SVMHIFG | SVSMHDLYIFG |
                              SVMLVLRIFG | SVMLIFG | SVSMLDLYIFG);
        PMMRIE = pmmrie_backup;
        PMMCTL0_H = 0;
        return 0;
    }

    SVSMHCTL |= (uint16_t)(SVSHRVL0 * level);
    while ((PMMIFG & SVSMHDLYIFG) == 0u) {
    }
    PMMIFG &= (uint16_t)~SVSMHDLYIFG;

    PMMCTL0_L = (uint8_t)(PMMCOREV0 * level);
    SVSMLCTL = SVMLE | (uint16_t)(SVSMLRRL0 * level) |
               SVSLE | (uint16_t)(SVSLRVL0 * level);
    while ((PMMIFG & SVSMLDLYIFG) == 0u) {
    }
    PMMIFG &= (uint16_t)~SVSMLDLYIFG;

    SVSMLCTL &= (uint16_t)(SVSLRVL0 | SVSLRVL1 | SVSMLRRL0 |
                           SVSMLRRL1 | SVSMLRRL2);
    svsmlctl_backup &= (uint16_t)~(SVSLRVL0 | SVSLRVL1 | SVSMLRRL0 |
                                   SVSMLRRL1 | SVSMLRRL2);
    SVSMLCTL |= svsmlctl_backup;

    SVSMHCTL &= (uint16_t)(SVSHRVL0 | SVSHRVL1 | SVSMHRRL0 |
                           SVSMHRRL1 | SVSMHRRL2);
    svsmhctl_backup &= (uint16_t)~(SVSHRVL0 | SVSHRVL1 | SVSMHRRL0 |
                                   SVSMHRRL1 | SVSMHRRL2);
    SVSMHCTL |= svsmhctl_backup;

    while (((PMMIFG & SVSMLDLYIFG) == 0u) &&
           ((PMMIFG & SVSMHDLYIFG) == 0u)) {
    }
    PMMIFG &= (uint16_t)~(SVMHVLRIFG | SVMHIFG | SVSMHDLYIFG |
                          SVMLVLRIFG | SVMLIFG | SVSMLDLYIFG);

    PMMRIE = pmmrie_backup;
    PMMCTL0_H = 0;
    return 1;
}

/* pmm_set_vcore: walks VCore upward until target_level is reached. */
static void pmm_set_vcore(uint8_t target_level)
{
    uint8_t level;
    uint8_t current_level;

    target_level &= PMMCOREV_3;
    current_level = (uint8_t)(PMMCTL0 & PMMCOREV_3);

    for (level = (uint8_t)(current_level + 1u); level <= target_level; level++) {
        while (pmm_set_vcore_up(level) == 0u) {
        }
    }
}

void board_clock_init(void)
{
    pmm_set_vcore(PMMCOREV_2);

    UCSCTL3 = SELREF_2;
    UCSCTL4 = SELA_2 | SELS_4 | SELM_4;

    __bis_SR_register(SCG0);
    UCSCTL0 = 0;
    UCSCTL1 = DCORSEL_5;
    UCSCTL2 = FLLD_0 | 487u;
    UCSCTL5 = 0;
    __bic_SR_register(SCG0);

    board_delay_ms(250u);
}

void board_gpio_init(void)
{
    STATUS_LED_DIR |= STATUS_LED_BIT;
    STATUS_LED_OUT &= (uint8_t)~STATUS_LED_BIT;
}

void board_tick_init(void)
{
    TA0CCR0 = (uint16_t)((SMCLK_HZ / BOARD_TICK_HZ) - 1u);
    TA0CCTL0 = CCIE;
    TA0CTL = TASSEL_2 | MC_1 | TACLR;
}

void board_delay_us(uint16_t us)
{
    while (us > 0u) {
        __delay_cycles(MCLK_HZ / 1000000UL);
        us--;
    }
}

void board_delay_ms(uint16_t ms)
{
    while (ms > 0u) {
        board_delay_us(1000u);
        ms--;
    }
}

uint32_t board_millis(void)
{
    uint16_t gie;
    uint32_t now;

    gie = (uint16_t)(__get_SR_register() & GIE);
    __disable_interrupt();
    now = g_millis;
    if (gie != 0u) {
        __enable_interrupt();
    }

    return now;
}

uint8_t board_elapsed_ms(uint32_t *stamp_ms, uint16_t interval_ms)
{
    uint32_t now;

    now = board_millis();
    if ((uint32_t)(now - *stamp_ms) >= (uint32_t)interval_ms) {
        *stamp_ms = now;
        return 1u;
    }

    return 0u;
}

void board_status_led_set(uint8_t on)
{
    if (on != 0u) {
        STATUS_LED_OUT |= STATUS_LED_BIT;
    } else {
        STATUS_LED_OUT &= (uint8_t)~STATUS_LED_BIT;
    }
}

void board_status_led_toggle(void)
{
    STATUS_LED_OUT ^= STATUS_LED_BIT;
}

void board_idle(void)
{
    __no_operation();
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
    g_millis++;
    __bic_SR_register_on_exit(LPM0_bits);
}
