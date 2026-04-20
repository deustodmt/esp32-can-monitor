#ifndef __CONFIG_H__
#define __CONFIG_H__

#define BAUD_RATE 9600

// PIN
#define PIN_5V_EN 16

#define CAN_TX_PIN 27
#define CAN_RX_PIN 26
#define CAN_SE_PIN 23

#define RS485_EN_PIN 17
#define RS485_TX_PIN 22
#define RS485_RX_PIN 21
#define RS485_SE_PIN 19

#define SD_MISO_PIN 2
#define SD_MOSI_PIN 15
#define SD_SCLK_PIN 14
#define SD_CS_PIN   13

#define BUTTON_PIN  0
#define WS2812_PIN  4

// CAN MESSAGE SIZE (bytes packed per frame in can_queue / wifi_queue)
#define CAN_MSG_SIZE 20

// WIFI
// Reemplaza 10.42.0.1 por la IP exacta de tu portátil si NetworkManager asignó otra
#define SERVER_URL  "http://10.42.0.1:8000" 
#define WIFI_SSID   "ESP32_Net"
#define WIFI_PASS   "secreto1234"

// MQTT — port 2000 is the external Docker-mapped port for mosquitto
#define MQTT_SERVER "10.42.0.1"
#define MQTT_PORT   2000
#define MQTT_USER   "admin"
#define MQTT_PASSWD "admin"
// [ADDED] Topic the Python server subscribes to
#define MQTT_TOPIC  "test_topic"

#endif
