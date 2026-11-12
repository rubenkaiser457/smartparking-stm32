/*
 * coreMQTT v2 — missing utility implementations.
 *
 * The coreMQTT v2 source tree factors these helpers out into a separate
 * compilation unit.  The copy in this project was missing that file, so the
 * linker could not find the symbols referenced by core_mqtt.c and
 * core_mqtt_serializer.c.  All implementations follow the MQTT 5.0 spec
 * (https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html).
 */

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "core_mqtt_serializer.h"
#include "private/core_mqtt_serializer_private.h"

/* ── Variable-length integer helpers (MQTT spec 1.5.5) ─────────────────── */

uint32_t variableLengthEncodedSize( uint32_t length )
{
    uint32_t encodedSize = 0U;
    uint32_t remaining = length;

    do
    {
        remaining >>= 7U;
        encodedSize++;
    } while( remaining > 0U );

    return encodedSize;
}

/*-----------------------------------------------------------*/

uint8_t * encodeVariableLength( uint8_t * pDestination,
                                uint32_t length )
{
    uint8_t * pIter = pDestination;
    uint32_t remaining = length;

    do
    {
        uint8_t encodedByte = ( uint8_t ) ( remaining & 0x7FU );
        remaining >>= 7U;

        if( remaining > 0U )
        {
            encodedByte |= 0x80U;
        }

        *pIter = encodedByte;
        pIter++;
    } while( remaining > 0U );

    return pIter;
}

/*-----------------------------------------------------------*/

MQTTStatus_t decodeVariableLength( const uint8_t * pBuffer,
                                   size_t bufferLength,
                                   uint32_t * pLength )
{
    MQTTStatus_t status = MQTTSuccess;
    uint32_t multiplier = 1U;
    uint32_t value = 0U;
    size_t i = 0U;
    uint8_t encodedByte;

    if( ( pBuffer == NULL ) || ( pLength == NULL ) )
    {
        return MQTTBadParameter;
    }

    do
    {
        if( i >= bufferLength )
        {
            status = MQTTBadResponse;
            break;
        }

        encodedByte = pBuffer[ i ];
        i++;
        value += ( uint32_t ) ( encodedByte & 0x7FU ) * multiplier;
        multiplier *= 128U;

        if( multiplier > ( 128U * 128U * 128U ) )
        {
            status = MQTTBadResponse;
            break;
        }
    } while( ( encodedByte & 0x80U ) != 0U );

    if( status == MQTTSuccess )
    {
        *pLength = value;
    }

    return status;
}

/* ── String encoder (2-byte length prefix + data) ──────────────────────── */

uint8_t * encodeString( uint8_t * pDestination,
                        const char * pSource,
                        uint16_t sourceLength )
{
    uint8_t * pIter = pDestination;

    *pIter = UINT16_HIGH_BYTE( sourceLength );
    pIter++;
    *pIter = UINT16_LOW_BYTE( sourceLength );
    pIter++;

    if( ( pSource != NULL ) && ( sourceLength > 0U ) )
    {
        ( void ) memcpy( pIter, pSource, sourceLength );
    }

    pIter += sourceLength;

    return pIter;
}

/* ── Property decoders ──────────────────────────────────────────────────── */

MQTTStatus_t decodeUint8t( uint8_t * pProperty,
                           uint32_t * pPropertyLength,
                           bool * pUsed,
                           uint8_t ** pIndex )
{
    if( ( pProperty == NULL ) || ( pPropertyLength == NULL ) ||
        ( pUsed == NULL ) || ( pIndex == NULL ) )
    {
        return MQTTBadParameter;
    }

    if( *pPropertyLength < 1U )
    {
        return MQTTBadResponse;
    }

    if( *pUsed )
    {
        return MQTTBadResponse;
    }

    *pProperty = **pIndex;
    ( *pIndex )++;
    ( *pPropertyLength )--;
    *pUsed = true;

    return MQTTSuccess;
}

/*-----------------------------------------------------------*/

MQTTStatus_t decodeUint16t( uint16_t * pProperty,
                            uint32_t * pPropertyLength,
                            bool * pUsed,
                            uint8_t ** pIndex )
{
    if( ( pProperty == NULL ) || ( pPropertyLength == NULL ) ||
        ( pUsed == NULL ) || ( pIndex == NULL ) )
    {
        return MQTTBadParameter;
    }

    if( *pPropertyLength < 2U )
    {
        return MQTTBadResponse;
    }

    if( *pUsed )
    {
        return MQTTBadResponse;
    }

    *pProperty = ( uint16_t ) ( ( ( uint16_t ) ( *pIndex )[ 0 ] ) << 8U ) |
                 ( uint16_t ) ( ( *pIndex )[ 1 ] );
    ( *pIndex ) += 2U;
    ( *pPropertyLength ) -= 2U;
    *pUsed = true;

    return MQTTSuccess;
}

