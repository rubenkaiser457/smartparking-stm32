#include "seg7.h"
#include "main.h"   /* HAL: GPIO + RCC-Clock-Makros */

/* ---- Pin-Tabelle: Segmente a..g (Index 0..6) -------------------------- */
/* Reihenfolge MUSS a,b,c,d,e,f,g sein -- passend zu den Bitmustern unten.
   Common Cathode: Segment leuchtet, wenn der Pin HIGH ist. */
typedef struct { GPIO_TypeDef *port; uint16_t pin; } Seg7Pin;

static const Seg7Pin SEGS[7] = {
    { GPIOE, GPIO_PIN_11 },  /* a  -> D5  */
    { GPIOE, GPIO_PIN_9  },  /* b  -> D6  */
    { GPIOE, GPIO_PIN_13 },  /* c  -> D3  */
    { GPIOD, GPIO_PIN_14 },  /* d  -> D10 */
    { GPIOG, GPIO_PIN_9  },  /* e  -> D0  */
    { GPIOG, GPIO_PIN_14 },  /* f  -> D1  */
    { GPIOA, GPIO_PIN_5  },  /* g  -> D13 */
};

/* Bitmuster 0-9: Bit0=a, Bit1=b, ... Bit6=g (gleiche Kodierung wie ueblich). */
static const uint8_t DIGITS[10] = {
    0x3F, /* 0 = a b c d e f   */
    0x06, /* 1 = b c           */
    0x5B, /* 2 = a b d e g     */
    0x4F, /* 3 = a b c d g     */
    0x66, /* 4 = b c f g       */
    0x6D, /* 5 = a c d f g     */
    0x7D, /* 6 = a c d e f g   */
    0x07, /* 7 = a b c         */
    0x7F, /* 8 = alle          */
    0x6F, /* 9 = a b c d f g   */
};
#define SEG_DASH   0x40U   /* nur Segment g = "-" */

/* ---- ein Bitmuster auf die 7 Pins schreiben --------------------------- */
static void seg7_write_mask(uint8_t mask)
{
    for (int i = 0; i < 7; i++) {
        GPIO_PinState st = (mask & (1U << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET;
        HAL_GPIO_WritePin(SEGS[i].port, SEGS[i].pin, st);
    }
}

/* ---- Oeffentliche API ------------------------------------------------- */
void seg7_init(void)
{
    /* Takte fuer alle benutzten GPIO-Ports einschalten. */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    GPIO_InitTypeDef io = {0};
    io.Mode  = GPIO_MODE_OUTPUT_PP;
    io.Pull  = GPIO_NOPULL;
    io.Speed = GPIO_SPEED_FREQ_LOW;

    for (int i = 0; i < 7; i++) {
        io.Pin = SEGS[i].pin;
        HAL_GPIO_Init(SEGS[i].port, &io);
    }

    seg7_show_digit(-1);   /* "-" bis erster parking/status kommt */
}

void seg7_show_digit(int value)
{
    if (value >= 0 && value <= 9) {
        seg7_write_mask(DIGITS[value]);
    } else {
        seg7_write_mask(SEG_DASH);   /* unbekannt / ausserhalb 0..9 */
    }
}

void seg7_clear(void)
{
    seg7_write_mask(0x00U);
}
