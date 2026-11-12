#ifndef SENSOR_TEST_H
#define SENSOR_TEST_H

/*
 * Eigenstaendiger HC-SR04-Test OHNE Netzwerk/MQTT.
 * Misst die Distanz und gibt sie ueber USART3 (ST-Link USB Virtual COM Port,
 * 115200 Baud 8N1) aus. Im seriellen Terminal mitlesen.
 *
 * Aufruf aus StartDefaultTask() statt mqtt_task_start().
 */
void sensor_test_start(void);

#endif /* SENSOR_TEST_H */
