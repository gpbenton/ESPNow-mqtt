/**
 * Battery Powered Sensor to detect door/window openings, with light sensor
 * 
                                          ___+3.3V
                                            |
                                           _|_ 
                                          |4k7|
                                          |_ _|
                                            |
LIGHT_SENSOR_PIN      ______________________|
                                           _|_
                                          |pho|
                                          |to |
                                          |_ _|
                                            |
                                            |
                                            |
                                            |
LIGHT_SENSOR_CONROL_PIN __________________|/
                                          |\e
                                            |
                                   _________|_GND

*/
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
#endif  // ESP32
#include <QuickEspNow.h>

#include <ESPNow-MQTT.h>
#include "secrets.h"

// For quickdebug
static const char* TAG = "battery_sensor";

// Send message every 2 seconds
const unsigned int SEND_MSG_MSEC = 2000;
const unsigned int WHOIS_RETRY_LIMIT = 5;
RTC_DATA_ATTR int sharedChannel = 0;
RTC_DATA_ATTR uint8_t gateway_address[6];
bool haveAddress = false;
struct data msg;

// put function declarations here:
void dataReceived(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);

void setup() {
#if CORE_DEBUG_LEVEL > 0
  Serial.begin(115200);
  setTagDebugLevel(TAG, CORE_DEBUG_LEVEL);
#endif
  analogReadResolution(9);
  pinMode(OPEN_SENSOR_PIN, INPUT_PULLUP);
  pinMode(LIGHT_SENSOR_PIN, ANALOG);

  msg.wakeupCause = esp_sleep_get_wakeup_cause();

  switch (msg.wakeupCause) {
    case ESP_SLEEP_WAKEUP_EXT0:
    case ESP_SLEEP_WAKEUP_EXT1:
    case ESP_SLEEP_WAKEUP_TIMER:
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
    case ESP_SLEEP_WAKEUP_ULP:
      haveAddress = true;
      break;
    default:
      // Power Up or reset so find correct channel
      DEBUG_DBG(TAG, "Finding channel.  Wakeup cause %d\n", msg.wakeupCause);
      sharedChannel = getWiFiChannel(WIFI_SSID);
      DEBUG_DBG(TAG, "sharedChannel = %d\n", sharedChannel);
      haveAddress = false;
      break;
  }

  WiFi.mode(WIFI_MODE_STA);
#if defined ESP32
  WiFi.disconnect(false, true);
#elif defined ESP8266
  WiFi.disconnect(false);
#endif  // ESP32
  quickEspNow.onDataRcvd(dataReceived);
#ifdef ESP32
  quickEspNow.setWiFiBandwidth(WIFI_IF_STA,
                               WIFI_BW_HT20);  // Only needed for ESP32 in case you need coexistence
                                               // with ESP8266 in the same network
#endif                               // ESP32
  quickEspNow.begin(sharedChannel);  // If you use no connected WiFi channel needs to be specified
}

void loop() {
  static uint8_t whoisretries = 0;

  digitalWrite(LIGHT_SENSOR_CONTROL_PIN, HIGH);
  msg.batteryLevel = analogReadMilliVolts(BATTERY_SENSOR_PIN);
  DEBUG_DBG(TAG, "batteryLevel = %d", msg.batteryLevel);
  msg.sensor1 = digitalRead(OPEN_SENSOR_PIN);
  msg.sensor2 = analogRead(LIGHT_SENSOR_PIN);
  msg.sensor3 = 0;
  if (haveAddress && !quickEspNow.send(gateway_address, (const unsigned char*)&msg, sizeof(msg))) {
    DEBUG_DBG(TAG, " Message sent: wakeCause = %d\n", msg.wakeupCause);
    gotoSleep(600, OPEN_SENSOR_PIN, msg.sensor1 ? 0 : 1);
  } else {
    DEBUG_DBG(TAG, " Message send failed\n");
    haveAddress = false;

    // look for the gateway on this channel
    if (whoisretries < WHOIS_RETRY_LIMIT) {
      DEBUG_INFO(TAG, "Sending Gateway query\n");
      quickEspNow.send(ESPNOW_BROADCAST_ADDRESS, GATEWAY_QUERY, sizeof(GATEWAY_QUERY));
      whoisretries++;
    } else {
      // No gateway on this channel, look for the network id
      DEBUG_INFO(TAG, "Searching for WiFi Channel\n");
      sharedChannel = getWiFiChannel(WIFI_SSID);
      whoisretries = 0;
    }
  }

  delay(SEND_MSG_MSEC);
}

// put function definitions here:

// we can init our channel number from wifi ssid, but it consts seconds so only in case of
// restarting
void dataReceived(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
  DEBUG_INFO(TAG, "Received From: " MACSTR "\n", MAC2STR(address));
  DEBUG_DBG(TAG, "RSSI: %d dBm\n", rssi);
  DEBUG_DBG(TAG, "%s\n", broadcast ? "Broadcast" : "Unicast");

  if (!broadcast &&
      !strncmp((const char*)data, (const char*)GATEWAY_QUERY, sizeof(GATEWAY_QUERY))) {
    DEBUG_DBG(TAG, "Setting gateway address to " MACSTR "\n", MAC2STR(address));
    for (int i = 0; i < 6; i++) {
      gateway_address[i] = address[i];
    }
    DEBUG_DBG(TAG, "gateway_address = " MACSTR "\n", MAC2STR(gateway_address));
    haveAddress = true;
  }
}
