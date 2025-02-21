#include "CAN_manage.h"
#include "config.h"
#include <HardwareSerial.h>
#include <message_queue.hpp>
#include <time.h>

CAN_device_t CAN_cfg;

CAN_Manage::CAN_Manage() {
  pinMode(PIN_5V_EN, OUTPUT);
  digitalWrite(PIN_5V_EN, HIGH);

  pinMode(CAN_SE_PIN, OUTPUT);
  digitalWrite(CAN_SE_PIN, LOW);

  CAN_cfg.speed = CAN_SPEED;
  CAN_cfg.tx_pin_id = TX_PIN;
  CAN_cfg.rx_pin_id = RX_PIN;
  CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));

  // Init CAN Module
  ESP32Can.CANInit();

  Serial.print("CAN SPEED :");
  Serial.println(CAN_cfg.speed);

  // put your setup code here, to run once:
}

void CAN_Manage::sendMessage() {
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    CAN_frame_t tx_frame;
    tx_frame.FIR.B.FF = CAN_frame_std;
    tx_frame.MsgID = 0x001;
    tx_frame.FIR.B.DLC = 8;
    tx_frame.data.u8[0] = 0x00;
    tx_frame.data.u8[1] = 0x01;
    tx_frame.data.u8[2] = 0x02;
    tx_frame.data.u8[3] = 0x03;
    tx_frame.data.u8[4] = 0x04;
    tx_frame.data.u8[5] = 0x05;
    tx_frame.data.u8[6] = 0x06;
    tx_frame.data.u8[7] = 0x07;

    ESP32Can.CANWriteFrame(&tx_frame);
    Serial.println("CAN send done");
  }
}

void CAN_Manage::poll() {

  CAN_frame_t rx_frame;
  uint8_t message[rx_frame.FIR.B.DLC + 4]; // binarry message: milliseconds/can_frame

  // Receive next CAN frame from queue
  
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) ==
      pdTRUE) {

    if (rx_frame.FIR.B.FF == CAN_frame_std) {
      printf("New standard frame");
    } else {
      printf("New extended frame");
    }

    if (rx_frame.FIR.B.RTR == CAN_RTR) { // Received Remote Transmission Request (we are asked to send data)
      printf(" RTR from 0x%08X, DLC %d\r\n", rx_frame.MsgID,
             rx_frame.FIR.B.DLC);
    } else { // No RTR, it is a common message
      printf(" from 0x%08X, DLC %d, Data ", rx_frame.MsgID, rx_frame.FIR.B.DLC);

      long currentTime = millis();
      
      message[0] = (uint8_t) (currentTime & 0xFF);            //We take the less significant byte
      message[1] = (uint8_t) ((currentTime >> 8) & 0xFF);     //We displace the current time millis
      message[2] = (uint8_t) ((currentTime >> 16) & 0xFF);
      message[3] = (uint8_t) ((currentTime >> 24) & 0xFF);

      for (int i = 0; i < rx_frame.FIR.B.DLC; i++) { // iterate over data using DLC (data length)
        message[i + 4] = rx_frame.data.u8[i];
        printf("0x%02X ", rx_frame.data.u8[i]);
      }
      // char string[rx_frame.FIR.B.DLC + 1];
      // memcpy(string, rx_frame.data.u8, rx_frame.FIR.B.DLC); // copy data to string
      // string[rx_frame.FIR.B.DLC] = '\0'; // null terminate string
      printf("\n");
    }
  }
}