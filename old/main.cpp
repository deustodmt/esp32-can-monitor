#include "Arduino.h"
#include "config.h"
#include "SD_manage.h"
#include "CAN_manage.h"
#include "WiFi_manage.h"
#include <Adafruit_NeoPixel.h>
#include <OneButton.h>

typedef enum
{
    CAN_TO_SD,
    CAN_TO_WIFI,
    DUMP_VIA_WIFI,
} STATES;

STATES current_state = CAN_TO_SD;

xQueueHandle canMessageQueue;

CAN_Manage *CAN_manage;
SD_Manage *sd_manage;
WiFi_Manage *wifi_manage;

Adafruit_NeoPixel strip(1, 4, NEO_GRB + NEO_KHZ800);
OneButton button;

void setup_LED()
{
    strip.begin();                                  // INITIALIZE NeoPixel strip object (REQUIRED)
    strip.show();                                   // Turn OFF all pixels ASAP
    strip.setBrightness(255);                       // Set BRIGHTNESS to MAX
    strip.setPixelColor(0, strip.Color(0, 255, 0)); // Set first pixel (there is only one) to green
    strip.show();
}

void setup_button()
{
    button.setup(BUTTON_PIN, INPUT_PULLUP, true); // Set BOT button to change between modes

    button.attachClick([] { // Set
        printf("botón pulsado -----");
        if (current_state == CAN_TO_SD)
        {
            printf("--- Cambiando a modo WiFI-DUMP!\n");
            current_state = DUMP_VIA_WIFI;
            strip.setPixelColor(0, strip.Color(0, 0, 255));
        }
        else if (current_state == DUMP_VIA_WIFI)
        {
            printf("--- Cambiando a modo CAN a SD!\n");
            current_state = CAN_TO_SD;
            strip.setPixelColor(0, strip.Color(0, 255, 0));
        }
        strip.show();
    });

    button.attachLongPressStop([] { // Set
        printf("botón pulsado -----");
        strip.setPixelColor(0, strip.Color(255, 0, 255));
        strip.show();
    });
}

void setupSDThread(void *pvParameters)
{
    const TickType_t xDelay = 500 / portTICK_PERIOD_MS;

    SD_Manage *sd_manager = (SD_Manage *)pvParameters;
    
    while (1)
    {
        sd_manager->writeQueueToSD();
        vTaskDelay(xDelay);
    }
}

void setup()
{
    Serial.begin(BAUD_RATE);

    setup_LED(); // Initialize LED light library
    setup_button();

    canMessageQueue = xQueueCreate(50, sizeof(uint8_t[CAN_MSG_SIZE]));

    if (canMessageQueue == NULL)
    {
        Serial.println("Error creating queue");
        while (1)
        { // Blink LED in red forever
            strip.setPixelColor(0, strip.Color(255, 0, 0));
            strip.show();
            vTaskDelay(500);
            strip.setPixelColor(0, strip.Color(0, 0, 0));
            strip.show();
            vTaskDelay(500);
        }
        return;
    }

    CAN_manage = new CAN_Manage(canMessageQueue);
    sd_manage = new SD_Manage(canMessageQueue);

    xTaskCreatePinnedToCore(setupSDThread, "setupSDThread", 4096, (void *)sd_manage, 5, NULL, 1);

    printf("Arrancado\n");
}

void loop()
{
    button.tick();

    switch (current_state)
    {
    case CAN_TO_SD:
        CAN_manage->poll();
        printf("Number of messages in queue: %d\n", uxQueueMessagesWaiting(canMessageQueue));
        break;
    case CAN_TO_WIFI:
        if (wifi_manage == NULL)
        {
            wifi_manage = new WiFi_Manage();
        }

        CAN_manage->poll();
        break;
    case DUMP_VIA_WIFI:
        if (wifi_manage == NULL)
        {
            wifi_manage = new WiFi_Manage();
        }
        wifi_manage->mqtt_test();
        break;
    }
}