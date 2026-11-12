#include "ampel.h"
#include "main.h"   /* HAL */

/*
 * Externe LEDs auf zwei freien Arduino-Pins (Zio-Header CN10):
 *   GRUEN -> D4 = PF14
 *   ROT   -> D2 = PF15
 * (D7/D8 = PF13/PF12 sind vom HC-SR04 belegt -> Ampel auf D4/D2 verlegt.)
 *
 * Beschaltung pro LED (active-high):
 *   Pin --[ 220 Ohm ]--|>|-- GND
 *                      LED (langes Bein = Anode zum Widerstand/Pin,
 *                           kurzes Bein = Kathode an GND)
 */
#define AMPEL_GRUEN_PORT   GPIOF
#define AMPEL_GRUEN_PIN    GPIO_PIN_14   /* D4 */
#define AMPEL_ROT_PORT     GPIOF
#define AMPEL_ROT_PIN      GPIO_PIN_15   /* D2 */

void ampel_init(void)
{
    __HAL_RCC_GPIOF_CLK_ENABLE();   /* GPIOF wird sonst nirgends getaktet */

    GPIO_InitTypeDef io = {0};
    io.Mode  = GPIO_MODE_OUTPUT_PP;
    io.Pull  = GPIO_NOPULL;
    io.Speed = GPIO_SPEED_FREQ_LOW;

    io.Pin = AMPEL_GRUEN_PIN;
    HAL_GPIO_Init(AMPEL_GRUEN_PORT, &io);
    io.Pin = AMPEL_ROT_PIN;
    HAL_GPIO_Init(AMPEL_ROT_PORT, &io);

    ampel_aus();
}

void ampel_gruen(void)
{
    HAL_GPIO_WritePin(AMPEL_GRUEN_PORT, AMPEL_GRUEN_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(AMPEL_ROT_PORT,   AMPEL_ROT_PIN,   GPIO_PIN_RESET);
}

void ampel_rot(void)
{
    HAL_GPIO_WritePin(AMPEL_GRUEN_PORT, AMPEL_GRUEN_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AMPEL_ROT_PORT,   AMPEL_ROT_PIN,   GPIO_PIN_SET);
}

void ampel_aus(void)
{
    HAL_GPIO_WritePin(AMPEL_GRUEN_PORT, AMPEL_GRUEN_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AMPEL_ROT_PORT,   AMPEL_ROT_PIN,   GPIO_PIN_RESET);
}
