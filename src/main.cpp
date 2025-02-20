#include "Arduino.h"
#include "config.h"
#include "SD_manage.h"
#include "CAN_manage.h"
#include "WiFi_manage.h"
#include <message_queue.hpp>
#include <thread.hpp>
#include <Adafruit_NeoPixel.h>

using namespace freertos;
message_queue<uint8_t[]> queue;
thread SD_thread;

CAN_Manage CAN;
SD_Manage SD_manage;

Adafruit_NeoPixel strip(1, 4, NEO_GRB + NEO_KHZ800);


void setup() {
    strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
    strip.show();            // Turn OFF all pixels ASAP
    strip.setBrightness(255); // Set BRIGHTNESS to about 1/5 (max = 255)
    strip.setPixelColor(0, strip.Color(255, 0, 0));  
    strip.show();
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