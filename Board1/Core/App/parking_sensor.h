#ifndef PARKING_SENSOR_H
#define PARKING_SENSOR_H

/*
 * HC-SR04-Sensor-Task fuer die Einfahrt (Board1).
 *
 * Misst zyklisch die Distanz (Treiber hcsr04.c) und erkennt entprellt, ob ein
 * Auto an der Schranke steht. Bei Zustandswechsel signalisiert er der
 * Einfahrt-Logik im MQTT-Task:
 *   Auto erkannt     -> parking_signal_arrival()
 *   Auto wieder weg  -> parking_signal_passed()
 *
 * Ersetzt damit den USER-Button als Trigger. Laeuft als eigener FreeRTOS-Task,
 * vom MQTT-/Netzwerk-Task entkoppelt (Signale sind nur Flags).
 */
void parking_sensor_start(void);

#endif /* PARKING_SENSOR_H */