/*-----------------------------------------------------------*/

MQTTStatus_t decodeUint32t( uint32_t * pProperty,
                            uint32_t * pPropertyLength,
                            bool * pUsed,
                            uint8_t ** pIndex )
{
    if( ( pProperty == NULL ) || ( pPropertyLength == NULL ) ||
        ( pUsed == NULL ) || ( pIndex == NULL ) )
    {
        return MQTTBadParameter;
    }

    if( *pPropertyLength < 4U )
    {
        return MQTTBadResponse;
    }

    if( *pUsed )
    {
        return MQTTBadResponse;
    }

    *pProperty = ( ( uint32_t ) ( *pIndex )[ 0 ] << 24U ) |
                 ( ( uint32_t ) ( *pIndex )[ 1 ] << 16U ) |
                 ( ( uint32_t ) ( *pIndex )[ 2 ] << 8U )  |
                 ( ( uint32_t ) ( *pIndex )[ 3 ] );
    ( *pIndex ) += 4U;
    ( *pPropertyLength ) -= 4U;
    *pUsed = true;

    return MQTTSuccess;
}

/*-----------------------------------------------------------*/

MQTTStatus_t decodeUtf8( const char ** pProperty,
                         size_t * pLength,
                         uint32_t * pPropertyLength,
                         bool * pUsed,
                         uint8_t ** pIndex )
{
    uint16_t strLen;

    if( ( pProperty == NULL ) || ( pLength == NULL ) ||
        ( pPropertyLength == NULL ) || ( pUsed == NULL ) || ( pIndex == NULL ) )
    {
        return MQTTBadParameter;
    }

    if( *pPropertyLength < 2U )
    {
        return MQTTBadResponse;
    }

    if( *pUsed )
    {
        return MQTTBadResponse;
    }

    strLen = ( uint16_t ) ( ( ( uint16_t ) ( *pIndex )[ 0 ] ) << 8U ) |
             ( uint16_t ) ( ( *pIndex )[ 1 ] );
    ( *pIndex ) += 2U;
    ( *pPropertyLength ) -= 2U;

    if( ( uint32_t ) strLen > *pPropertyLength )
    {
        return MQTTBadResponse;
    }

    *pProperty = ( const char * ) ( *pIndex );
    *pLength = ( size_t ) strLen;
    ( *pIndex ) += strLen;
    ( *pPropertyLength ) -= ( uint32_t ) strLen;
    *pUsed = true;

    return MQTTSuccess;
}

/*-----------------------------------------------------------*/

MQTTStatus_t decodeUserProp( const char ** pPropertyKey,
                             size_t * pPropertyKeyLen,
                             const char ** pPropertyValue,
                             size_t * pPropertyValueLen,
                             uint32_t * pPropertyLength,
                             uint8_t ** pIndex )
{
    uint16_t keyLen;
    uint16_t valLen;

    if( ( pPropertyKey == NULL ) || ( pPropertyKeyLen == NULL ) ||
        ( pPropertyValue == NULL ) || ( pPropertyValueLen == NULL ) ||
        ( pPropertyLength == NULL ) || ( pIndex == NULL ) )
    {
        return MQTTBadParameter;
    }

    /* Read key length (2 bytes) */
    if( *pPropertyLength < 2U )
    {
        return MQTTBadResponse;
    }

    keyLen = ( uint16_t ) ( ( ( uint16_t ) ( *pIndex )[ 0 ] ) << 8U ) |
             ( uint16_t ) ( ( *pIndex )[ 1 ] );
    ( *pIndex ) += 2U;
    ( *pPropertyLength ) -= 2U;

    /* Read key */
    if( ( uint32_t ) keyLen > *pPropertyLength )
    {
        return MQTTBadResponse;
    }

    *pPropertyKey = ( const char * ) ( *pIndex );
    *pPropertyKeyLen = ( size_t ) keyLen;
    ( *pIndex ) += keyLen;
    ( *pPropertyLength ) -= ( uint32_t ) keyLen;

    /* Read value length (2 bytes) */
    if( *pPropertyLength < 2U )
    {
        return MQTTBadResponse;
    }

    valLen = ( uint16_t ) ( ( ( uint16_t ) ( *pIndex )[ 0 ] ) << 8U ) |
             ( uint16_t ) ( ( *pIndex )[ 1 ] );
    ( *pIndex ) += 2U;
    ( *pPropertyLength ) -= 2U;

    /* Read value */
    if( ( uint32_t ) valLen > *pPropertyLength )
    {
        return MQTTBadResponse;
    }

    *pPropertyValue = ( const char * ) ( *pIndex );
    *pPropertyValueLen = ( size_t ) valLen;
    ( *pIndex ) += valLen;
    ( *pPropertyLength ) -= ( uint32_t ) valLen;

    return MQTTSuccess;
}

