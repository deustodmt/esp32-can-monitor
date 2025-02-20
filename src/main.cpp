#include "Arduino.h"
#include "config.h"
#include "SD_manage.h"
#include "CAN_manage.h"
#include "WiFi_manage.h"
#include <message_queue.hpp>
#include <thread.hpp>

using namespace freertos;
message_queue<int> queue;
thread SD_thread;

CAN_Manage CAN;
SD_Manage SD_manage;

void setup() {
    Serial.begin(BAUD_RATE);
    SD_manage.writeFile("/hello.txt", "Hello ");
    // queue.initialize();
    // SD_thread = thread::create([](void*){
    //     while(queue.receive()) {
    //         Serial.print("Queue received ");
    //         Serial.println(i);
    //     }
    // },nullptr,1,2000);
    // SD_thread.start();

    
    //setup_WiFi();
    //http_get_example();
    //post("TEST MESSAGE");
}

void loop() {
    CAN.poll();
    CAN.sendMessage();
}