#include "mqtt_task.h"
#include "mqtt_service.h"
#include "servo.h"         /* Schranken-Servo (TIM4_CH4 / PD15 / D9) */
#include "ampel.h"         /* Ampel-LED (Tor-Signal): rot = zu, gruen = offen */
#include "cmsis_os.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "main.h"
#include "mqtt_config.h"   /* gemeinsame Broker-IP (Shared/mqtt_config.h) */
#include <string.h>

/* Servo-Hardware-Selbsttest beim Boot: Schranke faehrt 3x auf/zu, dann laeuft
   die normale MQTT-Logik weiter. Zum Aktivieren diese Zeile einkommentieren. */
/* #define SERVO_SELFTEST */

/* ---- Konfiguration ---------------------------------------------------- */
/* MQTT_BROKER_IP kommt aus Shared/mqtt_config.h (fuer beide Boards) */
#define MQTT_CLIENT_ID     "board2"
#define TOPIC_STATUS       "status/board2"   /* Online/Offline */
#define TOPIC_EXIT         "parking/exit"    /* Auto ausgefahren -> Laptop zaehlt +1 */

/* Timing der Ausfahrt-Sequenz (ms) -- bei Bedarf anpassen. */
#define PASS_CLEAR_DELAY_MS   2000U   /* nach "Auto durch": Zeit zum sicheren Durchfahren, dann zu */

/* ---- Ausfahrt-Events (von der Sensor-Logik signalisiert) -------------- */
/* Die Abstandssensor-Logik (eigener Task) ruft diese Funktionen auf; sie
   setzen nur ein Flag und sind damit thread-sicher. Die eigentliche Reaktion
   (Schranke/Publish) passiert im MQTT-Task -> coreMQTT wird nie aus einem
   fremden Task heraus benutzt. */
static volatile uint8_t s_evtArrived = 0;   /* Auto an der Schranke erkannt */
static volatile uint8_t s_evtPassed  = 0;   /* Auto durchgefahren erkannt   */

void parking_signal_arrival(void) { s_evtArrived = 1; }
void parking_signal_passed(void)  { s_evtPassed  = 1; }

/* ---- Task-Attribute --------------------------------------------------- */
static const osThreadAttr_t s_mqttTaskAttr = {
    .name       = "mqttTask",
    .stack_size = 4096U,
    .priority   = osPriorityNormal,
};

static MqttServiceCtx_t s_mqttSvc;

/* ---- Watchdog (IWDG, direkt ueber Register) --------------------------- */
/* Friert der Task jemals ein (haengendes Senden/Verbinden), wird der IWDG
   nicht mehr gefuettert -> Board startet sich nach ~20s selbst neu und
   verbindet frisch. Im Normalbetrieb wird er in jeder Schleife gekickt.
   Direkter Registerzugriff, da das HAL-IWDG-Modul nicht eingebunden ist. */
static void watchdogInit(void)
{
#ifdef DBGMCU_APB1_FZ_DBG_IWDG_STOP
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP; /* IWDG beim Debug-Halt anhalten */
#endif
    IWDG->KR  = 0x5555U;  /* Schreibzugriff auf PR/RLR freigeben */
    IWDG->PR  = 6U;       /* Prescaler /256 -> LSI ~32kHz => ~8ms pro Tick */
    IWDG->RLR = 2500U;    /* 2500 * 8ms ~= 20s Timeout (kurze TX-Haenger
                             erholen sich selbst; nur echte Freezes -> Reboot) */
    IWDG->KR  = 0xAAAAU;  /* Zaehler neu laden */
    IWDG->KR  = 0xCCCCU;  /* Watchdog starten */
}

static inline void watchdogKick(void)
{
    IWDG->KR = 0xAAAAU;   /* Zaehler neu laden (fuettern) */
}

/* ---- Nachrichten-Callback --------------------------------------------- */
/* Board2 (Ausfahrt) abonniert keine Topics -- die Ausfahrt ist immer erlaubt,
   es ist kein Platz-Status noetig. Der Callback bleibt nur, weil
   mqtt_service_connect ihn als Parameter erwartet; er wird nie aufgerufen. */
static void onMessage(const char *pTopic, const char *pPayload, size_t payloadLen)
{
    (void)pTopic;
    (void)pPayload;
    (void)payloadLen;
}

/* ---- Hilfsfunktionen -------------------------------------------------- */
static void publishStatus(const char *value)
{
    mqtt_service_publish(&s_mqttSvc, TOPIC_STATUS, value);
}

