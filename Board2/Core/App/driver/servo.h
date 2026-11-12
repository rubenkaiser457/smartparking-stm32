#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

/*
 * Servo-Treiber (SG90) fuer die Parkhaus-Schranke.
 *
 * Erzeugt ein 50-Hz-PWM-Signal (alle 20 ms ein Puls). Die Pulsbreite legt die
 * Servo-Stellung fest:  ~1000us = 0deg (zu),  ~1500us = 90deg (auf).
 *
 * Umsetzung per direktem Registerzugriff (TIM4 CH4), da das HAL-TIM-Modul im
 * Projekt nicht eingebunden ist -- gleiche Vorgehensweise wie beim IWDG.
 *
 * Pin/Timer fest verdrahtet:
 *   TIM4_CH4  ->  PD15  ->  Arduino-Header D9  (AF2)
 *
 * Verkabelung SG90:
 *   ORANGE (Signal) -> PD15 (D9)
 *   ROT    (+5V)    -> externe 5-V-Versorgung (NICHT 3V3 des Boards!)
 *   BRAUN  (GND)    -> GND, gemeinsam mit dem STM32-GND verbinden
 */

/* Pulsbreiten in Mikrosekunden -- bei Bedarf am echten Servo nachkalibrieren. */
#define SERVO_CLOSED_US   1000U   /* Schranke zu  (~0deg)  */
#define SERVO_OPEN_US     2000U   /* Schranke auf (~90deg) -- am echten Servo kalibriert */

/** @brief TIM4/GPIO konfigurieren und PWM starten (Schranke startet zu). */
void servo_init(void);

/** @brief Schranke oeffnen (Puls = SERVO_OPEN_US). */
void servo_open(void);

/** @brief Schranke schliessen (Puls = SERVO_CLOSED_US). */
void servo_close(void);

/** @brief Pulsbreite direkt setzen (in us, geklemmt auf 600..2400). */
void servo_set_us(uint16_t us);

/** @brief Winkel 0..180 grad anfahren (linear auf 1000..2000us gemappt). */
void servo_set_angle(uint8_t deg);

#endif /* SERVO_H */
