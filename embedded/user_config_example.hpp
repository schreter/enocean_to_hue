// Copy to user_config.hpp and adjust for your network settings

/// WiFi name.
const char* ssid = "MY_NET";

/// WiFi password.
const char* password = "MY_PWD";

/// Syslog server host.
IPAddress syslog_host(192, 168, 1, 127);

/// Syslog server port. Set to 0 to not use syslog facility.
uint16_t syslog_port = 514;

// Indirect connection via server (repeater modus)

/// IP address of the master server.
IPAddress master(192, 168, 1, 129);

/// Port of the master server.
uint16_t master_port = 22554;

/// Password for OTA updates.
const char* ota_password = "admin";

