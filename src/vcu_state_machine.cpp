#include "vcu_state_machine.h"
#include "vcu_states.h"
#include "config.h"

#include "Arduino.h"
#include "OneButton.h"

extern void request_bms_state(BMS_STATE_t desired_state);
extern void request_ecu_torque(int torque_value);

static OneButton* g_button;
static void (*g_button_callback)() = nullptr;
static void (*g_long_press_callback)() = nullptr;

void setup_vcu_button(OneButton* button, void (*callback)(), void (*long_press_callback)())
{
    g_button = button;
    g_button_callback = callback;
    g_long_press_callback = long_press_callback;

    button->setup(BUTTON_PIN, INPUT_PULLUP, true);

    button->attachClick([]{
        printf("Botón pulsado ----- \n");
        if (g_button_callback) g_button_callback();
    });

    button->attachLongPressStop([]{
        printf("Botón pulsado largo -----\n");
        if (g_long_press_callback) g_long_press_callback();
    });
}

void vcuStateMachineTask(void *pvParameters)
{
    static ESP_STATE_t prev_vcu_state = ESP_INIT;
    static uint32_t state_entry_time = 0;

    for (;;) {
        bool is_charger_connected = (digitalRead(CHARGE_DETECT_PIN) == HIGH);
        bool is_hv_switch_on = (digitalRead(HV_SWITCH_PIN) == LOW);

        if (current_vcu_state != prev_vcu_state) {
            state_entry_time = millis();
            prev_vcu_state = current_vcu_state;
        }

        if (current_vcu_state != ESP_INIT && current_vcu_state != ESP_FAULT) {
            if (millis() - last_bms_msg_time > BMS_CAN_TIMEOUT_MS) {
                Serial.println("VCU ERROR CRÍTICO: ¡Señal CAN de la BMS perdida!");

                if (current_vcu_state == ESP_DRIVE ||
                    current_vcu_state == ESP_TO_HV_READY ||
                    current_vcu_state == ESP_EMERGENCY_TORQUE_CUT) {

                    request_ecu_torque(0);
                    Serial.println("VCU: Cortando tracción de emergencia (Torque = 0).");
                }

                current_vcu_state = ESP_FAULT;
            }
        }

        switch (current_vcu_state) {

            case ESP_INIT:
                Serial.println("VCU: Iniciando sistema...");
                current_vcu_state = ESP_STANDBY;
                break;

            case ESP_STANDBY:
                request_bms_state(BMS_STANDBY);
                if (is_charger_connected) {
                    current_vcu_state = ESP_CHARGE_TO_IDLE;
                } else if (is_hv_switch_on) {
                    current_vcu_state = ESP_TO_IDLE;
                }
                break;

            case ESP_TO_IDLE:
                request_bms_state(BMS_IDLE);

                if (current_bms_state == BMS_IDLE) {
                    current_vcu_state = ESP_TO_HV_READY;
                }
                else if (millis() - state_entry_time > BMS_TIMEOUT_MS) {
                    Serial.println("VCU ERROR: Timeout esperando IDLE de la BMS.");
                    current_vcu_state = ESP_FAULT;
                }

                if (!is_hv_switch_on) current_vcu_state = ESP_TO_SHUTDOWN;
                break;

            case ESP_TO_HV_READY:
                request_bms_state(BMS_HV_READY_PRECHARGE);

                if (current_bms_state == BMS_HV_READY) {
                    Serial.println("VCU: BMS en HV READY. ¡MOTO LISTA!");
                    current_vcu_state = ESP_DRIVE;
                }
                else if (millis() - state_entry_time > BMS_TIMEOUT_MS * 2) {
                    Serial.println("VCU ERROR: Timeout en Precarga/HV Ready.");
                    current_vcu_state = ESP_FAULT;
                }

                if (!is_hv_switch_on) current_vcu_state = ESP_TO_SHUTDOWN;
                break;

            case ESP_DRIVE:
                request_bms_state(BMS_HV_READY);

                if (!is_hv_switch_on) {
                    current_vcu_state = ESP_TO_SHUTDOWN;
                } else if (is_charger_connected) {
                    current_vcu_state = ESP_EMERGENCY_TORQUE_CUT;
                }
                break;

            case ESP_EMERGENCY_TORQUE_CUT:
                request_ecu_torque(0);

                if (millis() - state_entry_time > 50) {
                    current_vcu_state = ESP_TO_SHUTDOWN;
                }
                break;

            case ESP_TO_SHUTDOWN:
                request_bms_state(BMS_HV_SHUTDOWN);

                if (current_bms_state == BMS_HV_SHUTDOWN) {
                    current_vcu_state = ESP_SHUTDOWN_TO_IDLE;
                }
                else if (millis() - state_entry_time > BMS_TIMEOUT_MS) {
                    Serial.println("VCU ERROR: Timeout forzando SHUTDOWN.");
                    current_vcu_state = ESP_FAULT;
                }
                break;

            case ESP_SHUTDOWN_TO_IDLE:
                request_bms_state(BMS_IDLE);

                if (current_bms_state == BMS_IDLE) {
                    if (is_charger_connected) current_vcu_state = ESP_CHARGE_MODE;
                    else current_vcu_state = ESP_SHUTDOWN_TO_STANDBY;
                }
                else if (millis() - state_entry_time > BMS_TIMEOUT_MS) {
                    Serial.println("VCU ERROR: Timeout bajando a IDLE.");
                    current_vcu_state = ESP_FAULT;
                }
                break;

            case ESP_SHUTDOWN_TO_STANDBY:
                request_bms_state(BMS_STANDBY);

                if (current_bms_state == BMS_STANDBY) {
                    current_vcu_state = ESP_STANDBY;
                }
                else if (millis() - state_entry_time > BMS_TIMEOUT_MS) {
                    Serial.println("VCU ERROR: Timeout bajando a STANDBY.");
                    current_vcu_state = ESP_FAULT;
                }
                break;

            case ESP_CHARGE_TO_IDLE:
                request_bms_state(BMS_IDLE);

                if (current_bms_state == BMS_IDLE) {
                    current_vcu_state = ESP_CHARGE_MODE;
                }
                else if (millis() - state_entry_time > BMS_TIMEOUT_MS) {
                    Serial.println("VCU ERROR: Timeout preparando Carga.");
                    current_vcu_state = ESP_FAULT;
                }

                if (!is_charger_connected) current_vcu_state = ESP_TO_SHUTDOWN;
                break;

            case ESP_CHARGE_MODE:
                request_bms_state(BMS_HV_DC_CHARGE);

                if (!is_charger_connected) {
                    current_vcu_state = ESP_TO_SHUTDOWN;
                }
                break;

            case ESP_FAULT:
                request_bms_state(BMS_HV_SHUTDOWN);
                request_ecu_torque(0);

                if (millis() - state_entry_time > 1000) {
                    Serial.println("VCU FAULT: Bloqueo de seguridad activo. Requiere reinicio.");
                    state_entry_time = millis();
                }
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
