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

#include "ESPNow-MQTT.h"
#include "secrets.h"

// Send message every 2 seconds
const unsigned int SEND_MSG_MSEC = 2000;

uint8_t gateway_address[6];
bool haveAddress = false;

// put function declarations here:
int32_t getWiFiChannel(const char *ssid);
void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);
#if defined ESP32
  WiFi.disconnect(false, true);
#elif defined ESP8266
  WiFi.disconnect(false);
#endif // ESP32
  quickEspNow.onDataRcvd(dataReceived);
#ifdef ESP32
  quickEspNow.setWiFiBandwidth(WIFI_IF_STA, WIFI_BW_HT20); // Only needed for ESP32 in case you need coexistence with ESP8266 in the same network
#endif                                                     // ESP32
  int sharedChannel = getWiFiChannel(WIFI_SSID); 
  quickEspNow.begin(sharedChannel);                                    // If you use no connected WiFi channel needs to be specified

  quickEspNow.send(ESPNOW_BROADCAST_ADDRESS, GATEWAY_QUERY, sizeof(GATEWAY_QUERY));
}


void loop() {
    static unsigned int counter = 0;

    struct data msg;
    msg.batteryLevel = counter++;
    msg.wakeupCause = 1;
    msg.sensor1 = 0;
    msg.sensor2 = 0;
    msg.sensor3 = 0;

    if (haveAddress && !quickEspNow.send (gateway_address, (const unsigned char *)&msg, sizeof(msg))) {
        Serial.println (">>>>>>>>>> Message sent");
    } else {
        quickEspNow.send(ESPNOW_BROADCAST_ADDRESS, GATEWAY_QUERY, sizeof(GATEWAY_QUERY));
        Serial.println (">>>>>>>>>> Message not sent");
    }

    delay (SEND_MSG_MSEC);

}

// put function definitions here:
//we can init our channel number from wifi ssid, but it consts seconds so only in case of restarting
int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
  Serial.printf("Received From: " MACSTR "    ", MAC2STR(address));
  Serial.printf("%d bytes   ", len);
  Serial.printf("RSSI: %d dBm    ", rssi);
  Serial.printf("%s\n", broadcast ? "Broadcast" : "Unicast");

  if (!broadcast && !strncmp((const char *)data, (const char *)GATEWAY_QUERY, sizeof(GATEWAY_QUERY))) {
    Serial.printf("Setting gateway address to " MACSTR, MAC2STR(address));
    for (int i=0; i<6; i++) {
      gateway_address[i] = address[i];
    }
    haveAddress = true;
  }
}