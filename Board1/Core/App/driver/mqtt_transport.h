#ifndef MQTT_TRANSPORT_H
#define MQTT_TRANSPORT_H

#include <stdint.h>
#include <stddef.h>
#include "lwip/api.h"

/* Komplette Definition von NetworkContext_t — muss VOR transport_interface.h sichtbar sein */
struct NetworkContext {
    struct netconn *conn;
    struct netbuf  *rxBuf;     /* noch nicht vollständig gelesener Empfangspuffer */
    uint16_t        rxOffset;  /* bereits aus rxBuf gelesene Bytes */
};
typedef struct NetworkContext NetworkContext_t;

#include "transport_interface.h"

/**
 * @brief TCP-Verbindung zum MQTT-Broker aufbauen.
 *
 * @param pCtx       Zeiger auf einen vom Aufrufer verwalteten NetworkContext.
 * @param pBrokerIp  Broker-IP als String, z.B. "192.168.1.100".
 * @param port       TCP-Port (Standard: 1883).
 * @return  0 bei Erfolg, -1 bei Fehler.
 */
int mqtt_transport_connect(NetworkContext_t *pCtx, const char *pBrokerIp, uint16_t port);

/**
 * @brief TCP-Verbindung schließen.
 */
void mqtt_transport_disconnect(NetworkContext_t *pCtx);

/* Callbacks für TransportInterface_t */
int32_t mqtt_transport_send(NetworkContext_t *pCtx, const void *pBuf, size_t len);
int32_t mqtt_transport_recv(NetworkContext_t *pCtx, void *pBuf, size_t len);

#endif /* MQTT_TRANSPORT_H */
