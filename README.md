# esp32-can-monitor

Firmware para ESP32 que captura tramas del bus CAN, las almacena en tarjeta SD o las transmite en tiempo real a un servidor MQTT. Incluye un VCU (Vehicle Control Unit) que gestiona la máquina de estados de la BMS (Battery Management System) y envía comandos a la ECU del motor. Forma parte del sistema de telemetría junto con **esp32-server**.

## Descripción general

El firmware utiliza el driver nativo TWAI del ESP32 en modo normal (TWAI_MODE_NORMAL) a 500 kbps. Un botón físico permite cambiar entre tres modos de operación, indicados por el LED NeoPixel. Las tramas se empaquetan en un formato binario de 20 bytes y se pueden guardar en SD o publicar vía MQTT según el modo activo.

Además del datalogging, el firmware implementa una máquina de estados del VCU que interactúa con la BMS: envía comandos de control a la BMS y recibe su estado, gestiona precargas, transiciones de estado HV (High Voltage), y controla el par de la ECU del motor.

## Hardware requerido

| Componente | Conexión |
|---|---|
| Transceptor CAN (TJA1051T) | TX→GPIO27, RX→GPIO26, SE→GPIO23 (500 kbps) |
| Tarjeta SD (SPI) | MISO→GPIO2, MOSI→GPIO15, SCLK→GPIO14, CS→GPIO13 |
| Botón (contacto HV) | GPIO0 (pull-up interno) |
| LED NeoPixel (WS2812) | GPIO4 |
| Detección de carga | GPIO5 (pull-down) |
| Contacto HV | GPIO18 (pull-down) |

## Modos de operación

El estado se cambia pulsando el botón (pulsación corta):

| Modo | LED | Descripción |
|---|---|---|
| `CAN_TO_SD` | Verde | Captura tramas CAN y las escribe en `/log.bin` en la SD |
| `DUMP_VIA_WIFI` | Rojo | Lee `/log.bin` completo y lo envía por MQTT al servidor. Borra el archivo solo si el envío fue exitoso |

| `CAN_TO_WIFI` | Magenta | Envía tramas en tiempo real por MQTT sin guardar en SD |
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

## Configuración (`include/config.h`)

```cpp
#define BAUD_RATE 9600

#define BMS_TIMEOUT_MS 2500
#define BMS_CAN_TIMEOUT_MS 2500

#define CAN_MSG_SIZE 20

#define WIFI_SSID   "ESP32_Net"
#define WIFI_PASS   "secreto1234"

#define MQTT_SERVER "10.42.0.1"
#define MQTT_PORT   1883
#define MQTT_USER   "admin"
#define MQTT_PASSWD "admin"
#define MQTT_TOPIC  "test_topic"
```

Ajustar la IP al host que sirva `esp32-server` (por ejemplo, la IP asignada por NetworkManager en la red compartida).

## CAN ID defines

| CAN ID | Descripción |
|---|---|
| `0x462` | BMS_TX_STATE_3 — Estado actual de la BMS (interceptado automáticamente) |
| `0x400` | BMS_TX_CMD_1 — Datos de celdas (18 celdas + 5 temperaturas) |
| `0x401`-`0x413` | BMS_TX_CMD_2 a BMS_TX_CMD_20 — Datos de celdas adicionales |
| `0x360` | BMS_RX_CTRL_1 — Control BMS (enviados por el VCU) |
| `0x460` | BMS_TX_STATE_1 — Uptime, CPU load |
| `0x461` | BMS_TX_STATE_2 — DEM Code, DEM Present |
| `0x463` | BMS_TX_STATE_4 — Voltaje sintetizado, corriente, potencia |
| `0x464` | BMS_TX_STATE_5 — Voltaje de celdas individuales |
| `0x465` | BMS_TX_STATE_6 — Voltaje mín/máx/promedio de celdas |
| `0x466` | BMS_TX_STATE_7 — Temperaturas (4 sensores) |
| `0x467` | BMS_TX_STATE_8 — Diagnóstico de celdas |
| `0x468` | BMS_TX_STATE_9 — Temperatura promedio de celdas |
| `0x469` | BMS_TX_STATE_10 — IMD Res Pos/Neg |
| `0x470` | BMS_TX_STATE_11 — SOC, SOH |
| `0x0C00000A` | ECU_CMD — Comando de par al motor (exterior, 8 bytes, extended frame) |

## VCU / Máquina de estados del VCU

El firmware incluye una máquina de estados del VCU (Vehicle Control Unit) que gestiona la interacción con la BMS y la ECU del motor:

**Estados del VCU:**
- `ESP_INIT` → `ESP_STANDBY` → `ESP_TO_IDLE` → `ESP_TO_HV_READY` → `ESP_DRIVE`
- `ESP_EMERGENCY_TORQUE_CUT` (par de emergencia a 0)
- `ESP_TO_SHUTDOWN` → `ESP_SHUTDOWN_TO_IDLE` / `ESP_SHUTDOWN_TO_STANDBY`
- `ESP_CHARGE_TO_IDLE` / `ESP_CHARGE_MODE` (modo carga)
- `ESP_FAULT` (estado de fallo — requiere reinicio)

**Estados de la BMS (BMS_STATE_t):**
- `BMS_INIT`, `BMS_POST`, `BMS_STANDBY`, `BMS_IDLE_PRECHARGE`, `BMS_IDLE`
- `BMS_HV_READY_PRECHARGE`, `BMS_HV_READY`
- `BMS_HV_AC_CHARGE_PRECHARGE`, `BMS_HV_AC_CHARGE`, `BMS_HV_DC_CHARGE`
- `BMS_HV_SHUTDOWN`, `BMS_SLEEP`, `BMS_SOFT_FAULT`, `BMS_HARD_FAULT`

