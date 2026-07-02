#include "can_handler.h"
#include "config.h"
#include "nx0002_sts01_a01.h"
#include "vcu_states.h"

#include "Arduino.h"
#include "driver/twai.h"

extern xQueueHandle can_queue;
extern xQueueHandle wifi_queue;

void canReadTask(void *pvParameters) {
  twai_message_t message;
  uint8_t raw_msg[CAN_MSG_SIZE];
  uint32_t can_msg_count = 0;
  uint32_t rx_errors = 0;

  for (;;) {
    if (current_state == CAN_TO_SD || current_state == CAN_TO_WIFI) {
      esp_err_t ret = twai_receive(&message, pdMS_TO_TICKS(100));
      twai_status_info_t status;
      if (ret == ESP_OK) {
        can_msg_count++;

        // Si el mensaje viene de la BMS (IDs conocidos), actualizar timestamp
        if (message.identifier == BMS_TX_STATE_3_ID ||
            message.identifier == 0x400 || message.identifier == 0x460 ||
            message.identifier == 0x461 || message.identifier == 0x463 ||
            message.identifier == 0x464 || message.identifier == 0x466 ||
            message.identifier == 0x468) {
          last_bms_msg_time = millis();
        }
        if (message.identifier == BMS_TX_STATE_3_ID) {
          struct nx0002_sts01_a01_bms_tx_state_3_t bms_status;

          nx0002_sts01_a01_bms_tx_state_3_unpack(&bms_status, message.data,
                                                 message.data_length_code);

          current_bms_state = (BMS_STATE_t)bms_status.app_state_app;

          printf("[%lu] CAN[%s] ID:0x%03X | BMS_STATE:%d | DIO1:%d | DIO2:%d | "
                 "DIO3:%d | DIO4:%d | HVIL:%d\n",
                 millis(), "RX", message.identifier, current_bms_state,
                 bms_status.dio1_state, bms_status.dio2_state,
                 bms_status.dio3_state, bms_status.dio4_state,
                 bms_status.hvil_state);
        }

        uint32_t timestamp = millis();
        memset(raw_msg, 0, CAN_MSG_SIZE);

        memcpy(&raw_msg[0], &timestamp, 4);
        memcpy(&raw_msg[4], &message.identifier, 4);
        raw_msg[8] = message.data_length_code;
        memcpy(&raw_msg[9], message.data, message.data_length_code);

        if (current_state == CAN_TO_SD) {
          if (xQueueSend(can_queue, raw_msg, 0) == pdTRUE) {
            // printf("[CAN] -> CAN_QUEUE OK (total:%lu)\n", can_msg_count);
          } else {
            printf("[CAN] ERROR: CAN_QUEUE FULL (total:%lu)\n", can_msg_count);
          }
        } else {
          if (xQueueSend(wifi_queue, raw_msg, 0) == pdTRUE) {
            // printf("[CAN] -> WIFI_QUEUE OK (total:%lu)\n", can_msg_count);
          } else {
            printf("[CAN] ERROR: WIFI_QUEUE FULL (total:%lu)\n", can_msg_count);
          }
        }

        printf("[%lu] CAN[%s] ID:0x%03X | DLC:%d | Data:", millis(), "RX",
               message.identifier, message.data_length_code);
        for (int i = 0; i < message.data_length_code; i++)
          printf(" %02X", message.data[i]);
        printf("\n");
      } else {
        rx_errors++;
        printf("[CAN] twai_receive error: %d (count %lu)\n", ret, rx_errors);
      }
    } else {
      printf("[CAN] RX: state=%d, no recibiendo\n", current_state);
      vTaskDelay(pdMS_TO_TICKS(200));
    }
  }
}
