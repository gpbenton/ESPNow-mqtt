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
const unsigned int WHOIS_RETRY_LIMIT = 5;
RTC_DATA_ATTR int sharedChannel = 0 ;
RTC_DATA_ATTR uint8_t gateway_address[6];
bool responseRcvd = false;
bool msgSent = false;
bool haveAddress = false;
struct data msg;

// put function declarations here:
int32_t getWiFiChannel(const char *ssid);
void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);
void gotoSleep(const long sleepTime);

void setup() {
  Serial.begin(115200);
  delay(1000);

  msg.wakeupCause = esp_sleep_get_wakeup_cause();

  switch(msg.wakeupCause){
    case ESP_SLEEP_WAKEUP_EXT0:
    case ESP_SLEEP_WAKEUP_EXT1:
    case ESP_SLEEP_WAKEUP_TIMER: 
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
    case ESP_SLEEP_WAKEUP_ULP:
      haveAddress = true;
      break;
    default:
      // Power Up or reset so find correct channel
      Serial.printf("Finding channel.  Wakeup cause %d\n", msg.wakeupCause);
      sharedChannel = getWiFiChannel(WIFI_SSID); 
      Serial.printf("sharedChannel = %d\n", sharedChannel);
      haveAddress = false;
      break;
  }

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
  quickEspNow.begin(sharedChannel);                        // If you use no connected WiFi channel needs to be specified
}

void loop() {

  static uint8_t whoisretries = 0;

  msg.batteryLevel = whoisretries;
  msg.sensor1 = sharedChannel;
  msg.sensor2 = 0;
  msg.sensor3 = 0;
  if (haveAddress && !quickEspNow.send(gateway_address, (const unsigned char *)&msg, sizeof(msg))) {
    Serial.printf(">>>>>>>>>> Message sent: wakeCause = %d\n", msg.wakeupCause);
    gotoSleep(10);
  }
  else {
    Serial.printf(">>>>>>>>>> Message send failed\n");
    haveAddress = false;

    // look for the gateway on this channel
    if (whoisretries < WHOIS_RETRY_LIMIT) {
      Serial.printf("Sending Gateway query\n");
      quickEspNow.send(ESPNOW_BROADCAST_ADDRESS, GATEWAY_QUERY, sizeof(GATEWAY_QUERY));
      whoisretries++;
    }
    else {
      // No gateway on this channel, look for the network id
      Serial.printf("Searching for WiFi Channel\n");
      sharedChannel = getWiFiChannel(WIFI_SSID);
      whoisretries = 0;
    }
  }

  delay(SEND_MSG_MSEC);
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
  Serial.print("Received: ");
  Serial.printf("%.*s\n", len, data);
  Serial.printf("RSSI: %d dBm\n", rssi);
  Serial.printf("From: " MACSTR "\n", MAC2STR(address));
  Serial.printf("%s\n", broadcast ? "Broadcast" : "Unicast");

  if (!broadcast && !strncmp((const char *)data, (const char *)GATEWAY_QUERY, sizeof(GATEWAY_QUERY))) {
    Serial.printf("Setting gateway address to " MACSTR "\n", MAC2STR(address));
    for (int i=0; i<6; i++) {
      gateway_address[i] = address[i];
    }
    Serial.printf("gateway_address = " MACSTR "\n", MAC2STR(gateway_address));
    haveAddress = true;
  }
}

void gotoSleep(long sleepTime){
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

  esp_sleep_enable_timer_wakeup(sleepTime * uS_TO_S_FACTOR);
  Serial.flush();
  esp_deep_sleep_start();
}
