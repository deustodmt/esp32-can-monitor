#include "sd_handler.h"
#include "vcu_states.h"
#include "config.h"
#include "sd_manage.h"

#include "Arduino.h"
#include "WiFi.h"
#include <PubSubClient.h>

extern SD_Manage *sdManager;
extern xQueueHandle can_queue;
extern WiFiClient    wifiClient;
extern PubSubClient  mqttClient;

void sdWriteTask(void *pvParameters)
{
    for (;;)
    {
        if (current_state == CAN_TO_SD)
        {
            sdManager->write_queue_to_sd();
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        else if (current_state == DUMP_VIA_WIFI)
        {
            if (!mqttClient.connected()) {
                printf("DUMP_VIA_WIFI: esperando conexión MQTT...\n");
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            printf("DUMP_VIA_WIFI: iniciando volcado de SD...\n");

            fs::File file = SD.open("/log.bin", FILE_READ);
            if (!file) {
                printf("DUMP_VIA_WIFI: no se pudo abrir log.bin\n");
                vTaskDelay(pdMS_TO_TICKS(2000));
                current_state = CAN_TO_SD;
                continue;
            }

            uint8_t raw_msg[CAN_MSG_SIZE];
            char    hex_payload[41];
            int     count = 0;
            bool    all_sent_ok = true;

            while (file.available() >= CAN_MSG_SIZE)
            {
                if (!mqttClient.connected()) {
                    printf("DUMP_VIA_WIFI: ERROR - Conexión MQTT perdida. Abortando...\n");
                    all_sent_ok = false;
                    break;
                }

                file.read(raw_msg, CAN_MSG_SIZE);

                uint32_t ts_ms, can_id;
                memcpy(&ts_ms,  &raw_msg[0], 4);
                memcpy(&can_id, &raw_msg[4], 4);

                uint8_t server_msg[20] = {0};

                server_msg[0] = (can_id >> 24) & 0xFF;
                server_msg[1] = (can_id >> 16) & 0xFF;
                server_msg[2] = (can_id >>  8) & 0xFF;
                server_msg[3] =  can_id        & 0xFF;

                server_msg[4]  = 0;
                server_msg[5]  = 0;
                server_msg[6]  = 0;
                server_msg[7]  = 0;
                server_msg[8]  = (ts_ms >> 24) & 0xFF;
                server_msg[9]  = (ts_ms >> 16) & 0xFF;
                server_msg[10] = (ts_ms >>  8) & 0xFF;
                server_msg[11] =  ts_ms        & 0xFF;

                memcpy(&server_msg[12], &raw_msg[9], 8);

                for (int i = 0; i < 20; i++)
                    sprintf(&hex_payload[i * 2], "%02X", server_msg[i]);
                hex_payload[40] = '\0';

                if (!mqttClient.publish(MQTT_TOPIC, hex_payload)) {
                    printf("DUMP_VIA_WIFI: ERROR - Fallo al publicar el mensaje %d. Abortando...\n", count);
                    all_sent_ok = false;
                    break;
                }

                mqttClient.loop();
                count++;

                if (count % 50 == 0)
                    vTaskDelay(pdMS_TO_TICKS(10));
            }

            file.close();

            if (all_sent_ok && count > 0) {
                printf("DUMP_VIA_WIFI: Volcado completo con éxito — %d tramas enviadas.\n", count);
                sdManager->delete_sd_file();
            } else if (!all_sent_ok) {
                printf("DUMP_VIA_WIFI: Hubo errores. NO se ha borrado log.bin para evitar pérdida de datos.\n");
            } else {
                printf("DUMP_VIA_WIFI: El archivo estaba vacío.\n");
            }

            current_state = CAN_TO_SD;
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}