**Evolución del estado:**
- En `ESP_INIT`: estado inicial — inicia transición a `ESP_STANDBY`
- En `ESP_STANDBY`: envía `BMS_STANDBY` a la BMS. Si hay cargador conectado → `ESP_CHARGE_TO_IDLE`, si hay contacto HV → `ESP_TO_IDLE`
- En `ESP_TO_IDLE`: envía `BMS_IDLE`, espera confirmación de la BMS (timeout: 2s)
- En `ESP_TO_HV_READY`: envía `BMS_HV_READY_PRECHARGE`, espera `BMS_HV_READY` (timeout: 4s por precarga física)
- En `ESP_DRIVE`: envía `BMS_HV_READY`. Si se desconecta HV → `ESP_TO_SHUTDOWN`, si aparece cargador → `ESP_EMERGENCY_TORQUE_CUT`
- En `ESP_FAULT`: envía `BMS_HV_SHUTDOWN` de forma continua, requiere reinicio para recuperarse

**Watchdog CAN:** Si se pierde la señal CAN de la BMS por más de `BMS_CAN_TIMEOUT_MS` (2.5s), el VCU entra en `ESP_FAULT` y corta el par de la ECU si estaba en marcha. El timeout se imprime en ms con el tag `[VCU] ERROR CRÍTICO`.

**Comandos a la BMS:** Las tramas de control BMS (`0x360`) incluyen un contador E2E de 4 bits (0-15), CRC-8 SAE-J1850 calculado sobre los bytes 1-7, y campos de control de contactores (4 contactores), IMD, y limpieza de fallos. Cada envío se loguea con `[BMS_CMD]`.

**Torque a ECU:** El torque se envía como 2 bytes little-endian en un frame extendido. Cada envío se confirma con `[ECU_TORQUE] TX OK` o `[ECU_TORQUE] TX FAILED`.

**Tramas CAN recibidas:** Cada trama se loguea con `[CAN] RX` incluyendo timestamp, ID, DLC y payload. Las tramas de `BMS_TX_STATE_3` (0x462) se decodifican y muestran el estado de la BMS, DIOs y HVIL.

## Configuración de BMS

```cpp
#define BMS_TIMEOUT_MS 2500        // Timeout para transiciones de estado BMS
#define BMS_CAN_TIMEOUT_MS 2500    // Timeout del watchdog CAN (pérdida de señal BMS)
```

## Dependencias (gestionadas por PlatformIO)

- `adafruit/Adafruit NeoPixel` — control del LED WS2812
- `mathertel/OneButton` — gestión del botón (click / pulsación larga)
- `knolleary/PubSubClient` — cliente MQTT
- `SD` + `SPI` — almacenamiento en tarjeta SD
- `driver/twai.h` — driver TWAI CAN nativo del ESP32 (parte del framework Arduino-ESP32)
- `cantools` — header generado (incluido en `include/nx0002_sts01_a01.h`) para desempaquetado de tramas BMS

## Depuración

El firmware incluye prints de depuración extensivos en todas las tareas. Cada mensaje usa un tag de prefijo para identificar el origen:

| Tag | Origen |
|---|---|
| `[MAIN]` | Inicio, heap, heartbeat |
| `[CAN]` | Recepción de tramas CAN |
| `[BMS_CMD]` | Envío de comandos a la BMS |
| `[ECU_TORQUE]` | Envío de torque a la ECU |
| `[VCU]` | Transiciones de estado del VCU |
| `[WIFI]` | Conexión WiFi |
| `[MQTT]` | Conexión y publicación MQTT |
| `[SD]` | Inicialización y escritura en SD |
| `[DUMP]` | Volcado SD→MQTT |
| `[BTN]` | Pulsaciones del botón |

**Heartbeat:** Cada 5 segundos, `loop()` imprime un heartbeat con: conteo, heap libre, estado actual, estado VCU, estado BMS, y longitudes de las colas CAN y WiFi.

**Ejemplo de salida de heartbeat:**
```
[MAIN] Heartbeat #1: FreeHeap=131072, State=0, VCU=1, BMS=3, CAN_Q=45, WiFi_Q=0
```

**Ejemplo de inicialización de SD:**
```
[SD] Initializing...
[SD] Card type: SD, Size: 7489 MB
[SD] SD Inicializada correctamente.
```

La plataforma objetivo es `esp32dev`. Ver `platformio.ini` para detalles.

## Tareas FreeRTOS

| Tarea | Core | Prioridad | Función |
|---|---|---|---|
| `CAN_Read` | 0 | 5 | Lee tramas del bus CAN y las encola en CAN_QUEUE o WIFI_QUEUE |
| `VCU_State` | 1 | 4 | Máquina de estados del VCU — gestiona BMS y ECU |
| `SD_Write` | 1 | 3 | Escribe la cola en SD o realiza el volcado MQTT según estado |
| `WiFi_Pub` | 1 | 2 | Mantiene la conexión MQTT y publica tramas en tiempo real |

## Relación con esp32-server

Este firmware publica mensajes MQTT en el topic `test_topic`. El servidor Python de `esp32-server` los suscribe, decodifica el payload hex y almacena los datos en InfluxDB. Ver el repositorio [esp32-server](../esp32-server) para el stack de servidor.
