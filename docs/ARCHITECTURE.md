# Architecture and Lessons Learned

## Design decisions

- **MQTT with a central broker:** The boards never talk to each other directly. They only know the broker, so additional barriers could be added without changing existing nodes.
- **Retained status message:** Node-RED publishes the number of free spaces as a retained message. The entry board caches the last value and decides instantly when a car arrives. After a reboot it receives the current state right after connecting, which solves the cold-start problem.
- **Traffic light shows the gate state:** Red means barrier closed, green means barrier open. The number of free spaces is shown on the display and the dashboard, so the light can never contradict the barrier decision.
- **One sensor per barrier:** A single HC-SR04 detects both the arriving car (distance drops) and the completed pass-through (distance rises again).
- **Shared configuration:** The broker address lives in `Shared/mqtt_config.h` and is included by both CubeIDE projects.

## Hardware and wiring

| Signal | Pin (Nucleo-F439ZI) | Notes |
|---|---|---|
| HC-SR04 TRIG | D7 / PF13 | 3.3 V trigger is sufficient |
| HC-SR04 ECHO | D8 / PF12 | 5 V output, divided to 3.3 V with 1 kOhm / 2 kOhm |
| Servo PWM | D9 / PD15 | TIM4_CH4, 50 Hz, external 5 V supply, common ground |
| Traffic light green | D4 / PF14 | 220 Ohm series resistor |
| Traffic light red | D2 / PF15 | 220 Ohm series resistor |
| 7-segment a..g (Board 1) | PE11, PE9, PE13, PD14, PG9, PG14, PA5 | Common cathode, 220 to 330 Ohm per segment |

The servos must not be powered from the board. Their start-up current spike (up to about 1 A) can collapse the supply rail and reset both boards.

## Problems solved during development

1. **LwIP initialisation race:** The MQTT task started before `MX_LWIP_Init()` had finished. It is now started from the default task after the network stack is up.
2. **Ethernet never initialised:** The `.ioc` configured Ethernet, but the MSP init, the interrupt handler and the PHY link thread were never generated. They were added manually in user code sections, so they survive code regeneration.
3. **Duplicate MAC address:** Both boards used the same MAC. Board 2 now uses a different address.
4. **Transport dropped received bytes:** The TCP receive function discarded the rest of a network buffer when coreMQTT requested fewer bytes, so the CONNACK was never read completely. Fixed with a persistent receive buffer and partial copies.
5. **TX buffer leak:** Sent Ethernet frames were only released in a rare error path. After a few packets the TX descriptors were exhausted and sending stopped. They are now released on every transmit call.
6. **Payload read past its end:** coreMQTT payloads are not null-terminated. String search on the raw payload sometimes found old data in neighbouring memory. Payloads are now copied into a terminated buffer first.
7. **Rare task freeze:** In rare cases the MQTT task blocked forever inside a network call. An independent hardware watchdog (IWDG) now resets the board if the task stops feeding it, so the node always recovers on its own.
8. **Constant reconnect loop:** Since coreMQTT 2.1, `MQTT_ProcessLoop` returns `MQTTNeedMoreBytes` for fragmented packets. This normal status was treated as an error and triggered a reconnect every 20 to 45 seconds. It is now handled correctly and the connections stay stable.
9. **Servo brownout:** When the servo started, the current limit of the lab power supply collapsed the 5 V rail and reset both boards via the common ground. Fixed by raising the current limit. A 470 to 1000 uF buffer capacitor on the servo rail is recommended.

**Dead end:** A send timeout via `LWIP_SO_SNDTIMEO` made `netconn_write` fail immediately on the CONNECT packet, so no board could connect anymore. It was removed again.

## Known limitation and possible improvement

A single ultrasonic sensor cannot distinguish between a car that passes through and a car that backs away. In the second case the barrier still closes correctly, but an entry is counted. A second sensor behind the barrier (for example a light barrier) would confirm the pass-through.
