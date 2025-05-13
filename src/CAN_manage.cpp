#include "CAN_manage.h"
#include "config.h"
#include <HardwareSerial.h>
#include <time.h>

CAN_device_t CAN_cfg;

CAN_Manage::CAN_Manage(xQueueHandle sd_queue) {
  pinMode(PIN_5V_EN, OUTPUT);
  digitalWrite(PIN_5V_EN, HIGH);

  pinMode(CAN_SE_PIN, OUTPUT);
  digitalWrite(CAN_SE_PIN, LOW);

  CAN_cfg.speed = CAN_SPEED;
  CAN_cfg.tx_pin_id = TX_PIN;
  CAN_cfg.rx_pin_id = RX_PIN;
  CAN_cfg.rx_queue = xQueueCreate(50, sizeof(uint8_t[CAN_MSG_SIZE]));

  this->SD_queue = sd_queue;

  // Init CAN Module
  ESP32Can.CANInit();
  printf("CAN Manager ready\n");
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
  printf("CAN bus polling\n");

  CAN_frame_t rx_frame;
  uint8_t message[CAN_MSG_SIZE]; // binary message: milliseconds/can_frame

  // Receive next CAN frame from queue
  
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) ==
      pdTRUE) {

    if (rx_frame.FIR.B.RTR == CAN_RTR) { // Received Remote Transmission Request (we are asked to send data)
      printf(" RTR from 0x%08X, DLC %d\r\n", rx_frame.MsgID,
             rx_frame.FIR.B.DLC);
    } else { // No RTR, it is a common message
      printf(" from 0x%08X, DLC %d, Data ", rx_frame.MsgID, rx_frame.FIR.B.DLC);

      uint32_t millisTimestamp = millis();
      uint8_t timestamp[8];

      memcpy(timestamp, &millisTimestamp, sizeof(millisTimestamp));
      memset(timestamp + 4, 0, 4);

      // Message ID (4 bytes)
      memcpy(message, &rx_frame.MsgID, sizeof(rx_frame.MsgID));
      // Timestamp
      memcpy(message + 4, timestamp, sizeof(timestamp));
      // Payload (8 bytes)
      memcpy(message + 4 + 8, &rx_frame.data.u8, sizeof(rx_frame.data.u8));

      if (xQueueSend(this->SD_queue, message, 0) != pdTRUE) {
        printf("No se ha podido añadir a la cola el mensaje CAN. Posible cola llena.\n");
      }

      // for (int i = 0; i < rx_frame.FIR.B.DLC; i++) { // iterate over data using DLC (data length)
      //   message[i + 4] = rx_frame.data.u8[i];
      //   printf("0x%02X ", rx_frame.data.u8[i]);
      // }


    }

  }
}