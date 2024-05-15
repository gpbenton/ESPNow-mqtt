#ifndef ESPNOW_MQTT_H
#define ESPNOW_MQTT_H

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

#endif