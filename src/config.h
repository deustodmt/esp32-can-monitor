#ifndef __CONFIG_H__
#define __CONFIG_H__

#define BAUD_RATE 9600

// PIN
#define PIN_5V_EN 16

#define CAN_TX_PIN 26
#define CAN_RX_PIN 27
#define CAN_SE_PIN 23

#define RS485_EN_PIN 17 // 17 /RE
#define RS485_TX_PIN 22 // 21
#define RS485_RX_PIN 21 // 22
#define RS485_SE_PIN 19 // 22 /SHDN

#define SD_MISO_PIN 2
#define SD_MOSI_PIN 15
#define SD_SCLK_PIN 14
#define SD_CS_PIN 13

#define BUTTON_PIN 0
#define WS2812_PIN 4

//CAN MESSAGE SIZE
#define CAN_MSG_SIZE 12

//WIFI
#define SERVER_URL "http://10.33.3.166:8000"
#define WIFI_SSID "udmt"
#define WIFI_PASS "udmt2122"

// MQTT
#define MQTT_SERVER "10.33.3.144"
#define MQTT_PORT 2000
#define MQTT_USER "admin"
#define MQTT_PASSWD "admin"

#endif