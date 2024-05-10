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
  quickEspNow.begin(ESPN_CHANNEL);                                    // If you use no connected WiFi channel needs to be specified
}


void loop() {
    static unsigned int counter = 0;

    struct data msg;
    msg.batteryLevel = counter++;
    msg.wakeupCause = 1;
    msg.sensor1 = 0;
    msg.sensor2 = 0;
    msg.sensor3 = 0;

    if (!quickEspNow.send (GATEWAY_ADDRESS, (const unsigned char *)&msg, sizeof(msg))) {
        Serial.println (">>>>>>>>>> Message sent");
    } else {
        Serial.println (">>>>>>>>>> Message not sent");
    }

    delay (SEND_MSG_MSEC);

}

// put function definitions here:
void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
  Serial.print("Received: ");
  Serial.printf("%.*s\n", len, data);
  Serial.printf("RSSI: %d dBm\n", rssi);
  Serial.printf("From: " MACSTR "\n", MAC2STR(address));
  Serial.printf("%s\n", broadcast ? "Broadcast" : "Unicast");
}