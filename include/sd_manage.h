#ifndef SD_MANAGE_H
#define SD_MANAGE_H

#include "SD.h"
#include "SPI.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

class SD_Manage {
private:
  xQueueHandle queue;
  bool is_mounted;
  uint32_t sd_msg_count;

public:
  SD_Manage(xQueueHandle queue);
  void write_queue_to_sd();
  void delete_sd_file();
};

#endif
