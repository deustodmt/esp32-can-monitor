#include "sd_manage.h"


SD_Manage::SD_Manage(xQueueHandle queue) : queue(queue)
{
    printf("*** SD card initializing\n");

    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

    if (!SD.begin(SD_CS))
    {
        Serial.println("ERROR: Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE)
    {
        Serial.println("ERROR: No SD card attached");
        const TickType_t xDelay = 500 / portTICK_PERIOD_MS;
        while (1)
            vTaskDelay(xDelay);
        return;
    }
}

void SD_Manage::write_queue_to_sd()
{
    if (this->is_mounted == false) {
        return;
    }

}
void SD_Manage::delete_sd_file()
{
}