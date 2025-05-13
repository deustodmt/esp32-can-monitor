#ifndef SD_MANAGE_H
#define SD_MANAGE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "SD.h"
#include "SPI.h"


#define SD_MISO    2
#define SD_MOSI    15
#define SD_SCLK    14
#define SD_CS      13

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
