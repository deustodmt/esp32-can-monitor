#ifndef VCU_STATES_H
#define VCU_STATES_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"

// Estados validados físicamente y mediante el diagrama de la BMS
typedef enum {
    BMS_INIT                   = 1,
    BMS_POST                   = 2,
    BMS_STANDBY                = 3,
    BMS_IDLE_PRECHARGE         = 4,
    BMS_IDLE                   = 5,
    BMS_HV_READY_PRECHARGE     = 6,
    BMS_HV_READY               = 7,
    BMS_HV_AC_CHARGE_PRECHARGE = 8,
    BMS_HV_AC_CHARGE           = 9,
    BMS_HV_DC_CHARGE           = 10,
    BMS_HV_SHUTDOWN            = 11,
    BMS_SLEEP                  = 12,
    BMS_SOFT_FAULT             = 13,
    BMS_HARD_FAULT             = 14
} BMS_STATE_t;

typedef enum {
    ESP_INIT,
    ESP_STANDBY,
    ESP_TO_IDLE,
    ESP_TO_HV_READY,
    ESP_DRIVE,
    ESP_EMERGENCY_TORQUE_CUT,
    ESP_TO_SHUTDOWN,
    ESP_SHUTDOWN_TO_IDLE,
    ESP_SHUTDOWN_TO_STANDBY,
    ESP_CHARGE_TO_IDLE,
    ESP_CHARGE_MODE,
    ESP_FAULT
} ESP_STATE_t;

typedef enum {
    CAN_TO_SD,
    CAN_TO_WIFI,
    DUMP_VIA_WIFI,
} LOG_STATE_t;

/* Global variables — declared in main.cpp, referenced by other modules */
extern volatile uint32_t last_bms_msg_time;
extern LOG_STATE_t       current_state;
extern ESP_STATE_t       current_vcu_state;
extern BMS_STATE_t       current_bms_state;

#endif
