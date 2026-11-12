#include "parking_sensor.h"
#include "hcsr04.h"        /* HC-SR04 ultrasonic driver */
#include "mqtt_task.h"     /* parking_signal_arrival() / parking_signal_passed() */
#include "cmsis_os.h"

/* ---- Erkennungs-Parameter (wie im Bringup-Test) ----------------------- */
#define CAR_THRESHOLD_CM   10.0f   /* naeher als 10 cm -> Auto da */
#define DEBOUNCE_COUNT     3       /* so viele gleiche Messungen in Folge, bevor
                                      der Zustand wechselt (Fehlmessungen filtern) */
#define SENSOR_PERIOD_MS   200U    /* ~5 Messungen pro Sekunde */

static const osThreadAttr_t s_attr = {
    .name       = "parkSensor",
    .stack_size = 2048U,
    .priority   = osPriorityNormal,
};

static void prvSensorTask(void *arg)
{
    (void)arg;

    hcsr04_init();

    int carPresent = 0;   /* aktueller, entprellter Zustand */
    int diffCount  = 0;   /* wie oft die Rohmessung dem Gegenteil zustimmt */

    for (;;) {
        float cm = hcsr04_read_cm();

        /* kein Echo (-1) zaehlt als "kein Auto" */
        int raw = (cm >= 0.0f && cm < CAR_THRESHOLD_CM) ? 1 : 0;

        if (raw != carPresent) {
            if (++diffCount >= DEBOUNCE_COUNT) {
                carPresent = raw;
                diffCount  = 0;
                if (carPresent) {
                    parking_signal_arrival();   /* Auto steht an der Schranke */
                } else {
                    parking_signal_passed();    /* Auto durchgefahren / weg */
                }
            }
        } else {
            diffCount = 0;
        }

        osDelay(SENSOR_PERIOD_MS);
    }
}

void parking_sensor_start(void)
{
    osThreadNew(prvSensorTask, NULL, &s_attr);
}
