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
RTC_DATA_ATTR int bootCount = 0;
bool responseRcvd = false;
bool msgSent = false;
struct data msg;

// put function declarations here:
int32_t getWiFiChannel(const char *ssid);
void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);
void gotoSleep(const long sleepTime);

void setup() {
  Serial.begin(115200);
  delay(1000);

  msg.sensor2 = bootCount;
  bootCount = bootCount + 1;
  msg.wakeupCause = esp_sleep_get_wakeup_cause();

  switch(msg.wakeupCause){
    case ESP_SLEEP_WAKEUP_EXT0 : break;
    case ESP_SLEEP_WAKEUP_EXT1 : break; 
    case ESP_SLEEP_WAKEUP_TIMER : 
    case ESP_SLEEP_WAKEUP_TOUCHPAD : break;
    case ESP_SLEEP_WAKEUP_ULP : break;
      break;
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
#ifdef ESP32
  quickEspNow.setWiFiBandwidth(WIFI_IF_STA, WIFI_BW_HT20); // Only needed for ESP32 in case you need coexistence with ESP8266 in the same network
#endif                                                     // ESP32
  quickEspNow.begin(sharedChannel);                        // If you use no connected WiFi channel needs to be specified
}

void loop() {

    static uint8_t retries = 0;

    msg.batteryLevel = 50;
    msg.sensor1 = sharedChannel;
    msg.sensor2 = 0;
    msg.sensor3 = 0;
    comms_send_error_t err = COMMS_SEND_OK;   //quickEspNow.send(ESPNOW_BROADCAST_ADDRESS, (const unsigned char *)&msg, sizeof(msg));
    if (!err)
    {
      sleep(5);
      Serial.printf(">>>>>>>>>> Message sent: wakeCause = %d   bootCount= %d\n", msg.wakeupCause, bootCount);
      delay(1);
      gotoSleep(10);
    }
    else
    {
      Serial.printf(">>>>>>>>>> Message not sent: %d (0x%x)\n", err, err);
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
  Serial.print("Received: ");
  Serial.printf("%.*s\n", len, data);
  Serial.printf("RSSI: %d dBm\n", rssi);
  Serial.printf("From: " MACSTR "\n", MAC2STR(address));
  Serial.printf("%s\n", broadcast ? "Broadcast" : "Unicast");
}

void gotoSleep(long sleepTime){
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

  esp_sleep_enable_timer_wakeup(sleepTime * uS_TO_S_FACTOR);
  delay(1000);
  Serial.flush();
  esp_deep_sleep_start();
}
