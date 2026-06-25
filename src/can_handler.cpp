#include "can_handler.h"
#include "vcu_states.h"
#include "config.h"
#include "nx0002_sts01_a01.h"

#include "Arduino.h"
#include "driver/twai.h"

extern xQueueHandle can_queue;
extern xQueueHandle wifi_queue;

void canReadTask(void *pvParameters)
{
    twai_message_t message;
    uint8_t raw_msg[CAN_MSG_SIZE];

    for (;;)
    {
        if (current_state == CAN_TO_SD || current_state == CAN_TO_WIFI)
        {
            if (twai_receive(&message, pdMS_TO_TICKS(100)) == ESP_OK)
            {
                if (message.identifier == BMS_TX_STATE_3_ID) {
                    last_bms_msg_time = millis();

                    struct nx0002_sts01_a01_bms_tx_state_3_t bms_status;

                    nx0002_sts01_a01_bms_tx_state_3_unpack(&bms_status, message.data, message.data_length_code);

                    current_bms_state = (BMS_STATE_t)bms_status.app_state_app;
                }

                uint32_t timestamp = millis();
                memset(raw_msg, 0, CAN_MSG_SIZE);

                memcpy(&raw_msg[0], &timestamp,            4);
                memcpy(&raw_msg[4], &message.identifier,   4);
                raw_msg[8] = message.data_length_code;
                memcpy(&raw_msg[9], message.data, message.data_length_code);

                if (current_state == CAN_TO_SD)
                    xQueueSend(can_queue,  raw_msg, 0);
                else
                    xQueueSend(wifi_queue, raw_msg, 0);

                printf("ID: 0x%03X | DLC: %d | Data: ",
                       message.identifier, message.data_length_code);
                for (int i = 0; i < message.data_length_code; i++)
                    printf("%02X ", message.data[i]);
                printf("\n");
            }
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}
