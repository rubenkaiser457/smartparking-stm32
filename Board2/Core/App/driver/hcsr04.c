#include "hcsr04.h"
#include "main.h"   /* zieht stm32f4xx_hal.h + CMSIS (DWT, CoreDebug) */

/* ---- Pin-Belegung ----------------------------------------------------- */
#define HCSR04_TRIG_PORT   GPIOF
#define HCSR04_TRIG_PIN    GPIO_PIN_13   /* Arduino D7 */
#define HCSR04_ECHO_PORT   GPIOF
#define HCSR04_ECHO_PIN    GPIO_PIN_12   /* Arduino D8, via Spannungsteiler */

/* Schallgeschwindigkeit: Hin- und Rueckweg -> 58us pro cm. */
#define US_PER_CM          58.0f
/* Max. Wartezeit auf eine Echo-Flanke (~400cm Reichweite ~ 23ms). */
#define ECHO_TIMEOUT_US    30000U

/* ---- DWT-Mikrosekunden-Timing ----------------------------------------- */
static uint32_t s_cyclesPerUs = 1U;

static inline uint32_t dwt_us(void)
{
    return DWT->CYCCNT / s_cyclesPerUs;
}

static void dwt_delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * s_cyclesPerUs;
    while ((DWT->CYCCNT - start) < ticks) { /* warten */ }
}

/* ---- Init ------------------------------------------------------------- */
void hcsr04_init(void)
{
    /* DWT-Cycle-Counter aktivieren (fuer us-genaues Timing). */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
    s_cyclesPerUs = SystemCoreClock / 1000000U;
    if (s_cyclesPerUs == 0U) s_cyclesPerUs = 1U;

    /* GPIOF-Takt einschalten. */
    __HAL_RCC_GPIOF_CLK_ENABLE();

    /* TRIG = Push-Pull-Ausgang. */
    GPIO_InitTypeDef io = { 0 };
    io.Pin   = HCSR04_TRIG_PIN;
    io.Mode  = GPIO_MODE_OUTPUT_PP;
    io.Pull  = GPIO_NOPULL;
    io.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HCSR04_TRIG_PORT, &io);
    HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_RESET);

    /* ECHO = Eingang. */
    io.Pin  = HCSR04_ECHO_PIN;
    io.Mode = GPIO_MODE_INPUT;
    io.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(HCSR04_ECHO_PORT, &io);
}

/* ---- Messung ---------------------------------------------------------- */
float hcsr04_read_cm(void)
{
    /* 10us-Trigger-Impuls senden (davor sauber Low). */
    HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_RESET);
    dwt_delay_us(3);
    HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_SET);
    dwt_delay_us(10);
    HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_RESET);

    /* Auf steigende Echo-Flanke warten (Sensor beginnt Messung). */
    uint32_t t0 = dwt_us();
    while (HAL_GPIO_ReadPin(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN) == GPIO_PIN_RESET) {
        if ((dwt_us() - t0) > ECHO_TIMEOUT_US) return -1.0f;
    }

    /* Echo-Pulsbreite messen (High-Dauer = Laufzeit). */
    uint32_t start = dwt_us();
    while (HAL_GPIO_ReadPin(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN) == GPIO_PIN_SET) {
        if ((dwt_us() - start) > ECHO_TIMEOUT_US) return -1.0f;
    }
    uint32_t pulseUs = dwt_us() - start;

    return (float)pulseUs / US_PER_CM;
}
