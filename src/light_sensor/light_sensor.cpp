/**
 * Battery Powered Sensor to detect only light levels
 * 
                                          ___+3.3V    _______VBUS
                                            |         |
                                           _|_        \
                                          |4k7|       /  20k
                                          |_ _|       \
                                            |         |
LIGHT_SENSOR_PIN      ______________________|         |_____________ BATTERY_SENSOR_PIN
                                           _|_        |
                                          |pho|       \
                                          |to |       /  20k
                                          |_ _|       \
                                            |         |
                                            |__________
                                            |
                                            |
LIGHT_SENSOR_CONROL_PIN __________________|/
                                          |\e
                                            |
                                   _________|_GND

*/
#include <Arduino.h>

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

#include "ESPNow-MQTT.h"
#include "secrets.h"

// Send message every 2 seconds
const unsigned int SEND_MSG_MSEC = 2000;
const uint64_t WAIT_SECS = 60 * 5; // 5 minutes
const unsigned int WHOIS_RETRY_LIMIT = 2;
const unsigned int FIND_CHANNEL_RETRY_LIMIT = 3;
RTC_DATA_ATTR int sharedChannel = 0;
RTC_DATA_ATTR uint8_t gateway_address[6];
bool haveAddress = false;
RTC_DATA_ATTR u_int64_t last_sent_light_level;
const u_int64_t MAX_DIFFERENCE = 20;
struct data msg;

// put function declarations here:
void dataReceived(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);

void setup() {
#if CORE_DEBUG_LEVEL > 0
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  ets_set_printf_channel(0);
#endif
  analogReadResolution(10);
  analogSetAttenuation(ADC_11db);
  initAnalogPin(LIGHT_SENSOR_PIN, LIGHT_SENSOR_CONTROL_PIN);
  pinMode(BATTERY_SENSOR_PIN, ANALOG);

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
      log_d("Finding channel.  Wakeup cause %d\n", msg.wakeupCause);
      sharedChannel = getWiFiChannel(WIFI_SSID);
      log_d("sharedChannel = %d\n", sharedChannel);
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
  static uint8_t findchannelretries = 0;

  digitalWrite(LIGHT_SENSOR_CONTROL_PIN, HIGH);
  msg.batteryLevel = analogRead(BATTERY_SENSOR_PIN);
  log_d("batteryLevel = %d", msg.batteryLevel);
  msg.sensor1 = 0;
  msg.sensor2 = analogRead(LIGHT_SENSOR_PIN);
  msg.sensor3 = 0;

  if (msg.wakeupCause == ESP_SLEEP_WAKEUP_TIMER
      &&
      (int64_t(msg.sensor2 - last_sent_light_level)) < MAX_DIFFERENCE) {
    log_d("No Message sent:  difference too small - %ld", msg.sensor2);
    sleepfor(WAIT_SECS);
  }

  if (haveAddress && !quickEspNow.send(gateway_address, (const unsigned char*)&msg, sizeof(msg))) {
    log_d(" Message sent: wakeCause = %d\n", msg.wakeupCause);
    last_sent_light_level = msg.sensor2;
    sleepfor(WAIT_SECS);
  } else {
    log_d(" Message send failed\n");
    haveAddress = false;

    // look for the gateway on this channel
    if (whoisretries < WHOIS_RETRY_LIMIT) {
      log_d("Sending Gateway query\n");
      quickEspNow.send(ESPNOW_BROADCAST_ADDRESS, GATEWAY_QUERY, sizeof(GATEWAY_QUERY));
      whoisretries++;
    } else if (findchannelretries < FIND_CHANNEL_RETRY_LIMIT) {
      // No gateway on this channel, look for the network id
      log_i("Searching for WiFi Channel\n");
      sharedChannel = getWiFiChannel(WIFI_SSID);
      whoisretries = 0;
      findchannelretries++;
    } else {
      // Cannot find the channel so wait for next period
      sharedChannel = 0;
      sleepfor(WAIT_SECS);
    }
  }

  delay(SEND_MSG_MSEC);
}

// put function definitions here:

// we can init our channel number from wifi ssid, but it consts seconds so only in case of
// restarting
void dataReceived(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
  log_d("Received From: " MACSTR "\n", MAC2STR(address));
  log_v("RSSI: %d dBm\n", rssi);
  log_v("%s\n", broadcast ? "Broadcast" : "Unicast");

  if (!broadcast &&
      !strncmp((const char*)data, (const char*)GATEWAY_QUERY, sizeof(GATEWAY_QUERY))) {
    log_i("Setting gateway address to " MACSTR "\n", MAC2STR(address));
    for (int i = 0; i < 6; i++) {
      gateway_address[i] = address[i];
    }
    haveAddress = true;
  }
}
