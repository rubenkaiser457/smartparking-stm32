#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

/* ===========================================================================
 *  Gemeinsame Konfiguration fuer BEIDE Boards
 * ---------------------------------------------------------------------------
 *  Diese Datei liegt im Repo-Root unter  Shared/  und wird von Board1 UND
 *  Board2 ueber den Include-Pfad  ../../Shared  eingebunden.
 *
 *  >>> Broker-IP (Laptop-IP) NUR HIER aendern – gilt automatisch fuer beide
 *      Boards. Danach beide Boards neu bauen + flashen. <<<
 * ===========================================================================
 */

#define MQTT_BROKER_IP     "192.168.1.100"    /* <-- IP of the laptop running the broker */
#define MQTT_BROKER_PORT   1883

#endif /* MQTT_CONFIG_H */
