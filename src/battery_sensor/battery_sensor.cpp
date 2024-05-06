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

#define DEBUG 
// Repeat Send message every 2 seconds
const unsigned int SEND_MSG_MSEC = 2000;
RTC_NOINIT_ATTR int sharedChannel = 0;
bool gettingChannel = false;
RTC_DATA_ATTR int bootCount = 0;
bool responseRcvd = false;
bool msgSent = false;
struct data msg;

// put function declarations here:
int32_t getWiFiChannel(const char *ssid);
void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);
void gotoSleep(const long sleepTime);

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
  delay(1000);
#endif

  msg.sensor2 = bootCount;
  bootCount = bootCount + 1;
#ifdef DEBUG
  Serial.println("Getting Wakeup cause");
#endif
  msg.wakeupCause = esp_sleep_get_wakeup_cause();

#ifdef DEBUG
  Serial.printf("Got Wakeup Cause %d\n", msg.wakeupCause);
#endif

}

void loop() {

  //if (msg.wakeupCause == 0) {
  if (sharedChannel == 0) {
#ifdef DEBUG
      Serial.println("Scanning");
      Serial.flush();
#endif
    sharedChannel = getWiFiChannel(WIFI_SSID);
#ifdef DEBUG
          Serial.printf("Found channel %d\n", sharedChannel);
#endif
#if 0
    if (!gettingChannel) {
#ifdef DEBUG
      Serial.println("Scanning");
#endif
      WiFi.scanNetworks(true, false, false, 300U, 0U, WIFI_SSID);
      gettingChannel = true;
    } else{
      int16_t i;
      i = WiFi.scanComplete();

      switch (i) {
        case WIFI_SCAN_FAILED:
#ifdef DEBUG
          Serial.printf("Error found Scanning %d\n", i);
#endif
          gettingChannel = false;
          break;

        case WIFI_SCAN_RUNNING:
#ifdef DEBUG
          Serial.printf("Still Scanning %d\n", i);
#endif
          break;
        case 0:
#ifdef DEBUG
          Serial.printf("No networks found %d\n", i);
#endif
          gettingChannel = false;
          break;
        default:
          sharedChannel = WiFi.channel(0);
#ifdef DEBUG
          Serial.printf("Found channel %d\n", sharedChannel);
#endif
          break;

      }
    }
#endif
  } 

  if (sharedChannel != 0) {
#ifdef DEBUG
    Serial.printf("Using channel %d\n", sharedChannel);
#endif
    WiFi.begin(WIFI_SSID);
#if defined ESP32
    WiFi.disconnect(false, true);
#elif defined ESP8266
    WiFi.disconnect(false);
#endif // ESP32
    quickEspNow.onDataRcvd(dataReceived);
#ifdef ESP32
    quickEspNow.setWiFiBandwidth(WIFI_IF_STA, WIFI_BW_HT20); // Only needed for ESP32 in case you need coexistence with ESP8266 in the same network
#endif                                                     // ESP32
    if (quickEspNow.begin(sharedChannel)) {                        // If you use no connected WiFi channel needs to be specified
#ifdef DEBUG
      Serial.println("quickEsp started");
#endif

      msg.batteryLevel = 50;
      msg.sensor1 = sharedChannel;
      msg.sensor2 = bootCount;
      msg.sensor3 = 0;


#ifdef DEBUG
      Serial.printf("Sending message now on channel %d\n", sharedChannel);
      Serial.flush();
#endif
      comms_send_error_t err = quickEspNow.send(ESPNOW_BROADCAST_ADDRESS, (const unsigned char *)&msg, sizeof(msg));
      if (!err)
      {
#ifdef DEBUG
        Serial.printf(">>>>>>>>>> Message sent: wakeCause = %d   bootCount= %d\n", msg.wakeupCause, bootCount);
#endif
        gotoSleep(10);
      } else {
#ifdef DEBUG
        Serial.printf(">>>>>>>>>> Message not sent: %d (0x%x)\n", err, err);
#endif

      }
    } else {
#ifdef DEBUG
      Serial.println("quickEsp.begin failed");
#endif

    }

  }

#ifdef DEBUG
  Serial.printf("Restarting loop\n");
#endif

  delay (SEND_MSG_MSEC);
#ifdef DEBUG
      Serial.println("Scanning at end");
      Serial.flush();
#endif
  sharedChannel = getWiFiChannel(WIFI_SSID);

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
#ifdef DEBUG
  Serial.println("Got Wakedup Cause");
  Serial.print("Received: ");
  Serial.printf("%.*s\n", len, data);
  Serial.printf("RSSI: %d dBm\n", rssi);
  Serial.printf("From: " MACSTR "\n", MAC2STR(address));
  Serial.printf("%s\n", broadcast ? "Broadcast" : "Unicast");
#endif
}

void gotoSleep(long sleepTime){
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

  esp_sleep_enable_timer_wakeup(sleepTime * uS_TO_S_FACTOR);
#ifdef DEBUG
  Serial.flush();
#endif
  esp_deep_sleep_start();
}
