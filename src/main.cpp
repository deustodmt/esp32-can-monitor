#include "Arduino.h"
#include "config.h"
#include "SD_manage.h"
#include "CAN_manage.h"
#include "WiFi_manage.h"
#include <message_queue.hpp>
#include <Adafruit_NeoPixel.h>
#include <OneButton.h>
#include <freertos_core.hpp>

typedef enum
{
    CAN_TO_SD,
    CAN_TO_WIFI,
    DUMP_VIA_WIFI,
} STATES;

STATES current_state = CAN_TO_SD;

using namespace freertos;
extern message_queue<uint8_t[12]> queue;

xTaskHandle CAN_task_handle;
xTaskHandle WiFi_task_handle;
xTaskHandle SD_task_handle;

CAN_Manage CAN_manage;
SD_Manage *sd_manage;
WiFi_Manage *wifi_manage = NULL;

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

static void CAN_Task(void *pvParameters)
{
    CAN_Manage *canInstance = (CAN_Manage *)pvParameters;
    while (1)
    {
        canInstance->poll();
        // Opcional: añadir un pequeño retraso
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup()
{
    Serial.begin(BAUD_RATE);

    setup_LED(); // Initialize LED light library
    setup_button();

    printf("Arrancado\n");

    // sd_manage = new SD_Manage();

    // queue.initialize();
    // SD_thread = thread::create([](void*){
    //     while(queue.receive()) {
    //         Serial.print("Queue received ");
    //         Serial.println(i);
    //     }
    // },nullptr,1,2000);
    // SD_thread.start();

    // setup_WiFi();
    //  http_get_example();
    //  post("TEST MESSAGE");
    // printf("COriiendo en: %d", xPortGetCoreID()); // corre en el core 1
}

void change_mode(STATES new_mode)
{
}

void loop()
{
    button.tick();

    switch (current_state)
    {
    case CAN_TO_SD:
        if (sd_manage == NULL)
        {
            sd_manage = new SD_Manage();
        }
        CAN_manage.poll();
        
        break;
    case CAN_TO_WIFI:
        if (wifi_manage == NULL)
        {
            wifi_manage = new WiFi_Manage();
        }

        CAN_manage.poll();
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