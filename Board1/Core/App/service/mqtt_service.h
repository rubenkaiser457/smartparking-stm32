#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#include "mqtt_transport.h"
#include "core_mqtt.h"
#include <stddef.h>

/* Callback-Typ für eingehende Nachrichten (Topic + Payload, null-terminiert) */
typedef void (*MqttMessageCb_t)(const char *pTopic, const char *pPayload, size_t payloadLen);

typedef struct {
    MQTTContext_t        mqttCtx;
    NetworkContext_t     netCtx;
    TransportInterface_t transport;
    MQTTPubAckInfo_t     outgoingRecords[10];
    MQTTPubAckInfo_t     incomingRecords[10];
    uint8_t              networkBuf[1024];
    MqttMessageCb_t      onMessage;  /* wird bei eingehenden Publishes aufgerufen */
} MqttServiceCtx_t;

MQTTStatus_t mqtt_service_connect(MqttServiceCtx_t *pSvc,
                                  const char *pBrokerIp,
                                  const char *pClientId,
                                  MqttMessageCb_t onMessage);

MQTTStatus_t mqtt_service_subscribe(MqttServiceCtx_t *pSvc,
                                    const char *pTopic);

MQTTStatus_t mqtt_service_publish(MqttServiceCtx_t *pSvc,
                                  const char *pTopic,
                                  const char *pPayload);

MQTTStatus_t mqtt_service_process(MqttServiceCtx_t *pSvc);

void mqtt_service_disconnect(MqttServiceCtx_t *pSvc);

#endif /* MQTT_SERVICE_H */
