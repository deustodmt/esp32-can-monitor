# esp32-can-monitor

Firmware para ESP32 que captura tramas del bus CAN y las almacena en tarjeta SD o las transmite en tiempo real a un servidor MQTT. Forma parte del sistema de telemetría junto con **esp32-server**.

## Descripción general

El firmware utiliza el driver nativo TWAI del ESP32 en modo escucha pasiva (listen-only) a 250 kbps. Un botón físico permite cambiar entre tres modos de operación, indicados por el LED NeoPixel. Las tramas se empaquetan en un formato binario de 20 bytes y se pueden guardar en SD o publicar vía MQTT según el modo activo.

## Hardware requerido

| Componente | Conexión |
|---|---|
| Transceptor CAN | TX→GPIO27, RX→GPIO26, SE→GPIO23 |
| Tarjeta SD (SPI) | MISO→GPIO2, MOSI→GPIO15, CLK→GPIO14, CS→GPIO13 |
| Botón | GPIO0 (pull-up interno) |
| LED NeoPixel (WS2812) | GPIO4 |
| Habilitación 5V | GPIO16 |
| RS485 (opcional) | EN→GPIO17, TX→GPIO22, RX→GPIO21, SE→GPIO19 |

## Modos de operación

El estado se cambia pulsando el botón (pulsación corta):

| Modo | LED | Descripción |
|---|---|---|
| `CAN_TO_SD` | Verde | Captura tramas CAN y las escribe en `/log.bin` en la SD |
| `DUMP_VIA_WIFI` | Azul → Naranja | Lee `/log.bin` completo y lo envía por MQTT al servidor. Borra el archivo solo si el envío fue exitoso |
| `CAN_TO_WIFI` | Magenta (pulsación larga) | Envía tramas en tiempo real por MQTT sin guardar en SD |

## Formato de datos

Cada trama CAN se empaqueta como 20 bytes. La función `packForServer()` convierte el formato interno de SD al formato de red para el servidor:

**Formato SD (interno, little-endian):**
```
Bytes  0- 3: timestamp millis (uint32, LE)
Bytes  4- 7: CAN ID           (uint32, LE)
Byte      8: DLC
Bytes  9-16: payload (8 bytes, zero-padded)
Bytes 17-19: sin uso
```

**Formato de red / MQTT (string hex de 40 caracteres, big-endian):**
```
Bytes  0- 3: CAN ID      (uint32, BE)
Bytes  4-11: timestamp   (uint64, BE — 4 bytes superiores = 0, 4 inferiores = millis)
Bytes 12-19: payload     (8 bytes, zero-padded)
```

## Configuración (`src/config.h`)

```cpp
#define WIFI_SSID     "ESP32_Net"
#define WIFI_PASS     "secreto1234"
#define MQTT_SERVER   "10.42.0.1"
#define MQTT_PORT     2000            // Puerto externo Docker de Mosquitto
#define MQTT_TOPIC    "test_topic"
#define MQTT_USER     "admin"
#define MQTT_PASSWD   "admin"
#define SERVER_URL    "http://10.42.0.1:8000"  // No utilizado actualmente
```

Ajustar la IP al host que sirva `esp32-server` (por ejemplo, la IP asignada por NetworkManager en la red compartida).

## Dependencias (gestionadas por PlatformIO)

- `Adafruit NeoPixel` — control del LED WS2812
- `OneButton` — gestión del botón (click / pulsación larga)
- `PubSubClient` — cliente MQTT
- `SD` + `SPI` — almacenamiento en tarjeta SD
- `driver/twai.h` — driver CAN nativo del ESP32 (parte del framework Arduino-ESP32)

## Compilar y flashear

```bash
# Con PlatformIO CLI
pio run --target upload

# O desde VS Code con la extensión PlatformIO
```

La plataforma objetivo es `esp32dev` (ESP32 genérico). Ver `platformio.ini` para detalles.

## Tareas FreeRTOS

| Tarea | Core | Prioridad | Función |
|---|---|---|---|
| `CAN_Read` | 0 | 5 | Lee tramas del bus CAN y las encola |
| `SD_Write` | 1 | 3 | Escribe la cola en SD o realiza el volcado MQTT |
| `WiFi_Pub` | 1 | 2 | Mantiene la conexión MQTT y publica tramas en tiempo real |

## Relación con esp32-server

Este firmware publica mensajes MQTT en el topic `test_topic`. El servidor Python de `esp32-server` los suscribe, decodifica el payload hex y almacena los datos en InfluxDB. Ver el repositorio [esp32-server](../esp32-server) para el stack de servidor.
