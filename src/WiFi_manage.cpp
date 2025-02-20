#include "WiFi_manage.h"
#include "config.h"
#include <Arduino.h>
#include <HTTPClient.h>

WiFiMulti wifiMulti;

WiFi_Manage::WiFi_Manage() {
  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  wifiMulti.addAP(WIFI_SSID, WIFI_PASS);
}

void WiFi_Manage::get_example() {
  // wait for WiFi connection
  if ((wifiMulti.run() == WL_CONNECTED)) {

    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    // configure traged server and url
    // http.begin("https://www.howsmyssl.com/a/check", ca); //HTTPS
    http.begin("http://example.com/index.html"); // HTTP

    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n",
                    http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}

void WiFi_Manage::post(String message) {
  if ((wifiMulti.run() == WL_CONNECTED)) {

    HTTPClient http;

    http.begin(SERVER_URL);
    int httpCode = http.POST(message);
    Serial.println("Message sent");
  }
}