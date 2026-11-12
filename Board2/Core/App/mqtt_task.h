#ifndef MQTT_TASK_H
#define MQTT_TASK_H

/**
 * @brief MQTT-Task starten (aus freertos.c aufrufen).
 */
void mqtt_task_start(void);

/**
 * @brief Vom Abstandssensor aufrufen: ein Auto steht an der Schranke.
 *
 * Setzt nur ein Flag (thread-sicher, aus beliebigem Task aufrufbar). Der
 * MQTT-Task oeffnet daraufhin die Schranke (Ausfahrt ist immer erlaubt).
 */
void parking_signal_arrival(void);

/**
 * @brief Vom Abstandssensor aufrufen: das Auto ist durchgefahren.
 *
 * Setzt nur ein Flag. Der MQTT-Task meldet dann parking/exit und schliesst
 * die Schranke wieder.
 */
void parking_signal_passed(void);

#endif /* MQTT_TASK_H */
