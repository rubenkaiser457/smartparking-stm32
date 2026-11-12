#include "mqtt_service.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#define MQTT_KEEP_ALIVE_SEC       60U
#define MQTT_CONNACK_TIMEOUT_MS   2000U
#define MQTT_PROCESS_TIMEOUT_MS   100U

static uint32_t prvGetTimeMs(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

static bool prvEventCallback(MQTTContext_t *pCtx,
                             MQTTPacketInfo_t *pPacketInfo,
                             MQTTDeserializedInfo_t *pDeserializedInfo,
                             MQTTSuccessFailReasonCode_t *pReasonCode,
                             MQTTPropBuilder_t *pSendPropsBuffer,
                             MQTTPropBuilder_t *pGetPropsBuffer)
{
    (void)pReasonCode;
    (void)pSendPropsBuffer;
    (void)pGetPropsBuffer;

    if ((pPacketInfo->type & 0xF0U) == MQTT_PACKET_TYPE_PUBLISH) {
        MQTTPublishInfo_t *pPublish = pDeserializedInfo->pPublishInfo;
        if (pPublish == NULL) return true;

        /* pNetworkContext zeigt auf netCtx — darüber den umgebenden Struct finden */
        NetworkContext_t *pNet = (NetworkContext_t *)pCtx->transportInterface.pNetworkContext;
        MqttServiceCtx_t *pService = (MqttServiceCtx_t *)((uint8_t *)pNet -
                                      offsetof(MqttServiceCtx_t, netCtx));

        if (pService->onMessage != NULL) {
            /* Topic null-terminieren (temporärer Puffer) */
            char topicBuf[128];
            size_t topicLen = pPublish->topicNameLength;
            if (topicLen >= sizeof(topicBuf)) topicLen = sizeof(topicBuf) - 1;
            memcpy(topicBuf, pPublish->pTopicName, topicLen);
            topicBuf[topicLen] = '\0';

            pService->onMessage(topicBuf, (const char *)pPublish->pPayload,
                                pPublish->payloadLength);
        }
    }
    return true;
}

MQTTStatus_t mqtt_service_connect(MqttServiceCtx_t *pSvc,
                                  const char *pBrokerIp,
                                  const char *pClientId,
                                  MqttMessageCb_t onMessage)
{
    pSvc->onMessage = onMessage;

    if (mqtt_transport_connect(&pSvc->netCtx, pBrokerIp, 1883) != 0) {
        return MQTTNoMemory;
    }

    pSvc->transport.recv            = mqtt_transport_recv;
    pSvc->transport.send            = mqtt_transport_send;
    pSvc->transport.writev          = NULL;
    pSvc->transport.pNetworkContext = &pSvc->netCtx;

    MQTTFixedBuffer_t fixedBuf = {
        .pBuffer = pSvc->networkBuf,
        .size    = sizeof(pSvc->networkBuf)
    };

    MQTTStatus_t status = MQTT_Init(&pSvc->mqttCtx,
                                    &pSvc->transport,
                                    prvGetTimeMs,
                                    prvEventCallback,
                                    &fixedBuf);
    if (status != MQTTSuccess) {
        mqtt_transport_disconnect(&pSvc->netCtx);
        return status;
    }

    status = MQTT_InitStatefulQoS(&pSvc->mqttCtx,
                                  pSvc->outgoingRecords, 10,
                                  pSvc->incomingRecords, 10,
                                  NULL, 0);
    if (status != MQTTSuccess) {
        mqtt_transport_disconnect(&pSvc->netCtx);
        return status;
    }

    /* Last Will: Broker published "offline" wenn die Verbindung unerwartet bricht */
    char willTopic[48];
    snprintf(willTopic, sizeof(willTopic), "status/%s", pClientId);
    MQTTPublishInfo_t will = { 0 };
    will.qos             = MQTTQoS0;
    will.retain          = true;
    will.pTopicName      = willTopic;
    will.topicNameLength = (uint16_t)strlen(willTopic);
    will.pPayload        = "offline";
    will.payloadLength   = 7U;

    MQTTConnectInfo_t conn = { 0 };
    conn.cleanSession           = true;
    conn.keepAliveSeconds       = MQTT_KEEP_ALIVE_SEC;
    conn.pClientIdentifier      = pClientId;
    conn.clientIdentifierLength = (uint16_t)strlen(pClientId);

    bool sessionPresent = false;
    status = MQTT_Connect(&pSvc->mqttCtx, &conn, &will,
                          MQTT_CONNACK_TIMEOUT_MS, &sessionPresent,
                          NULL, NULL);
    if (status != MQTTSuccess) {
        mqtt_transport_disconnect(&pSvc->netCtx);
    }

    return status;
}

MQTTStatus_t mqtt_service_subscribe(MqttServiceCtx_t *pSvc, const char *pTopic)
{
    MQTTSubscribeInfo_t sub = { 0 };
    sub.qos               = MQTTQoS0;
    sub.pTopicFilter      = pTopic;
    sub.topicFilterLength = (uint16_t)strlen(pTopic);

    uint16_t packetId = MQTT_GetPacketId(&pSvc->mqttCtx);
    return MQTT_Subscribe(&pSvc->mqttCtx, &sub, 1U, packetId, NULL);
}

MQTTStatus_t mqtt_service_publish(MqttServiceCtx_t *pSvc,
                                  const char *pTopic,
                                  const char *pPayload)
{
    MQTTPublishInfo_t pub = { 0 };
    pub.qos             = MQTTQoS0;
    pub.pTopicName      = pTopic;
    pub.topicNameLength = (uint16_t)strlen(pTopic);
    pub.pPayload        = pPayload;
    pub.payloadLength   = strlen(pPayload);

    return MQTT_Publish(&pSvc->mqttCtx, &pub, 0U, NULL);
}

MQTTStatus_t mqtt_service_process(MqttServiceCtx_t *pSvc)
{
    return MQTT_ProcessLoop(&pSvc->mqttCtx);
}

void mqtt_service_disconnect(MqttServiceCtx_t *pSvc)
{
    MQTT_Disconnect(&pSvc->mqttCtx, NULL, NULL);
    mqtt_transport_disconnect(&pSvc->netCtx);
}
