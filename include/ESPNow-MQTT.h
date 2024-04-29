

struct data {
    u_int64_t sensor1;
    u_int64_t sensor2;
    u_int64_t sensor3;
    u_int8_t wakeupCause;
    u_int8_t batteryLevel;
};

#define TOPIC_ROOT "espnow/"