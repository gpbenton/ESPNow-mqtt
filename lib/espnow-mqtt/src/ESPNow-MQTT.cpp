#include <Arduino.h>

#include "ESPNow-MQTT.h"

#if defined ESP32
/**
 * gotoSleep - deep sleep until sleepTime or wakeupPin is set to level
*/
void gotoSleep(long sleepTime, gpio_num_t wakeupPin, uint8_t level) {
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */

#if 1
  Serial.printf("Sleeping on pin %d at level %d\n", wakeupPin, level);
  esp_sleep_enable_timer_wakeup(sleepTime * uS_TO_S_FACTOR);
  esp_sleep_enable_ext0_wakeup(wakeupPin, level);
#if CORE_DEBUG_LEVEL > 0
  Serial.flush();
#endif
  esp_deep_sleep_start();
#else
  DEBUG_DBG(TAG, "Sleeping");
  sleep(sleepTime);
#endif
}
#else
#warning "Deep Sleep Unsupported platform"
#endif // ESP 32

/**
 * getWiFiChannel - returns wifi channel being used by ssid
*/
int32_t getWiFiChannel(const char* ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
    for (uint8_t i = 0; i < n; i++) {
      if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}
