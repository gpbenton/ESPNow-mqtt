// Failed attempt at combining mqtt over WiFi and ESPNow on same chip.
// This code works ok, but the lolin S2 nodes couldn't scan to find the channel for the WiFi network
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
#include <ArduinoJson.h>


#include "ESPNow-MQTT.h"
#include "secrets.h"

// put function declarations here:
void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);
void dataSent (uint8_t* address, uint8_t status);

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);
#if defined ESP32
  WiFi.disconnect(false, true);
#elif defined ESP8266
  WiFi.disconnect(false);
#endif // ESP32

  quickEspNow.begin(ESPN_CHANNEL, 0, false);
  quickEspNow.onDataSent(dataSent);
  quickEspNow.onDataRcvd(dataReceived);
  Serial.printf("Connected channel %d\n", WiFi.channel());
  Serial.printf("MAC address: %s\n", WiFi.macAddress().c_str());
}

void loop() {
}

// put function definitions here:

// Message received from ESPNow
void dataReceived(uint8_t *mac_addr, uint8_t *data, uint8_t len, signed int rssi, bool broadcast)
{
  Serial.print("Received: ");
  Serial.printf("length %d   ", len);
  Serial.printf("RSSI: %d dBm   ", rssi);
  Serial.printf("From: " MACSTR "   ", MAC2STR(mac_addr));
  Serial.printf("%s\n", broadcast ? "Broadcast" : "Unicast");
    struct data rcvd_data;
    char macStr[18];

    snprintf(macStr, sizeof(macStr), "%02x%02x%02x%02x%02x%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    memcpy(&rcvd_data, data, min((unsigned int)len, sizeof(rcvd_data)));
    
#if 0
    json json_message;
    json_message["rssi"] = rssi;
    json_message["battery_level"] = rcvd_data.batteryLevel;
    json_message["wakeupCause"] = rcvd_data.wakeupCause;
    json_message["sensor1"] = rcvd_data.sensor1;
    json_message["sensor2"] = rcvd_data.sensor2;
    json_message["sensor3"] = rcvd_data.sensor3;
    std::string mqttMsg = json_message.dump();
#else
    JsonDocument json_message;
    json_message[0]["rssi"] = rssi;
    json_message[0]["battery_level"] = rcvd_data.batteryLevel;
    json_message[0]["wakeupCause"] = rcvd_data.wakeupCause;
    json_message[0]["sensor1"] = rcvd_data.sensor1;
    json_message[0]["sensor2"] = rcvd_data.sensor2;
    json_message[0]["sensor3"] = rcvd_data.sensor3;

    serializeJson(json_message, Serial);

    Serial.println();
#endif

    String topic(TOPIC_ROOT);
    topic.concat(macStr);

    // TODO publish to UART
}

void dataSent (uint8_t* address, uint8_t status) {
    Serial.printf ("Message sent to " MACSTR ", status: %d\n", MAC2STR (address), status);
}

