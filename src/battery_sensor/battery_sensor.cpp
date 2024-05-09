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
  msg.wakeupCause = esp_sleep_get_wakeup_cause();

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
  quickEspNow.begin(ESPN_CHANNEL);                        // If you use no connected WiFi channel needs to be specified
}

void loop() {

    static uint8_t retries = 0;

    msg.batteryLevel = 50;
    msg.sensor1 = ESPN_CHANNEL;
    msg.sensor2 = retries;
    msg.sensor3 = 0;
    //comms_send_error_t err = quickEspNow.send(GATEWAY_ADDRESS, (const unsigned char *)&msg, sizeof(msg));
    comms_send_error_t err = quickEspNow.send(ESPNOW_BROADCAST_ADDRESS, (const unsigned char *)&msg, sizeof(msg));
    //comms_send_error_t err = quickEspNow.sendBcast((const unsigned char *)&msg, sizeof(msg));
    if (!err)
    {
      sleep(5);
      Serial.printf(">>>>>>>>>> Message sent: wakeCause = %d   \n", msg.wakeupCause );
      delay(1);
      gotoSleep(5);
    }
    else
    {
      Serial.printf(">>>>>>>>>> Message not sent: %d (0x%x)\n", err, err);
      Serial.flush();
      retries++;
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

void gotoSleep(long sleepTime){
#if 0
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

  esp_sleep_enable_timer_wakeup(sleepTime * uS_TO_S_FACTOR);
  delay(1000);
  Serial.flush();
  esp_deep_sleep_start();
#else
  sleep(sleepTime);
#endif


}
