#include "mqtt_handler.h"
#include "vcu_states.h"
#include "config.h"

#include "Arduino.h"
#include "WiFi.h"
#include <PubSubClient.h>

extern WiFiClient    wifiClient;
extern PubSubClient  mqttClient;
extern xQueueHandle  wifi_queue;
extern void          packForServer(const uint8_t *raw_msg, char *hex_out);

void wifiPublishTask(void *pvParameters)
{
    printf("WiFi: conectando a %s...\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED)
        vTaskDelay(pdMS_TO_TICKS(500));
    printf("WiFi conectado: %s\n", WiFi.localIP().toString().c_str());

    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

    uint8_t raw_msg[CAN_MSG_SIZE];
    char    hex_payload[41];

    for (;;)
    {
        if (!mqttClient.connected())
        {
            printf("MQTT: conectando a %s:%d...\n", MQTT_SERVER, MQTT_PORT);
            if (mqttClient.connect("ESP32Monitor", MQTT_USER, MQTT_PASSWD))
                printf("MQTT: conectado\n");
            else {
                printf("MQTT: fallo (estado %d), reintentando en 5s\n",
                       mqttClient.state());
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
        }

        mqttClient.loop();

        if (current_state == CAN_TO_WIFI)
        {
            while (xQueueReceive(wifi_queue, raw_msg, 0) == pdTRUE)
            {
                packForServer(raw_msg, hex_payload);
                if (!mqttClient.publish(MQTT_TOPIC, hex_payload))
                    printf("MQTT: publish falló (buffer lleno?)\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void packForServer(const uint8_t *raw_msg, char *hex_out)
{
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
        sprintf(&hex_out[i * 2], "%02X", server_msg[i]);
    hex_out[40] = '\0';
}
