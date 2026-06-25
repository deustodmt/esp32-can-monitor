#include "hardware_setup.h"
#include "config.h"

#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include <OneButton.h>
#include "driver/twai.h"

extern Adafruit_NeoPixel strip;
extern OneButton button;

void setup_CAN()
{
    printf("[CAN] Inicializando bus CAN (TWAI)...\n");
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_LISTEN_ONLY);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    printf("[CAN] TX_PIN=%d, RX_PIN=%d, SE_PIN=%d\n", CAN_TX_PIN, CAN_RX_PIN, CAN_SE_PIN);

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
        printf("[CAN] Driver CAN instalado con éxito.\n");
    else {
        printf("[CAN] ERROR: Fallo al instalar CAN.\n");
        return;
    }
    if (twai_start() == ESP_OK) {
        printf("[CAN] Bus CAN Iniciado con éxito (250kbps, listen-only).\n");
    }
}

void setup_LED()
{
    strip.begin();
    strip.setBrightness(255);
    strip.setPixelColor(0, strip.Color(0, 255, 0));
    strip.show();
}

void setup_button()
{
    printf("[BTN] Setup button on pin %d\n", BUTTON_PIN);
    button.setup(BUTTON_PIN, INPUT_PULLUP, true);

    button.attachClick([]{
        printf("[BTN] Click\n");
    });

    button.attachLongPressStop([]{
        printf("[BTN] Long press\n");
    });
}
