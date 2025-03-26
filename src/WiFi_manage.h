#ifndef __WIFI_MANAGE_H__
#define __WIFI_MANAGE_H__

#include <WiFi.h>

void setup_WiFi(void);
void http_get_example(void);
void post(String);

class WiFi_Manage {
    private:
        WiFiClient wifiClient;
    public:
        WiFi_Manage();
        void setup();
        void get_example();
        void post(String);
        void mqtt_test(void);
};

#endif
