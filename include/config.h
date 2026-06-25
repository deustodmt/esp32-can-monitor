#ifndef __CONFIG_H__
#define __CONFIG_H__

#define BAUD_RATE 9600

#define BMS_TIMEOUT_MS 2000
#define BMS_CAN_TIMEOUT_MS 1000

// ==========================================
// PINES ESP32-S3-DevKitC-1
// ==========================================

// CAN BUS (TJA1051T)
#define CAN_TX_PIN 27
#define CAN_RX_PIN 26
#define CAN_SE_PIN 23
// Standby/Silent

// DETECCIÓN DE CARGA Y CONTROL
#define CHARGE_DETECT_PIN 5
#define HV_SWITCH_PIN 18

// MÓDULO SD (SPI)
#define SD_MISO_PIN 2
#define SD_SCLK_PIN 14
#define SD_CS_PIN 13
#define SD_MOSI_PIN 15

// INTERFAZ DE USUARIO (Mantenemos los de la placa DevKit)
#define BUTTON_PIN 0
#define WS2812_PIN 4

// ==========================================
// CONFIGURACIÓN DE TRAMAS Y RED
// ==========================================
#define CAN_MSG_SIZE 20

#define WIFI_SSID "ESP32_Net"
#define WIFI_PASS "secreto1234"

#define MQTT_SERVER "10.42.0.1"
#define MQTT_PORT 2000
#define MQTT_USER "admin"
#define MQTT_PASSWD "admin"
#define MQTT_TOPIC "test_topic"

// CAN IDs for BMS/ECU communication
#define CAN_ID_BMS_CMD 0x1000000A
#define CAN_ID_BMS_STATUS 0x1000000B
#define CAN_ID_ECU_CMD 0x0C00000A

#define BMS_TX_STATE_3_ID 0x462

#define BMS_RX_CTRL_1_ID 0x360

#endif
