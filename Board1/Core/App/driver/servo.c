#include "servo.h"
#include "main.h"   /* zieht CMSIS-Register (RCC, GPIOD, TIM4) ueber stm32f4xx_hal.h herein */

/*
 * Timer-Mathematik:
 *   TIM4 haengt am APB1. Bei APB1-Prescaler /4 (PCLK1 = 42 MHz) laufen die
 *   Timer mit dem doppelten Takt -> 84 MHz.
 *   PSC = 83  -> 84 MHz / 84       = 1 MHz  -> 1 Tick = 1 us
 *   ARR = 19999 -> 20000 us Periode = 20 ms -> 50 Hz
 *   CCR4 = Pulsbreite in us (z. B. 1500 = 1,5 ms = 90deg)
 */
#define SERVO_TIMER_PSC     (84U - 1U)
#define SERVO_TIMER_ARR     (20000U - 1U)

#define SERVO_MIN_US        600U    /* mechanische Untergrenze (Schutz) */
#define SERVO_MAX_US        2400U   /* mechanische Obergrenze (Schutz) */

/* PD15 = Pin 15 an GPIOD, Timer-Alternativfunktion AF2 (TIM4_CH4). */
#define SERVO_GPIO_PIN      15U
#define SERVO_AF_TIM4       2U

void servo_init(void)
{
    /* --- Takte einschalten --- */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;   /* GPIOD */
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;    /* TIM4  */

    /* --- PD15 auf Alternativfunktion AF2 (TIM4_CH4) --- */
    /* MODER: 10b = Alternate Function */
    GPIOD->MODER &= ~(3U << (SERVO_GPIO_PIN * 2U));
    GPIOD->MODER |=  (2U << (SERVO_GPIO_PIN * 2U));
    /* Push-Pull (OTYPER=0) ist Default; mittlere Geschwindigkeit genuegt fuer 50 Hz */
    GPIOD->OSPEEDR |= (1U << (SERVO_GPIO_PIN * 2U));
    /* kein Pull-up/Pull-down */
    GPIOD->PUPDR &= ~(3U << (SERVO_GPIO_PIN * 2U));
    /* AFR[1] zustaendig fuer Pins 8..15; Pin 15 -> Bits 28..31 */
    GPIOD->AFR[1] &= ~(0xFU << ((SERVO_GPIO_PIN - 8U) * 4U));
    GPIOD->AFR[1] |=  (SERVO_AF_TIM4 << ((SERVO_GPIO_PIN - 8U) * 4U));

    /* --- TIM4 als 50-Hz-PWM auf Kanal 4 --- */
    TIM4->PSC = SERVO_TIMER_PSC;
    TIM4->ARR = SERVO_TIMER_ARR;
    TIM4->CCR4 = SERVO_CLOSED_US;          /* Schranke startet geschlossen */

    /* OC4 = PWM-Modus 1 (110b in OC4M) + Preload aktiv */
    TIM4->CCMR2 &= ~TIM_CCMR2_OC4M;
    TIM4->CCMR2 |= (TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1);
    TIM4->CCMR2 |= TIM_CCMR2_OC4PE;

    TIM4->CCER |= TIM_CCER_CC4E;           /* Kanal-4-Ausgang aktivieren */
    TIM4->CR1  |= TIM_CR1_ARPE;            /* ARR-Preload */
    TIM4->EGR  |= TIM_EGR_UG;              /* PSC/ARR sofort uebernehmen */
    TIM4->CR1  |= TIM_CR1_CEN;             /* Zaehler starten */
}

void servo_set_us(uint16_t us)
{
    if (us < SERVO_MIN_US) us = SERVO_MIN_US;
    if (us > SERVO_MAX_US) us = SERVO_MAX_US;
    TIM4->CCR4 = us;
}

void servo_set_angle(uint8_t deg)
{
    if (deg > 180U) deg = 180U;
    /* 0deg -> 1000us, 180deg -> 2000us */
    servo_set_us((uint16_t)(1000U + ((uint32_t)deg * 1000U) / 180U));
}

void servo_open(void)
{
    servo_set_us(SERVO_OPEN_US);
}

void servo_close(void)
{
    servo_set_us(SERVO_CLOSED_US);
}
