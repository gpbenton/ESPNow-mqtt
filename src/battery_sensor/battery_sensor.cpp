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
RTC_DATA_ATTR int sharedChannel = 0 ;
bool responseRcvd = false;
bool msgSent = false;
esp_sleep_wakeup_cause_t wakeup_reason;

// For testing
const long sleepTime = 4000 * 1000L;

// put function declarations here:
int32_t getWiFiChannel(const char *ssid);
void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);
void dataAcked (uint8_t* address, uint8_t status);
void gotoSleep(const long sleepTime);

void setup() {
  Serial.begin(115200);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);

  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason){
    //case ESP_SLEEP_WAKEUP_EXT0 : break;
    //case ESP_SLEEP_WAKEUP_EXT1 : break; 
    case ESP_SLEEP_WAKEUP_TIMER : 
      break;
    //case ESP_SLEEP_WAKEUP_TOUCHPAD : break;
    //case ESP_SLEEP_WAKEUP_ULP : break;
    default:
      sharedChannel = getWiFiChannel(WIFI_SSID); 
      break;
  }

  WiFi.mode(WIFI_MODE_STA);
#if defined ESP32
  WiFi.disconnect(false, true);
#elif defined ESP8266
  WiFi.disconnect(false);
#endif // ESP32
  quickEspNow.onDataRcvd(dataReceived);
  quickEspNow.onDataSent(dataAcked);
#ifdef ESP32
  quickEspNow.setWiFiBandwidth(WIFI_IF_STA, WIFI_BW_HT20); // Only needed for ESP32 in case you need coexistence with ESP8266 in the same network
#endif                                                     // ESP32
  quickEspNow.begin(sharedChannel);                        // If you use no connected WiFi channel needs to be specified
}


void loop() {
    static uint8_t counter = 0;

    if (!msgSent)
    {

      struct data msg;
      msg.batteryLevel = counter++;
      msg.wakeupCause = wakeup_reason;
      msg.sensor1 = 0;
      msg.sensor2 = 0;
      msg.sensor3 = 0;

      if (!quickEspNow.send(ESPNOW_BROADCAST_ADDRESS, (const unsigned char *)&msg, sizeof(msg)))
      {
        Serial.println(">>>>>>>>>> Message sent");
      }
      else
      {
        Serial.println(">>>>>>>>>> Message not sent");
      }
      msgSent = true;
    }
    if (responseRcvd) {
      digitalWrite(BUILTIN_LED, LOW);
      gotoSleep(sleepTime);
    }
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
}

void dataAcked (uint8_t* address, uint8_t status) {
  Serial.printf("Acked From: " MACSTR "   ", MAC2STR(address));
  Serial.printf("Status: %d\n", status);
  if (status == 0) {
    responseRcvd = true;
  }
}

void gotoSleep(long sleepTime){
  
  esp_sleep_enable_timer_wakeup(sleepTime);
  esp_deep_sleep_start();
}