static void reconnect(void)
{
    mqtt_service_disconnect(&s_mqttSvc);
    osDelay(2000);
    for (;;) {
        watchdogKick();
        if (mqtt_service_connect(&s_mqttSvc, MQTT_BROKER_IP, MQTT_CLIENT_ID, onMessage)
                == MQTTSuccess) {
            publishStatus("online");
            break;
        }
        osDelay(1000);
    }
}

/* ---- Ausfahrt-Ablauf (laeuft im MQTT-Task) ---------------------------- */
static int s_carAdmitted = 0;   /* wurde fuer das aktuelle Auto geoeffnet? */

/* Auto steht an der Schranke: Ausfahrt ist immer erlaubt -> Schranke auf + gruen. */
static void handleCarArrived(void)
{
    servo_open();           /* Schranke auf UND sofort gruen ("durchfahren") */
    ampel_gruen();
    s_carAdmitted = 1;
}

/* Auto ist durch: exit melden, dann schliessen. Der Laptop zaehlt freie
   Plaetze +1 und schickt neuen parking/status an Board1/Dashboard. */
static void handleCarPassed(void)
{
    if (s_carAdmitted) {
        ampel_rot();                    /* sofort rot: kein weiteres Auto durch */
        osDelay(PASS_CLEAR_DELAY_MS);   /* Auto sicher durchfahren lassen ... */
        mqtt_service_publish(&s_mqttSvc, TOPIC_EXIT, "{\"event\":\"exit\"}");
        servo_close();                  /* ... erst jetzt Schranke runter */
        s_carAdmitted = 0;
    } else {
        servo_close();                  /* nur sicherstellen, dass zu + rot */
        ampel_rot();
    }
}

/* ---- Task ------------------------------------------------------------- */
static void prvMqttTask(void *arg)
{
    (void)arg;

    watchdogInit();
    servo_init();                 /* Schranke konfigurieren (startet geschlossen) */
    ampel_init();                 /* Ampel-LEDs konfigurieren (starten aus) */
    ampel_rot();                  /* Ruhezustand: Schranke zu -> rot */

#ifdef SERVO_SELFTEST
    for (int i = 0; i < 3; i++) {
        servo_open();  osDelay(700); watchdogKick();
        servo_close(); osDelay(700); watchdogKick();
    }
#endif

    extern struct netif gnetif;
    while (!dhcp_supplied_address(&gnetif)) {
        watchdogKick();
        osDelay(500);
    }

    for (;;) {
        watchdogKick();
        if (mqtt_service_connect(&s_mqttSvc, MQTT_BROKER_IP, MQTT_CLIENT_ID, onMessage)
                == MQTTSuccess) {
            break;
        }
        osDelay(1000);
    }
    publishStatus("online");

    /* TEST-Stand-in fuer den Abstandssensor: der USER-Button simuliert ein Auto.
       gedrueckt   = Auto steht an der Schranke  -> handleCarArrived()
       losgelassen = Auto ist durchgefahren      -> handleCarPassed()
       Im echten Betrieb signalisiert der HC-SR04 dasselbe ueber
       parking_signal_arrival() / parking_signal_passed(). */
    GPIO_PinState lastBtn = HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin);

    for (;;) {
        watchdogKick();
        MQTTStatus_t st = mqtt_service_process(&s_mqttSvc);
        /* MQTTNeedMoreBytes = Paket kam fragmentiert an, kein Fehler (coreMQTT >= 2.1) */
        if (st != MQTTSuccess && st != MQTTNeedMoreBytes) {
            reconnect();
            lastBtn = HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin);
            continue;
        }

        /* Events von der Sensor-Logik (echter Betrieb) */
        if (s_evtArrived) { s_evtArrived = 0; handleCarArrived(); }
        if (s_evtPassed)  { s_evtPassed  = 0; handleCarPassed();  }

        /* Button als Test-Trigger (Flankenerkennung, active-high) */
        GPIO_PinState btn = HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin);
        if (btn != lastBtn) {
            if (btn == GPIO_PIN_SET) handleCarArrived();
            else                     handleCarPassed();
            lastBtn = btn;
        }

        osDelay(20);   /* Entprellung + CPU-Entlastung */
    }
}

/* ---- Öffentliche Funktion --------------------------------------------- */
void mqtt_task_start(void)
{
    osThreadNew(prvMqttTask, NULL, &s_mqttTaskAttr);
}
