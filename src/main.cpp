#include "Arduino.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>
#include <OneButton.h>

typedef enum
{
    CAN_TO_SD,
    CAN_TO_WIFI,
    DUMP_VIA_WIFI,
} STATES;

STATES current_state = CAN_TO_SD;

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

void setup()
{
    Serial.begin(BAUD_RATE);

    setup_button();
    setup_LED();
}

void loop()
{
}