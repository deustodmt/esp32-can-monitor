#include "Arduino.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>
#include <OneButton.h>
#include "driver/twai.h"
#include "sd_manage.h"
// [ADDED] WiFi + MQTT
#include <WiFi.h>
#include <PubSubClient.h>

typedef enum
{
    CAN_TO_SD,
    CAN_TO_WIFI,
    DUMP_VIA_WIFI,
} STATES;

STATES current_state = CAN_TO_SD;

Adafruit_NeoPixel strip(1, WS2812_PIN, NEO_GRB + NEO_KHZ800);
OneButton button;

xQueueHandle can_queue;
// [ADDED] Separate queue for WiFi publishing (capacity: 200 messages)
xQueueHandle wifi_queue;

SD_Manage *sdManager;

// [ADDED] MQTT client globals
WiFiClient    wifiClient;
PubSubClient  mqttClient(wifiClient);

// Declaración de las tareas de FreeRTOS
void canReadTask(void *pvParameters);
void sdWriteTask(void *pvParameters);
// [ADDED]
void wifiPublishTask(void *pvParameters);

// ---------------------------------------------------------------------------
// [ADDED] packForServer()
//
// Converts a 20-byte SD-format message into the 40-char hex string the server
// expects. Reformats from:
//   raw_msg[ 0- 3] = timestamp_ms  (uint32, little-endian)
//   raw_msg[ 4- 7] = CAN ID        (uint32, little-endian)
//   raw_msg[   8 ] = DLC
//   raw_msg[ 9-16] = payload bytes (up to 8 bytes, zero-padded)
//   raw_msg[17-19] = unused
//
// To the server's 20-byte binary layout (transmitted as hex string):
//   bytes[ 0- 3] = CAN ID    (uint32, big-endian)
//   bytes[ 4-11] = timestamp (uint64, big-endian) — upper 4B = 0, lower 4B = ts_ms
//   bytes[12-19] = payload   (8 bytes, zero-padded)
// ---------------------------------------------------------------------------
static void packForServer(const uint8_t *raw_msg, char *hex_out)
{
    uint32_t ts_ms, can_id;
    memcpy(&ts_ms,  &raw_msg[0], 4);   // little-endian on ESP32
    memcpy(&can_id, &raw_msg[4], 4);

    uint8_t server_msg[20] = {0};

    // CAN ID — big-endian
    server_msg[0] = (can_id >> 24) & 0xFF;
    server_msg[1] = (can_id >> 16) & 0xFF;
    server_msg[2] = (can_id >>  8) & 0xFF;
    server_msg[3] =  can_id        & 0xFF;

    // Timestamp as uint64 big-endian (upper 32 bits = 0, lower 32 = ts_ms)
    server_msg[4]  = 0;
    server_msg[5]  = 0;
    server_msg[6]  = 0;
    server_msg[7]  = 0;
    server_msg[8]  = (ts_ms >> 24) & 0xFF;
    server_msg[9]  = (ts_ms >> 16) & 0xFF;
    server_msg[10] = (ts_ms >>  8) & 0xFF;
    server_msg[11] =  ts_ms        & 0xFF;

    // Payload (raw_msg bytes 9-16 → server bytes 12-19)
    memcpy(&server_msg[12], &raw_msg[9], 8);

    // Hex-encode
    for (int i = 0; i < 20; i++)
        sprintf(&hex_out[i * 2], "%02X", server_msg[i]);
    hex_out[40] = '\0';
}

// ---------------------------------------------------------------------------
// LED / button helpers (unchanged)
// ---------------------------------------------------------------------------
void setup_LED()
{
    strip.begin();
    strip.setBrightness(255);
    strip.setPixelColor(0, strip.Color(0, 255, 0)); // Verde = Guardando
    strip.show();
}

