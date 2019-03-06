// Copy to user_config.hpp and adjust for your network settings
//
// NOTE: Group ID is determined by using last octet of the assigned IP address.

/// WiFi name.
const char* ssid = "MY_NET";

/// WiFi password.
const char* password = "MY_PWD";

/// Syslog server host.
IPAddress syslog_host(192, 168, 1, 127);

/// Syslog server port. Set to 0 to not use syslog facility.
uint16_t syslog_port = 514;

/// Bridge IP address.
IPAddress bridge(192, 168, 1, 128);

/// Password for OTA updates.
const char* ota_password = "admin";

/// API key to authorize connection to the bridge.
const char* api_key = "xxx";

/// Sensor to trigger for external input.
int sensor_id = 36;
