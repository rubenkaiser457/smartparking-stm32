/*
 * User configuration for coreMQTT.
 * Muss im Compiler-Include-Pfad liegen (bereits in Core/Inc).
 */
#ifndef CORE_MQTT_CONFIG_H
#define CORE_MQTT_CONFIG_H

/* Max. gleichzeitig in-flight QoS1/2 Pakete */
#define MQTT_STATE_ARRAY_MAX_COUNT    ( 10U )

/* Logging abschalten (kein printf im embedded target) */
#define LogError( x )
#define LogWarn( x )
#define LogInfo( x )
#define LogDebug( x )

#endif /* CORE_MQTT_CONFIG_H */