/* ── Fixed-header serializers ───────────────────────────────────────────── */

uint8_t * serializeAckFixed( uint8_t * pIndex,
                             uint8_t packetType,
                             uint16_t packetId,
                             uint32_t remainingLength,
                             MQTTSuccessFailReasonCode_t reasonCode )
{
    uint8_t * pIter = pIndex;

    /* Fixed header byte */
    *pIter = packetType;
    pIter++;

    /* Remaining length (variable-length encoded) */
    pIter = encodeVariableLength( pIter, remainingLength );

    /* Packet ID */
    *pIter = UINT16_HIGH_BYTE( packetId );
    pIter++;
    *pIter = UINT16_LOW_BYTE( packetId );
    pIter++;

    /* Reason code: present only when remaining length > 2 (MQTT5 extended form) */
    if( remainingLength > 2U )
    {
        *pIter = ( uint8_t ) reasonCode;
        pIter++;
    }

    return pIter;
}

/*-----------------------------------------------------------*/

uint8_t * serializeConnectFixedHeader( uint8_t * pIndex,
                                       const MQTTConnectInfo_t * pConnectInfo,
                                       const MQTTPublishInfo_t * pWillInfo,
                                       uint32_t remainingLength )
{
    uint8_t * pIter = pIndex;
    uint8_t connectFlags = 0U;

    /* Fixed header: CONNECT packet type (0x10), no reserved bits */
    *pIter = MQTT_PACKET_TYPE_CONNECT;
    pIter++;

    /* Remaining length */
    pIter = encodeVariableLength( pIter, remainingLength );

    /* Protocol name: length (2 bytes) + "MQTT" */
    *pIter = 0x00U;
    pIter++;
    *pIter = 0x04U;
    pIter++;
    *pIter = ( uint8_t ) 'M';
    pIter++;
    *pIter = ( uint8_t ) 'Q';
    pIter++;
    *pIter = ( uint8_t ) 'T';
    pIter++;
    *pIter = ( uint8_t ) 'T';
    pIter++;

    /* Protocol version: 5 for MQTT 5.0 */
    *pIter = 0x05U;
    pIter++;

    /* Connect flags */
    if( pConnectInfo->cleanSession )
    {
        connectFlags |= 0x02U; /* Clean Start */
    }

    if( pWillInfo != NULL )
    {
        connectFlags |= 0x04U; /* Will Flag */
        connectFlags |= ( uint8_t ) ( ( uint8_t ) pWillInfo->qos << 3U ); /* Will QoS */

        if( pWillInfo->retain )
        {
            connectFlags |= 0x20U; /* Will Retain */
        }
    }

    if( pConnectInfo->pPassword != NULL )
    {
        connectFlags |= 0x40U; /* Password Flag */
    }

    if( pConnectInfo->pUserName != NULL )
    {
        connectFlags |= 0x80U; /* User Name Flag */
    }

    *pIter = connectFlags;
    pIter++;

    /* Keep Alive (2 bytes, big-endian) */
    *pIter = UINT16_HIGH_BYTE( pConnectInfo->keepAliveSeconds );
    pIter++;
    *pIter = UINT16_LOW_BYTE( pConnectInfo->keepAliveSeconds );
    pIter++;

    return pIter;
}

/*-----------------------------------------------------------*/

uint8_t * serializeDisconnectFixed( uint8_t * pIndex,
                                    const MQTTSuccessFailReasonCode_t * pReasonCode,
                                    uint32_t remainingLength )
{
    uint8_t * pIter = pIndex;

    /* Fixed header: DISCONNECT packet type (0xE0) */
    *pIter = MQTT_PACKET_TYPE_DISCONNECT;
    pIter++;

    /* Remaining length */
    pIter = encodeVariableLength( pIter, remainingLength );

    /* Reason code: present only when there is content beyond the header */
    if( ( remainingLength > 0U ) && ( pReasonCode != NULL ) )
    {
        *pIter = ( uint8_t ) ( *pReasonCode );
        pIter++;
    }

    return pIter;
}

