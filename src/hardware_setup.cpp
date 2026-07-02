#include "hardware_setup.h"
#include "config.h"
#include "vcu_states.h"

#include "Arduino.h"
#include "driver/twai.h"
#include <Adafruit_NeoPixel.h>
#include <OneButton.h>

extern Adafruit_NeoPixel strip;
extern OneButton button;

void setup_CAN() {
  printf("[CAN] Inicializando bus CAN (TWAI)...\n");
  pinMode(CAN_SE_PIN, OUTPUT);
  digitalWrite(CAN_SE_PIN, LOW); // Modo normal (alta velocidad)

  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  printf("[CAN] TX_PIN=%d, RX_PIN=%d, SE_PIN=%d\n", CAN_TX_PIN, CAN_RX_PIN,
         CAN_SE_PIN);

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    printf("[CAN] Driver CAN instalado con éxito.\n");
  else {
    printf("[CAN] ERROR: Fallo al instalar CAN.\n");
    return;
  }
  if (twai_start() == ESP_OK) {
    printf("[CAN] Bus CAN Iniciado con éxito.\n");
  }
}

void setup_LED() {
  strip.begin();
  strip.setBrightness(255);
  strip.setPixelColor(0, strip.Color(0, 255, 0));
  strip.show();
}

void set_mode_color(LOG_STATE_t state) {
  switch (state) {
  case CAN_TO_SD:
    strip.setPixelColor(0, strip.Color(0, 255, 0));
    break;
  case DUMP_VIA_WIFI:
    strip.setPixelColor(0, strip.Color(255, 0, 0));
    break;
  case CAN_TO_WIFI:
    strip.setPixelColor(0, strip.Color(255, 0, 255));
    break;
  }
  strip.show();
}

void setup_button() {
  printf("[BTN] Setup button on pin %d\n", BUTTON_PIN);
  button.setup(BUTTON_PIN, INPUT_PULLUP, true);

  button.attachClick([] {
    printf("[BTN] Click\n");
    if (current_state == CAN_TO_SD) {
      current_state = DUMP_VIA_WIFI;
    } else if (current_state == DUMP_VIA_WIFI) {
      current_state = CAN_TO_WIFI;
    } else {
      current_state = CAN_TO_SD;
    }
    set_mode_color(current_state);
    printf("[BTN] Mode changed to: %d\n", current_state);
  });

  button.attachLongPressStop([] {
    printf("[BTN] Long press\n");
    if (current_state == CAN_TO_WIFI) {
      current_state = CAN_TO_SD;
    } else if (current_state == CAN_TO_SD) {
      current_state = DUMP_VIA_WIFI;
    } else {
      current_state = CAN_TO_WIFI;
    }
    set_mode_color(current_state);
    printf("[BTN] Long press mode changed to: %d\n", current_state);
  });
}
