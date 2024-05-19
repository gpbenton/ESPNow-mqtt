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
#include <AsyncMqttClient.h>
#include <Ticker.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;


#include <ESPNow-MQTT.h>
#include "secrets.h"

AsyncMqttClient mqttClient;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;
Ticker mqttReconnectTimer;

// For quickdebug
static const char* TAG = "gateway";

// put function declarations here:
void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);
void dataSent(uint8_t *mac_addr, uint8_t status);
void connectToWifi();
void onWifiConnect(const WiFiEventStationModeGotIP& event);
void onWifiDisconnect(const WiFiEventStationModeDisconnected& event);
void connectToMqtt();
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttSubscribe(uint16_t packetId, uint8_t qos);
void onMqttUnsubscribe(uint16_t packetId);
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
void onMqttPublish(uint16_t packetId);

void setup()
{
  Serial.begin(115200);

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  quickEspNow.onDataRcvd(dataReceived);
  quickEspNow.onDataSent(dataSent);

  WiFi.mode(WIFI_MODE_STA);
  connectToWifi();
}

void loop() {
}

// put function definitions here:
void dataSent(uint8_t *mac_addr, uint8_t status) {
  DEBUG_DBG(TAG, "dataSent to " MACSTR "  status %d\n", MAC2STR(mac_addr), status);

}

// Message received from ESPNow
void dataReceived(uint8_t *mac_addr, uint8_t *data, uint8_t len, signed int rssi, bool broadcast)
{
  DEBUG_INFO(TAG, "Received From: " MACSTR , MAC2STR(mac_addr));
  DEBUG_DBG(TAG, "RSSI: %d dBm", rssi);
  DEBUG_DBG(TAG, "%s", broadcast ? "Broadcast" : "Unicast");

  if (broadcast) {
    if (!strncmp((const char *)data, (const char *)GATEWAY_QUERY, sizeof(GATEWAY_QUERY))) {
      DEBUG_DBG(TAG, "Sending whois response\n" );
      quickEspNow.send(mac_addr, GATEWAY_QUERY, sizeof(GATEWAY_QUERY));
      DEBUG_DBG(TAG, "Sent whois response\n" );
    }

  } else if (mqttClient.connected()) {
    struct data rcvd_data;
    char macStr[18];

    snprintf(macStr, sizeof(macStr), "%02x%02x%02x%02x%02x%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    memcpy(&rcvd_data, data, min((unsigned int)len, sizeof(rcvd_data)));
    
    json json_message;
    json_message["rssi"] = rssi;
    json_message["battery_level"] = rcvd_data.batteryLevel;
    json_message["wakeupCause"] = rcvd_data.wakeupCause;
    json_message["sensor1"] = rcvd_data.sensor1;
    json_message["sensor2"] = rcvd_data.sensor2;
    json_message["sensor3"] = rcvd_data.sensor3;
    std::string mqttMsg = json_message.dump();

    String topic(TOPIC_ROOT);
    topic.concat(macStr);

    mqttClient.publish(topic.c_str(), 0, false, mqttMsg.c_str(), mqttMsg.length());
  }
}

void connectToWifi() {
  DEBUG_DBG(TAG, "Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  DEBUG_INFO(TAG, "Connected to %s in channel %d\n", WiFi.SSID().c_str(), WiFi.channel());
  DEBUG_DBG(TAG, "IP address: %s\n", WiFi.localIP().toString().c_str());
  DEBUG_DBG(TAG, "MAC address: %s\n", WiFi.macAddress().c_str());
  connectToMqtt();
  quickEspNow.begin(255U, 0U, false); // Use no parameters to start ESP-NOW on same channel as WiFi, in STA mode and synchronous send mode
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  DEBUG_WARN(TAG, "Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
  quickEspNow.stop();
}

void connectToMqtt() {
  DEBUG_DBG(TAG, "Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  DEBUG_INFO(TAG, "Connected to MQTT.");
  DEBUG_DBG(TAG, "Session present: %d", sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  DEBUG_WARN(TAG, "Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  DEBUG_DBG(TAG, "Subscribe acknowledged.");
  DEBUG_DBG(TAG, "  packetId: %d", packetId);
  DEBUG_DBG(TAG, "  qos: %d", qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  DEBUG_DBG(TAG, "Unsubscribe acknowledged.");
  DEBUG_DBG(TAG, "  packetId: %d", packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  DEBUG_DBG(TAG, "Publish received.");
  DEBUG_DBG(TAG, "  topic: %s", topic);
  DEBUG_DBG(TAG, "  qos: %d", properties.qos);
  DEBUG_DBG(TAG, "  dup: %d", properties.dup);
  DEBUG_DBG(TAG, "  retain: %d", properties.retain);
  DEBUG_DBG(TAG, "  len: %d", len);
  DEBUG_DBG(TAG, "  index: %d", index);
  DEBUG_DBG(TAG, "  total: %d", total);
}

void onMqttPublish(uint16_t packetId) {
  DEBUG_DBG(TAG, "Publish acknowledged.");
  DEBUG_DBG(TAG, "  packetId: %d", packetId);
}
