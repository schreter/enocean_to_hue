// Copy to user_config.hpp and adjust for your network settings

/// WiFi name.
const char* ssid = "MY_NET";

/// WiFi password.
const char* password = "MY_PWD";

/// Bridge IP address.
IPAddress bridge(192, 168, 1, 128);

/// Password for OTA updates.
const char* ota_password = "admin";

/// API key to authorize connection to the bridge.
const char* api_key = "xxx";

/// Sensor to trigger for external input.
int sensor_id = 36;
