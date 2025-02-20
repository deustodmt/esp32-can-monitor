#include "Arduino.h"
#include "config.h"
#include "SD_manage.h"
#include "CAN_manage.h"
#include "WiFi_manage.h"


void setup() {
    setup_SD();
    setup_CAN();
    //setup_WiFi();
    //http_get_example();
    //post("TEST MESSAGE");
}

void loop() {
    poll_CAN();
}