void setup_button()
{
    button.setup(BUTTON_PIN, INPUT_PULLUP, true);

    button.attachClick([]{
        printf("Botón pulsado ----- \n");
        if (current_state == CAN_TO_SD) {
            printf("--- Cambiando a modo WiFI-DUMP!\n");
            current_state = DUMP_VIA_WIFI;
            strip.setPixelColor(0, strip.Color(0, 0, 255)); // Azul = WiFi
        } else if (current_state == DUMP_VIA_WIFI) {
            printf("--- Cambiando a modo CAN a SD!\n");
            current_state = CAN_TO_SD;
            strip.setPixelColor(0, strip.Color(0, 255, 0)); // Verde = Guardando
        }
        strip.show();
    });

    button.attachLongPressStop([]{
        printf("Botón pulsado largo -----\n");
        strip.setPixelColor(0, strip.Color(255, 0, 255)); // Magenta
        strip.show();
    });
}

void setup_CAN()
{
    printf("*** Inicializando bus CAN (TWAI)...\n");
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_LISTEN_ONLY);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
        printf("Driver CAN instalado.\n");
    else {
        printf("ERROR: Fallo al instalar CAN.\n");
        return;
    }
    if (twai_start() == ESP_OK)
        printf("Bus CAN Iniciado con éxito.\n");
}

// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(BAUD_RATE);

    setup_button();
    setup_LED();

    // 1. Crear las colas
    can_queue  = xQueueCreate(200, CAN_MSG_SIZE);
    wifi_queue = xQueueCreate(200, CAN_MSG_SIZE); // [ADDED]
    if (can_queue == NULL || wifi_queue == NULL) {
        printf("ERROR: No se pudo crear la cola FreeRTOS\n");
        return;
    }

    // 2. Inicializar SD
    sdManager = new SD_Manage(can_queue);

    // 3. Inicializar hardware CAN
    setup_CAN();

    // 4. Arrancar las tareas
    xTaskCreatePinnedToCore(canReadTask,      "CAN_Read",    4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(sdWriteTask,      "SD_Write",    8192, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(wifiPublishTask,  "WiFi_Pub",    8192, NULL, 2, NULL, 1); // [ADDED]
}

void loop()
{
    button.tick();
    vTaskDelay(pdMS_TO_TICKS(10));
}

// ---------------------------------------------------------------------------
// canReadTask — routes frames to the right queue depending on state
// ---------------------------------------------------------------------------
void canReadTask(void *pvParameters)
{
    twai_message_t message;
    uint8_t raw_msg[CAN_MSG_SIZE];

    for (;;)
    {
        if (current_state == CAN_TO_SD || current_state == CAN_TO_WIFI) // [MODIFIED]
        {
            if (twai_receive(&message, pdMS_TO_TICKS(100)) == ESP_OK)
            {
                uint32_t timestamp = millis();
                memset(raw_msg, 0, CAN_MSG_SIZE);

                memcpy(&raw_msg[0], &timestamp,            4);
                memcpy(&raw_msg[4], &message.identifier,   4);
                raw_msg[8] = message.data_length_code;
                memcpy(&raw_msg[9], message.data, message.data_length_code);

                // [MODIFIED] Route to the right queue
                if (current_state == CAN_TO_SD)
                    xQueueSend(can_queue,  raw_msg, 0);
                else
                    xQueueSend(wifi_queue, raw_msg, 0); // [ADDED]

                printf("ID: 0x%03X | DLC: %d | Data: ",
                       message.identifier, message.data_length_code);
                for (int i = 0; i < message.data_length_code; i++)
                    printf("%02X ", message.data[i]);
                printf("\n");
            }
        }
        else // DUMP_VIA_WIFI — CAN sniffing paused while dumping
        {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

// ---------------------------------------------------------------------------
// sdWriteTask — SD writing + DUMP_VIA_WIFI SD→MQTT
// ---------------------------------------------------------------------------
void sdWriteTask(void *pvParameters)
{
    for (;;)
    {
        if (current_state == CAN_TO_SD)
        {
            sdManager->write_queue_to_sd();
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        else if (current_state == DUMP_VIA_WIFI) // SD dump over MQTT
        {
            // Wait until MQTT is connected before dumping
            if (!mqttClient.connected()) {
                printf("DUMP_VIA_WIFI: esperando conexión MQTT...\n");
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            printf("DUMP_VIA_WIFI: iniciando volcado de SD...\n");
            strip.setPixelColor(0, strip.Color(255, 165, 0)); // Naranja = volcando
            strip.show();

            fs::File file = SD.open("/log.bin", FILE_READ);
            if (!file) {
                printf("DUMP_VIA_WIFI: no se pudo abrir log.bin\n");
                vTaskDelay(pdMS_TO_TICKS(2000));
                // Volvemos al estado normal si no hay archivo
                current_state = CAN_TO_SD;
                strip.setPixelColor(0, strip.Color(0, 255, 0));
                strip.show();
                continue;
            }

            uint8_t raw_msg[CAN_MSG_SIZE];
            char    hex_payload[41];
            int     count = 0;
            
            // [NUEVO] Bandera para controlar si todo salió bien
            bool    all_sent_ok = true; 

            while (file.available() >= CAN_MSG_SIZE)
            {
                // 1. Comprobar que no hemos perdido la conexión a medio camino
                if (!mqttClient.connected()) {
                    printf("DUMP_VIA_WIFI: ERROR - Conexión MQTT perdida. Abortando...\n");
                    all_sent_ok = false;
                    break; 
                }

                file.read(raw_msg, CAN_MSG_SIZE);
                packForServer(raw_msg, hex_payload);
                
                // 2. Comprobar que el publish se ejecuta correctamente
                if (!mqttClient.publish(MQTT_TOPIC, hex_payload)) {
                    printf("DUMP_VIA_WIFI: ERROR - Fallo al publicar el mensaje %d. Abortando...\n", count);
                    all_sent_ok = false;
                    break;
                }

                mqttClient.loop();
                count++;
                
                // Brief yield every 50 messages to avoid WDT
                if (count % 50 == 0)
                    vTaskDelay(pdMS_TO_TICKS(10));
            }

            file.close();

            // [NUEVO] Lógica de borrado seguro
            if (all_sent_ok && count > 0) {
                printf("DUMP_VIA_WIFI: Volcado completo con éxito — %d tramas enviadas.\n", count);
                sdManager->delete_sd_file(); // Borramos el archivo
            } else if (!all_sent_ok) {
                printf("DUMP_VIA_WIFI: Hubo errores. NO se ha borrado log.bin para evitar pérdida de datos.\n");
            } else {
                printf("DUMP_VIA_WIFI: El archivo estaba vacío.\n");
            }

            // Return to CAN_TO_SD and restore LED
            current_state = CAN_TO_SD;
            strip.setPixelColor(0, strip.Color(0, 255, 0));
            strip.show();
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

// ---------------------------------------------------------------------------
// [ADDED] wifiPublishTask — manages WiFi + MQTT and drains wifi_queue
// ---------------------------------------------------------------------------
void wifiPublishTask(void *pvParameters)
{
    // --- Connect to WiFi (blocking until connected) ---
    printf("WiFi: conectando a %s...\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED)
        vTaskDelay(pdMS_TO_TICKS(500));
    printf("WiFi conectado: %s\n", WiFi.localIP().toString().c_str());

    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

    uint8_t raw_msg[CAN_MSG_SIZE];
    char    hex_payload[41];

    for (;;)
    {
        // --- Reconnect MQTT if dropped ---
        if (!mqttClient.connected())
        {
            printf("MQTT: conectando a %s:%d...\n", MQTT_SERVER, MQTT_PORT);
            // mosquitto is configured allow_anonymous, credentials are optional
            // but we send them anyway for compatibility with auth-enabled setups
            if (mqttClient.connect("ESP32Monitor", MQTT_USER, MQTT_PASSWD))
                printf("MQTT: conectado\n");
            else {
                printf("MQTT: fallo (estado %d), reintentando en 5s\n",
                       mqttClient.state());
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
        }

        mqttClient.loop();

        // --- Publish all queued CAN frames when in CAN_TO_WIFI mode ---
        if (current_state == CAN_TO_WIFI)
        {
            while (xQueueReceive(wifi_queue, raw_msg, 0) == pdTRUE)
            {
                packForServer(raw_msg, hex_payload);
                if (!mqttClient.publish(MQTT_TOPIC, hex_payload))
                    printf("MQTT: publish falló (buffer lleno?)\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
