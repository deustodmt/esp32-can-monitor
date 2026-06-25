#include "Arduino.h"
#include "config.h"
#include "vcu_states.h"
#include "sd_manage.h"
#include "sd_handler.h"
#include "can_handler.h"
#include "can_commands.h"
#include "mqtt_handler.h"
#include "hardware_setup.h"
#include "vcu_state_machine.h"

#include <Adafruit_NeoPixel.h>
#include <OneButton.h>
#include "driver/twai.h"
#include <WiFi.h>
#include <PubSubClient.h>

volatile uint32_t last_bms_msg_time = 0;
LOG_STATE_t current_state = CAN_TO_SD;
ESP_STATE_t current_vcu_state = ESP_INIT;
BMS_STATE_t current_bms_state = BMS_STANDBY;

Adafruit_NeoPixel strip(1, WS2812_PIN, NEO_GRB + NEO_KHZ800);
OneButton button;

xQueueHandle can_queue;
xQueueHandle wifi_queue;

SD_Manage *sdManager;

WiFiClient    wifiClient;
PubSubClient  mqttClient(wifiClient);

void setup()
{
    Serial.begin(BAUD_RATE);

    pinMode(CHARGE_DETECT_PIN, INPUT_PULLDOWN);
    pinMode(HV_SWITCH_PIN, INPUT_PULLDOWN);

    setup_button();
    setup_LED();

    can_queue  = xQueueCreate(200, CAN_MSG_SIZE);
    wifi_queue = xQueueCreate(200, CAN_MSG_SIZE);
    if (can_queue == NULL || wifi_queue == NULL) {
        printf("ERROR: No se pudo crear la cola FreeRTOS\n");
        return;
    }

    sdManager = new SD_Manage(can_queue);

    setup_CAN();

    xTaskCreatePinnedToCore(canReadTask,         "CAN_Read",      4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(vcuStateMachineTask, "VCU_State",    4096, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(sdWriteTask,         "SD_Write",     8192, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(wifiPublishTask,     "WiFi_Pub",     8192, NULL, 2, NULL, 1);
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(100));
}
