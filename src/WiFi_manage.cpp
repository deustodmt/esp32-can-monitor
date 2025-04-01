#include "WiFi_manage.h"
#include "config.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

WiFiClient wifiClient;
PubSubClient client(wifiClient);

WiFi_Manage::WiFi_Manage()
{
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("...................................");

  Serial.print("Connecting to WiFi.");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to the WiFi network");

  client.setServer(MQTT_SERVER, MQTT_PORT);

  while (!client.connected())
  {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client", MQTT_USER, MQTT_PASSWD))
      Serial.println("connected");
    else
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  mqtt_test();
}

void WiFi_Manage::get_example()
{
  HTTPClient http;

  Serial.print("[HTTP] begin...\n");
  // configure traged server and url
  // http.begin("https://www.howsmyssl.com/a/check", ca); //HTTPS
  http.begin("http://example.com/index.html"); // HTTP

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0)
  {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();
      Serial.println(payload);
    }
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %s\n",
                  http.errorToString(httpCode).c_str());
  }

  http.end();
}

void WiFi_Manage::post(String message)
{
  HTTPClient http;

  http.begin(SERVER_URL);
  int httpCode = http.POST(message);
  Serial.println("Message sent");
}

void WiFi_Manage::mqtt_test(void)
{
  // char str[16];
  // sprintf(str, "%u", random(100));

  client.publish("test_topic", "0CF11E0500000195f1621e0a05000000C3010000");
  sleep(1);
}
