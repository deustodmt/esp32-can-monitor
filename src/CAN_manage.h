#ifndef __CAN_MANAGE_H__
#define __CAN_MANAGE_H__
#include <Arduino.h>
#include <CAN_config.h>
#include <ESP32CAN.h>

extern CAN_device_t CAN_cfg;             // CAN Config

class CAN_Manage {
    private:
        const CAN_speed_t CAN_SPEED = CAN_SPEED_250KBPS;
        const gpio_num_t RX_PIN = GPIO_NUM_26;   // CAN Receiver Pin
        const gpio_num_t TX_PIN = GPIO_NUM_27;   // CAN Transmitter Pin
        unsigned long previousMillis = 0; // will store last time a CAN Message was send
        unsigned long currentMillis = millis();
        const int interval = 1000;        // interval at which send CAN Messages (milliseconds)
        const int rx_queue_size = 10;     // Receive Queue size
  
    public:
        CAN_Manage();
        void setup();
        void sendMessage();
        void poll();
};
#endif