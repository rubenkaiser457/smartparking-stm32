#include "sensor_test.h"
#include "hcsr04.h"
#include "cmsis_os.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

/* USART3 ist in main.c konfiguriert (115200 8N1, auf den ST-Link-Pins). */
extern UART_HandleTypeDef huart3;

static const osThreadAttr_t s_testTaskAttr = {
    .name       = "sensorTest",
    .stack_size = 2048U,
    .priority   = osPriorityNormal,
};

static void uartPrint(const char *s)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

/* ---- Auto-Erkennung --------------------------------------------------- */
#define CAR_THRESHOLD_CM   10.0f   /* naeher als 10cm -> Auto da */
#define DEBOUNCE_COUNT     3       /* so viele gleiche Messungen in Folge
                                      noetig, bevor der Zustand wechselt */

static void prvSensorTestTask(void *arg)
{
    (void)arg;

    hcsr04_init();
    uartPrint("\r\nHC-SR04 Test gestartet (PF13=TRIG, PF12=ECHO)\r\n");

    int carPresent = 0;   /* aktueller, entprellter Zustand */
    int sameCount  = 0;   /* wie oft die Rohmessung dem Gegenteil zustimmt */

    for (;;) {
        float cm = hcsr04_read_cm();

        /* Rohmessung -> Auto da? (kein Echo zaehlt als "kein Auto") */
        int rawPresent = (cm >= 0.0f && cm < CAR_THRESHOLD_CM) ? 1 : 0;

        /* Entprellen: nur wechseln, wenn DEBOUNCE_COUNT mal in Folge anders */
        if (rawPresent != carPresent) {
            if (++sameCount >= DEBOUNCE_COUNT) {
                carPresent = rawPresent;
                sameCount  = 0;
                uartPrint(carPresent ? ">>> AUTO ERKANNT\r\n"
                                     : ">>> frei\r\n");
            }
        } else {
            sameCount = 0;
        }

        /* Zusaetzlich die Live-Distanz mitschreiben. */
        char line[64];
        if (cm < 0.0f) {
            snprintf(line, sizeof(line), "Distanz: -- (kein Echo)\r\n");
        } else {
            int whole = (int)cm;
            int frac  = (int)((cm - (float)whole) * 10.0f);
            snprintf(line, sizeof(line), "Distanz: %d.%d cm\r\n", whole, frac);
        }
        uartPrint(line);

        osDelay(300);   /* ~3 Messungen pro Sekunde */
    }
}

void sensor_test_start(void)
{
    osThreadNew(prvSensorTestTask, NULL, &s_testTaskAttr);
}
