#include <Arduino.h>

#if defined ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#define WIFI_MODE_STA WIFI_STA
#else
#error "Unsupported platform"
#endif //ESP32
#include <QuickEspNow.h>

#include "secrets.h"

// put function declarations here:
void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("Connected to %s in channel %d\n", WiFi.SSID().c_str(), WiFi.channel());
  Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("MAC address: %s\n", WiFi.macAddress().c_str());
  quickEspNow.onDataRcvd(dataReceived);
  quickEspNow.begin(); // Use no parameters to start ESP-NOW on same channel as WiFi, in STA mode and synchronous send mode
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:

void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
    Serial.print ("Received: ");
    Serial.printf ("%.*s\n", len, data);
    Serial.printf ("RSSI: %d dBm\n", rssi);
    Serial.printf ("From: " MACSTR "\n", MAC2STR(address));
    Serial.printf ("%s\n", broadcast ? "Broadcast" : "Unicast");
}