/*-----------------------------------------------------------*/

uint8_t * serializeSubscribeHeader( uint32_t remainingLength,
                                    uint8_t * pIndex,
                                    uint16_t packetId )
{
    uint8_t * pIter = pIndex;

    /* Fixed header: SUBSCRIBE (0x82) */
    *pIter = MQTT_PACKET_TYPE_SUBSCRIBE;
    pIter++;

    /* Remaining length */
    pIter = encodeVariableLength( pIter, remainingLength );

    /* Packet ID */
    *pIter = UINT16_HIGH_BYTE( packetId );
    pIter++;
    *pIter = UINT16_LOW_BYTE( packetId );
    pIter++;

    return pIter;
}

/*-----------------------------------------------------------*/

uint8_t * serializeUnsubscribeHeader( uint32_t remainingLength,
                                      uint8_t * pIndex,
                                      uint16_t packetId )
{
    uint8_t * pIter = pIndex;

    /* Fixed header: UNSUBSCRIBE (0xA2) */
    *pIter = MQTT_PACKET_TYPE_UNSUBSCRIBE;
    pIter++;

    /* Remaining length */
    pIter = encodeVariableLength( pIter, remainingLength );

    /* Packet ID */
    *pIter = UINT16_HIGH_BYTE( packetId );
    pIter++;
    *pIter = UINT16_LOW_BYTE( packetId );
    pIter++;

    return pIter;
}

/* ── SUBACK/UNSUBACK property length decoder ────────────────────────────── */

MQTTStatus_t decodeSubackPropertyLength( const uint8_t * pIndex,
                                         uint32_t remainingLength,
                                         uint32_t * subackPropertyLength )
{
    MQTTStatus_t status;
    uint32_t propertyLength = 0U;
    uint32_t encodedSize;

    if( ( pIndex == NULL ) || ( subackPropertyLength == NULL ) )
    {
        return MQTTBadParameter;
    }

    /* Decode the variable-length property length field */
    status = decodeVariableLength( pIndex, ( size_t ) remainingLength, &propertyLength );

    if( status == MQTTSuccess )
    {
        /* encodedSize = bytes consumed by the variable-length field itself */
        encodedSize = variableLengthEncodedSize( propertyLength );

        if( ( encodedSize + propertyLength ) > remainingLength )
        {
            status = MQTTBadResponse;
        }
        else
        {
            /* Return total: bytes for the length field + the property bytes */
            *subackPropertyLength = encodedSize + propertyLength;
        }
    }

    return status;
}

/* ── MQTTPropAdd helpers ────────────────────────────────────────────────── */

MQTTStatus_t MQTTPropAdd_MaxPacketSize( MQTTPropBuilder_t * pPropertyBuilder,
                                        uint32_t maxPacketSize,
                                        const uint8_t * pOptionalMqttPacketType )
{
    /* Suppress unused-parameter warning — validation not required here */
    ( void ) pOptionalMqttPacketType;

    if( pPropertyBuilder == NULL )
    {
        return MQTTBadParameter;
    }

    /* Need 1 byte property ID + 4 bytes value */
    if( ( pPropertyBuilder->bufferLength - pPropertyBuilder->currentIndex ) < 5U )
    {
        return MQTTNoMemory;
    }

    uint8_t * pDest = &pPropertyBuilder->pBuffer[ pPropertyBuilder->currentIndex ];

    /* Property ID: Maximum Packet Size (0x27) */
    *pDest = MQTT_MAX_PACKET_SIZE_ID;
    pDest++;

    /* Value: 4-byte big-endian */
    *pDest = ( uint8_t ) ( maxPacketSize >> 24U );
    pDest++;
    *pDest = ( uint8_t ) ( ( maxPacketSize >> 16U ) & 0xFFU );
    pDest++;
    *pDest = ( uint8_t ) ( ( maxPacketSize >> 8U ) & 0xFFU );
    pDest++;
    *pDest = ( uint8_t ) ( maxPacketSize & 0xFFU );
    pDest++;

    pPropertyBuilder->currentIndex += 5U;

    return MQTTSuccess;
}
