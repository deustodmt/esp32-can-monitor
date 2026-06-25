#ifndef SD_MANAGE_H
#define SD_MANAGE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "SD.h"
#include "SPI.h"

class SD_Manage
{
private:
    xQueueHandle queue;
    bool is_mounted;

public:
    SD_Manage(xQueueHandle queue);
    void write_queue_to_sd();
    void delete_sd_file();
};

#endif
