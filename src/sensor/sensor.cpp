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

#include "ESPNow-MQTT.h"
#include "secrets.h"

// Send message every 2 seconds
const unsigned int SEND_MSG_MSEC = 10000;

uint8_t gateway_address[6];
bool haveAddress = false;

// put function declarations here:
void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  ets_set_printf_channel(0);

  WiFi.mode(WIFI_MODE_STA);
  analogReadResolution(10);
  analogSetAttenuation(ADC_11db);
  initAnalogPin(LIGHT_SENSOR_PIN, LIGHT_SENSOR_CONTROL_PIN);

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
  log_d("Gateway Query Sent");
}


void loop() {
  static unsigned int counter = 0;

  digitalWrite(LIGHT_SENSOR_CONTROL_PIN, HIGH);
  struct data msg;
  msg.batteryLevel = 0;
  msg.wakeupCause = 0;
  msg.sensor1 = 0;
  msg.sensor2 = analogRead(LIGHT_SENSOR_PIN);
  msg.sensor3 = 0;
  digitalWrite(LIGHT_SENSOR_CONTROL_PIN, LOW);

  if (haveAddress && !quickEspNow.send(gateway_address, (const unsigned char*)&msg, sizeof(msg))) {
    log_v("Message sent");
    } else {
        log_e("Message not sent");
        quickEspNow.send(ESPNOW_BROADCAST_ADDRESS, GATEWAY_QUERY, sizeof(GATEWAY_QUERY));
    }

    delay (SEND_MSG_MSEC);

}

// put function definitions here:

void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
  log_d("Received From: " MACSTR "    ", MAC2STR(address));
  log_d("%d bytes   ", len);
  log_d("RSSI: %d dBm    ", rssi);
  log_d("%s\n", broadcast ? "Broadcast" : "Unicast");

  if (!broadcast && !strncmp((const char *)data, (const char *)GATEWAY_QUERY, sizeof(GATEWAY_QUERY))) {
    log_i("Setting gateway address to " MACSTR, MAC2STR(address));
    for (int i=0; i<6; i++) {
      gateway_address[i] = address[i];
    }
    haveAddress = true;
  }
}