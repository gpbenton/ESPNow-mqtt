

struct data {
    u_int8_t wakeupCause;
    u_int64_t batteryLevel;
    u_int64_t sensor1;
    u_int64_t sensor2;
    u_int64_t sensor3;
};

#define TOPIC_ROOT "espnow/"