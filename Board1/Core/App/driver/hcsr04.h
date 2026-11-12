#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>

/*
 * Treiber fuer den HC-SR04 Ultraschall-Abstandssensor.
 *
 * Verkabelung (Nucleo-F439ZI):
 *   VCC  -> 5V
 *   TRIG -> PF13 (Arduino D7)        3,3V-Ausgang reicht
 *   ECHO -> PF12 (Arduino D8)        NUR ueber Spannungsteiler 1k/2k (5V->3,3V)!
 *   GND  -> GND
 *
 * Zeitmessung erfolgt mikrosekundengenau ueber den DWT-Cycle-Counter
 * (der 1ms-FreeRTOS-Tick waere viel zu grob).
 */

/* Einmalig aufrufen, bevor gemessen wird (konfiguriert GPIO + DWT). */
void hcsr04_init(void);

/*
 * Eine Messung ausfuehren.
 * Rueckgabe: Distanz in Zentimetern, oder -1.0f bei Timeout/keinem Echo
 * (z.B. ausserhalb der Reichweite ~2..400cm). Blockiert max. ~30ms.
 */
float hcsr04_read_cm(void);

#endif /* HCSR04_H */
