#ifndef PARKING_SENSOR_H
#define PARKING_SENSOR_H

/*
 * HC-SR04-Sensor-Task fuer die Ausfahrt (Board2).
 *
 * Misst zyklisch die Distanz (Treiber hcsr04.c) und erkennt entprellt, ob ein
 * Auto an der Schranke steht. Bei Zustandswechsel signalisiert er der
 * Ausfahrt-Logik im MQTT-Task:
 *   Auto erkannt     -> parking_signal_arrival()
 *   Auto wieder weg  -> parking_signal_passed()
 *
 * Laeuft als eigener FreeRTOS-Task, vom MQTT-/Netzwerk-Task entkoppelt
 * (Signale sind nur Flags). Identische Logik wie Board1, nur Ausfahrt statt
 * Einfahrt (keine Platz-Pruefung, Schranke oeffnet immer).
 */
void parking_sensor_start(void);

#endif /* PARKING_SENSOR_H */
