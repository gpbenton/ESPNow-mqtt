Sends messages from ESPNow to MQTT.

A powered gateway is defined that receives ESPNow messages and forwards them to the MQTT Broker.  ESPNow is configured to use the same WiFi channel as the configured SSID, so the gateway can interoperate with them.  On powerup, and after failed message sending, the sensors scan the wifi signals to find the correct channel, and then do an ESPNow broadcast to find the gateway.

MQTT Topic is  `espnow/{mac_address_of_sensor_node} `
MQTT

# Status
Alpha - basic principle works with `wemos_d1_mini` gateway and `lolin s2 mini` sensor.

## TODO:

- [ ] Battery level detection - use esp32 get vdd?  Is there an internal voltage reference for analog io?  Turn on voltage divider with transistor.
- [ ] Send JSON message over ESPNow?  Simpler for gateway and more flexible, but increases the size of the ESPNow message.
- [ ] Light sensor.
- [ ] Two way communication.

# ACKNOWLEDGEMENTS
Building on the shoulders of giants

[QuickEspNow](https://github.com/gmag11/QuickESPNow)

[async-mqtt-client](https://github.com/HeMan/async-mqtt-client)

[nlohman-json](https://github.com/Johboh/nlohmann-json)
