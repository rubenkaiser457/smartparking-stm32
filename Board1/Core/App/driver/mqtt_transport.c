#include "mqtt_transport.h"
#include "lwip/api.h"
#include "lwip/ip_addr.h"
#include <string.h>

/* Empfangs-Timeout in ms — gibt coreMQTT Zeit zum Prozessieren ohne zu blockieren */
#define RECV_TIMEOUT_MS    100U

int mqtt_transport_connect(NetworkContext_t *pCtx, const char *pBrokerIp, uint16_t port)
{
    ip_addr_t addr;
    if (ipaddr_aton(pBrokerIp, &addr) == 0) {
        return -1;
    }

    pCtx->conn = netconn_new(NETCONN_TCP);
    if (pCtx->conn == NULL) {
        return -1;
    }

    pCtx->rxBuf    = NULL;
    pCtx->rxOffset = 0;

    netconn_set_recvtimeout(pCtx->conn, RECV_TIMEOUT_MS);

    err_t err = netconn_connect(pCtx->conn, &addr, port);
    if (err != ERR_OK) {
        netconn_delete(pCtx->conn);
        pCtx->conn = NULL;
        return -1;
    }

    return 0;
}

void mqtt_transport_disconnect(NetworkContext_t *pCtx)
{
    if (pCtx->rxBuf != NULL) {
        netbuf_delete(pCtx->rxBuf);
        pCtx->rxBuf = NULL;
    }
    pCtx->rxOffset = 0;

    if (pCtx->conn != NULL) {
        netconn_close(pCtx->conn);
        netconn_delete(pCtx->conn);
        pCtx->conn = NULL;
    }
}

int32_t mqtt_transport_send(NetworkContext_t *pCtx, const void *pBuf, size_t len)
{
    err_t err = netconn_write(pCtx->conn, pBuf, len, NETCONN_COPY);
    return (err == ERR_OK) ? (int32_t)len : -1;
}

int32_t mqtt_transport_recv(NetworkContext_t *pCtx, void *pBuf, size_t len)
{
    /* Falls noch kein Puffer ansteht, einen neuen empfangen. */
    if (pCtx->rxBuf == NULL) {
        err_t err = netconn_recv(pCtx->conn, &pCtx->rxBuf);

        if (err == ERR_TIMEOUT) {
            pCtx->rxBuf = NULL;
            return 0; /* kein Fehler, nur keine Daten */
        }
        if (err != ERR_OK || pCtx->rxBuf == NULL) {
            pCtx->rxBuf = NULL;
            return -1;
        }
        pCtx->rxOffset = 0;
    }

    /* Aus dem aktuellen Puffer so viele Bytes wie angefragt herauskopieren.
       netbuf_copy_partial behandelt auch verkettete (fragmentierte) Puffer. */
    u16_t total     = netbuf_len(pCtx->rxBuf);
    u16_t remaining = (u16_t)(total - pCtx->rxOffset);
    u16_t toCopy    = (remaining > (u16_t)len) ? (u16_t)len : remaining;

    netbuf_copy_partial(pCtx->rxBuf, pBuf, toCopy, pCtx->rxOffset);
    pCtx->rxOffset = (uint16_t)(pCtx->rxOffset + toCopy);

    /* Puffer erst freigeben, wenn er komplett gelesen wurde. */
    if (pCtx->rxOffset >= total) {
        netbuf_delete(pCtx->rxBuf);
        pCtx->rxBuf    = NULL;
        pCtx->rxOffset = 0;
    }

    return (int32_t)toCopy;
}
