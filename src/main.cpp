#include "Arduino.h"
#include "config.h"
#include "SD_manage.h"
#include "CAN_manage.h"


void setup() {
    setup_CAN();
    setup_SD();
    
}

void loop() {
    poll_CAN();
}