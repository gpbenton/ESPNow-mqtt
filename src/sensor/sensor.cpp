#include <Arduino.h>
#include <QuickDebug.h>
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

#include <ESPNow-MQTT.h>
#include "secrets.h"

// For quickdebug
static const char* TAG = "sensor";

// Send message every 2 seconds
const unsigned int SEND_MSG_MSEC = 2000;

uint8_t gateway_address[6];
bool haveAddress = false;

// put function declarations here:
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
        DEBUG_DBG(TAG,"Message sent");
    } else {
        DEBUG_ERROR(TAG," Message not sent");
        quickEspNow.send(ESPNOW_BROADCAST_ADDRESS, GATEWAY_QUERY, sizeof(GATEWAY_QUERY));
    }

    delay (SEND_MSG_MSEC);

}

// put function definitions here:

void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
  DEBUG_DBG(TAG,"Received From: " MACSTR "    ", MAC2STR(address));
  DEBUG_DBG(TAG,"%d bytes   ", len);
  DEBUG_DBG(TAG,"RSSI: %d dBm    ", rssi);
  DEBUG_DBG(TAG,"%s\n", broadcast ? "Broadcast" : "Unicast");

  if (!broadcast && !strncmp((const char *)data, (const char *)GATEWAY_QUERY, sizeof(GATEWAY_QUERY))) {
    DEBUG_WARN(TAG, "Setting gateway address to " MACSTR, MAC2STR(address));
    for (int i=0; i<6; i++) {
      gateway_address[i] = address[i];
    }
    haveAddress = true;
  }
}