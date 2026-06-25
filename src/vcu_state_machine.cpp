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
    static uint32_t state_transition_count = 0;

    for (;;) {
        bool is_charger_connected = (digitalRead(CHARGE_DETECT_PIN) == HIGH);
        bool is_hv_switch_on = (digitalRead(HV_SWITCH_PIN) == LOW);

        if (current_vcu_state != prev_vcu_state) {
            state_entry_time = millis();
            prev_vcu_state = current_vcu_state;
            state_transition_count++;
            printf("[VCU] TRANSITION %d: %d -> %d\n", state_transition_count, prev_vcu_state, current_vcu_state);
        }

        if (current_vcu_state != ESP_INIT && current_vcu_state != ESP_FAULT) {
            if (millis() - last_bms_msg_time > BMS_CAN_TIMEOUT_MS) {
                printf("[VCU] ERROR CRÍTICO: ¡Señal CAN de la BMS perdida! (%lu ms sin mensaje)\n", millis() - last_bms_msg_time);

                if (current_vcu_state == ESP_DRIVE ||
                    current_vcu_state == ESP_TO_HV_READY ||
                    current_vcu_state == ESP_EMERGENCY_TORQUE_CUT) {

                    request_ecu_torque(0);
                    printf("[VCU] Cortando tracción de emergencia (Torque = 0)\n");
                }

                current_vcu_state = ESP_FAULT;
            }
        }

        switch (current_vcu_state) {

            case ESP_INIT:
                printf("[VCU] INIT: Iniciando sistema...\n");
                current_vcu_state = ESP_STANDBY;
                break;

            case ESP_STANDBY:
                printf("[VCU] STANDBY: charger=%d, hv_switch=%d\n", is_charger_connected, is_hv_switch_on);
                request_bms_state(BMS_STANDBY);
                if (is_charger_connected) {
                    printf("[VCU] STANDBY -> CHARGE_TO_IDLE (charger detected)\n");
                    current_vcu_state = ESP_CHARGE_TO_IDLE;
                } else if (is_hv_switch_on) {
                    printf("[VCU] STANDBY -> TO_IDLE (HV switch on)\n");
                    current_vcu_state = ESP_TO_IDLE;
                }
                break;

            case ESP_TO_IDLE:
                printf("[VCU] TO_IDLE: BMS state=%d, timeout=%lu ms\n", current_bms_state, millis() - state_entry_time);
                request_bms_state(BMS_IDLE);

                if (current_bms_state == BMS_IDLE) {
                    printf("[VCU] TO_IDLE -> TO_HV_READY\n");
                    current_vcu_state = ESP_TO_HV_READY;
                }
                else if (millis() - state_entry_time > BMS_TIMEOUT_MS) {
                    printf("[VCU] ERROR: Timeout esperando IDLE de la BMS\n");
                    current_vcu_state = ESP_FAULT;
                }

                if (!is_hv_switch_on) {
                    printf("[VCU] TO_IDLE -> TO_SHUTDOWN (HV switch off)\n");
                    current_vcu_state = ESP_TO_SHUTDOWN;
                }
                break;

            case ESP_TO_HV_READY:
                printf("[VCU] TO_HV_READY: BMS state=%d, timeout=%lu ms\n", current_bms_state, millis() - state_entry_time);
                request_bms_state(BMS_HV_READY_PRECHARGE);

                if (current_bms_state == BMS_HV_READY) {
                    printf("[VCU] TO_HV_READY -> DRIVE (BMS HV READY)\n");
                    printf("[VCU] BMS en HV READY. ¡MOTO LISTA!\n");
                    current_vcu_state = ESP_DRIVE;
                }
                else if (millis() - state_entry_time > BMS_TIMEOUT_MS * 2) {
                    printf("[VCU] ERROR: Timeout en Precarga/HV Ready\n");
                    current_vcu_state = ESP_FAULT;
                }

                if (!is_hv_switch_on) {
                    printf("[VCU] TO_HV_READY -> TO_SHUTDOWN (HV switch off)\n");
                    current_vcu_state = ESP_TO_SHUTDOWN;
                }
                break;

            case ESP_DRIVE:
                printf("[VCU] DRIVE: charger=%d, hv_switch=%d\n", is_charger_connected, is_hv_switch_on);
                request_bms_state(BMS_HV_READY);

                if (!is_hv_switch_on) {
                    printf("[VCU] DRIVE -> TO_SHUTDOWN (HV switch off)\n");
                    current_vcu_state = ESP_TO_SHUTDOWN;
                } else if (is_charger_connected) {
                    printf("[VCU] DRIVE -> EMERGENCY_TORQUE_CUT (charger connected)\n");
                    current_vcu_state = ESP_EMERGENCY_TORQUE_CUT;
                }
                break;

            case ESP_EMERGENCY_TORQUE_CUT:
                printf("[VCU] EMERGENCY_TORQUE_CUT: timeout=%lu ms\n", millis() - state_entry_time);
                request_ecu_torque(0);

                if (millis() - state_entry_time > 50) {
                    printf("[VCU] EMERGENCY_TORQUE_CUT -> TO_SHUTDOWN\n");
                    current_vcu_state = ESP_TO_SHUTDOWN;
                }
                break;

            case ESP_TO_SHUTDOWN:
                printf("[VCU] TO_SHUTDOWN: BMS state=%d, timeout=%lu ms\n", current_bms_state, millis() - state_entry_time);
                request_bms_state(BMS_HV_SHUTDOWN);

                if (current_bms_state == BMS_HV_SHUTDOWN) {
                    printf("[VCU] TO_SHUTDOWN -> SHUTDOWN_TO_IDLE\n");
                    current_vcu_state = ESP_SHUTDOWN_TO_IDLE;
                }
                else if (millis() - state_entry_time > BMS_TIMEOUT_MS) {
                    printf("[VCU] ERROR: Timeout forzando SHUTDOWN\n");
                    current_vcu_state = ESP_FAULT;
                }
                break;

            case ESP_SHUTDOWN_TO_IDLE:
                printf("[VCU] SHUTDOWN_TO_IDLE: BMS state=%d, timeout=%lu ms, charger=%d\n", current_bms_state, millis() - state_entry_time, is_charger_connected);
                request_bms_state(BMS_IDLE);

                if (current_bms_state == BMS_IDLE) {
                    if (is_charger_connected) {
                        printf("[VCU] SHUTDOWN_TO_IDLE -> CHARGE_MODE\n");
                        current_vcu_state = ESP_CHARGE_MODE;
                    } else {
                        printf("[VCU] SHUTDOWN_TO_IDLE -> SHUTDOWN_TO_STANDBY\n");
                        current_vcu_state = ESP_SHUTDOWN_TO_STANDBY;
                    }
                }
                else if (millis() - state_entry_time > BMS_TIMEOUT_MS) {
                    printf("[VCU] ERROR: Timeout bajando a IDLE\n");
                    current_vcu_state = ESP_FAULT;
                }
                break;

            case ESP_SHUTDOWN_TO_STANDBY:
                printf("[VCU] SHUTDOWN_TO_STANDBY: BMS state=%d, timeout=%lu ms\n", current_bms_state, millis() - state_entry_time);
                request_bms_state(BMS_STANDBY);

                if (current_bms_state == BMS_STANDBY) {
                    printf("[VCU] SHUTDOWN_TO_STANDBY -> STANDBY\n");
                    current_vcu_state = ESP_STANDBY;
                }
                else if (millis() - state_entry_time > BMS_TIMEOUT_MS) {
                    printf("[VCU] ERROR: Timeout bajando a STANDBY\n");
                    current_vcu_state = ESP_FAULT;
                }
                break;

            case ESP_CHARGE_TO_IDLE:
                printf("[VCU] CHARGE_TO_IDLE: BMS state=%d, timeout=%lu ms\n", current_bms_state, millis() - state_entry_time);
                request_bms_state(BMS_IDLE);

                if (current_bms_state == BMS_IDLE) {
                    printf("[VCU] CHARGE_TO_IDLE -> CHARGE_MODE\n");
                    current_vcu_state = ESP_CHARGE_MODE;
                }
                else if (millis() - state_entry_time > BMS_TIMEOUT_MS) {
                    printf("[VCU] ERROR: Timeout preparando Carga\n");
                    current_vcu_state = ESP_FAULT;
                }

                if (!is_charger_connected) {
                    printf("[VCU] CHARGE_TO_IDLE -> TO_SHUTDOWN (charger disconnected)\n");
                    current_vcu_state = ESP_TO_SHUTDOWN;
                }
                break;

            case ESP_CHARGE_MODE:
                printf("[VCU] CHARGE_MODE: charger=%d\n", is_charger_connected);
                request_bms_state(BMS_HV_DC_CHARGE);

                if (!is_charger_connected) {
                    printf("[VCU] CHARGE_MODE -> TO_SHUTDOWN (charger disconnected)\n");
                    current_vcu_state = ESP_TO_SHUTDOWN;
                }
                break;

            case ESP_FAULT:
                printf("[VCU] FAULT: timeout=%lu ms\n", millis() - state_entry_time);
                request_bms_state(BMS_HV_SHUTDOWN);
                request_ecu_torque(0);

                if (millis() - state_entry_time > 1000) {
                    printf("[VCU] FAULT: Bloqueo de seguridad activo. Requiere reinicio\n");
                    state_entry_time = millis();
                }
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
