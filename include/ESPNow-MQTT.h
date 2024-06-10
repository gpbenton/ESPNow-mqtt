#ifndef ESPNOW_MQTT_H
#define ESPNOW_MQTT_H

#include <Arduino.h>

#if defined ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#define WIFI_MODE_STA WIFI_STA
#define gpio_num_t uint8_t
// TODO: Find pinModes for ESP8266
#define ANALOG 0xC0   
#else
#error "Unsupported platform"
#endif  // ESP32


struct data {
    u_int64_t sensor1;
    u_int64_t sensor2;
    u_int64_t sensor3;
    u_int8_t wakeupCause;
    u_int8_t batteryLevel;
};

#define TOPIC_ROOT "espnow/"

const uint8_t GATEWAY_ADDRESS[] = { 0xDC, 0x4F, 0x22, 0x60, 0xAC, 0xAD };

const uint8_t GATEWAY_QUERY[] = "whois_gateway";

int32_t getWiFiChannel(const char* ssid);

void initAnalogPin(gpio_num_t pin, gpio_num_t control_pin);

#if defined ESP32
void gotoSleep(const uint64_t sleepTime, gpio_num_t wakeupPin, uint8_t level);
#endif

#endif