#ifndef CAN_COMMANDS_H
#define CAN_COMMANDS_H

#include "vcu_states.h"

void request_bms_state(BMS_STATE_t desired_state);
void request_ecu_torque(int torque_value);

#endif
