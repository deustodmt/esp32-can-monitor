#include "sd_manage.h"
#include "config.h"

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
    if (this->is_mounted == false)
    {
        return;
    }

    if (uxQueueMessagesWaiting(this->queue) > 20)
    {
        uint8_t message[CAN_MSG_SIZE];
        fs::File file;
        file = SD.open("/log.bin", "ab");

        while (xQueueReceive(this->queue, &message, 0) == pdTRUE)
        { // Writes a binary file with CAN messages
            if (file == NULL)
            {
                Serial.println("Failed to open file for writing!!!\n");
                return;
            }
            if (file.write(message, CAN_MSG_SIZE))
            {
                printf("Message written to SD\n");
            }
            else
                printf("Append failed\n");
            file.flush();
        }
        file.close();
    }
    else
    {
        printf("less than 20 messages on queue, waiting\n");
    }
}
void SD_Manage::delete_sd_file()
{
}