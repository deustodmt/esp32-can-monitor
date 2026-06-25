#include "can_commands.h"
#include "config.h"
#include "nx0002_sts01_a01.h"
#include "vcu_states.h"

#include "Arduino.h"
#include "driver/twai.h"

static uint8_t bms_msg_counter = 0;

static uint8_t calculate_crc8_j1850(uint8_t *data, size_t length) {
  uint8_t crc = 0xFF;

  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; bit++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x1D;
      } else {
        crc <<= 1;
      }
    }
  }

  return crc ^ 0xFF;
}

void request_bms_state(BMS_STATE_t desired_state) {
  struct nx0002_sts01_a01_bms_rx_ctrl_1_t tx_cmd;
  memset(&tx_cmd, 0, sizeof(tx_cmd));

  tx_cmd.app_state_req = (uint8_t)desired_state;

  tx_cmd.contactor_1_req = 0;
  tx_cmd.contactor_2_req = 0;
  tx_cmd.contactor_3_req = 0;
  tx_cmd.contactor_4_req = 0;
  tx_cmd.flt_clear = 0;
  tx_cmd.imd_en = 1;

  tx_cmd.e2_e_p1_cnt = bms_msg_counter;
  tx_cmd.e2_e_p1_zero = 0;

  tx_cmd.e2_e_p1_crc = 0x00;

  uint8_t payload[8];
  nx0002_sts01_a01_bms_rx_ctrl_1_pack(payload, &tx_cmd, 8);

  uint8_t calculated_crc = calculate_crc8_j1850(&payload[1], 7);

  tx_cmd.e2_e_p1_crc = calculated_crc;

  nx0002_sts01_a01_bms_rx_ctrl_1_pack(payload, &tx_cmd, 8);

  twai_message_t can_msg;
  can_msg.extd = 0;
  can_msg.rtr = 0;
  can_msg.identifier = BMS_RX_CTRL_1_ID;
  can_msg.data_length_code = 8;
  memcpy(can_msg.data, payload, 8);

  twai_transmit(&can_msg, pdMS_TO_TICKS(10));

  bms_msg_counter = (bms_msg_counter + 1) % 16;
}

void request_ecu_torque(int torque_value) {
  twai_message_t tx_msg;
  tx_msg.extd = 1;
  tx_msg.rtr = 0;
  tx_msg.identifier = CAN_ID_ECU_CMD;
  tx_msg.data_length_code = 8;
  memset(tx_msg.data, 0, 8);

  tx_msg.data[0] = torque_value & 0xFF;
  tx_msg.data[1] = (torque_value >> 8) & 0xFF;

  twai_transmit(&tx_msg, pdMS_TO_TICKS(10));
}
