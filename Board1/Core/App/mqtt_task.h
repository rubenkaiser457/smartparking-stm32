#ifndef MQTT_TASK_H
#define MQTT_TASK_H

/**
 * @brief MQTT-Task starten (aus freertos.c aufrufen).
 */
void mqtt_task_start(void);

/**
 * @brief Zuletzt vom Laptop gemeldete freie Parkplaetze (parking/status).
 * @return  Anzahl freier Plaetze, oder -1 wenn noch kein Status empfangen.
 *
 * Integrationspunkt fuer die Einfahrt-Logik: vor dem Oeffnen der Schranke
 * pruefen, ob der Rueckgabewert > 0 ist.
 */
int mqtt_task_get_free_spaces(void);

/**
 * @brief Vom Abstandssensor aufrufen: ein Auto steht an der Schranke.
 *
 * Setzt nur ein Flag (thread-sicher, aus beliebigem Task aufrufbar). Der
 * MQTT-Task prueft dann den freien Platz und oeffnet ggf. Schranke + Ampel.
 */
void parking_signal_arrival(void);

/**
 * @brief Vom Abstandssensor aufrufen: das Auto ist durchgefahren.
 *
 * Setzt nur ein Flag. Der MQTT-Task meldet dann parking/entry und
 * schliesst Schranke + Ampel wieder.
 */
void parking_signal_passed(void);

#endif /* MQTT_TASK_H */
