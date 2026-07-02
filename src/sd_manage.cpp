#include "sd_manage.h"
#include "config.h"

SD_Manage::SD_Manage(xQueueHandle queue) : queue(queue), is_mounted(false) {
  printf("[SD] Initializing...\n");

  SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("[SD] ERROR: Card Mount Failed");
    this->is_mounted = false;
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("[SD] ERROR: No SD card attached");
    this->is_mounted = false;
    return;
  }

  this->is_mounted = true;
  printf("[SD] Card type: %s, Size: %llu MB\n",
         cardType == CARD_MMC
             ? "MMC"
             : (cardType == CARD_SD
                    ? "SD"
                    : (cardType == CARD_SDHC ? "SDHC" : "UNKNOWN")),
         (unsigned long long)SD.cardSize() / (1024 * 1024));
  printf("[SD] SD Inicializada correctamente.\n");
}

void SD_Manage::write_queue_to_sd() {
  if (!this->is_mounted)
    return;

  // Escribir solo cuando haya un bloque consistente (ej: 20 mensajes)
  if (uxQueueMessagesWaiting(this->queue) > 20) {
    sd_msg_count++;
    uint32_t queue_len = uxQueueMessagesWaiting(this->queue);
    printf("[SD] write_queue_to_sd #%lu: %lu mensajes en cola\n", sd_msg_count,
           queue_len);

    // FILE_APPEND añade al final del binario en el ESP32
    fs::File file = SD.open("/log.bin", FILE_APPEND);

    if (!file) {
      Serial.println("[SD] ERROR: Failed to open file for writing!!!");
      return;
    }

    uint8_t message[CAN_MSG_SIZE];
    int messages_written = 0;

    // Vaciamos la cola a toda velocidad
    while (xQueueReceive(this->queue, message, 0) == pdTRUE) {
      file.write(message, CAN_MSG_SIZE);
      messages_written++;
    }

    file.close(); // Cerramos y forzamos el guardado en la memoria Flash

    printf("[SD] Bloque guardado: %d tramas (total:%lu)\n", messages_written,
           sd_msg_count);
  }
}

void SD_Manage::delete_sd_file() {
  if (this->is_mounted && SD.exists("/log.bin")) {
    SD.remove("/log.bin");
    printf("Archivo log.bin eliminado.\n");
  }
}
