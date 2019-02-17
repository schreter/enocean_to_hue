cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
project(enocean_to_hue)
add_executable(${PROJECT_NAME}
  "main.cpp"
  "command_mapping.cpp"
  "embedded/crc8.cpp"
  "embedded/enocean_serial.cpp"
  "enocean_serial_posix.cpp"
  "enocean_to_hue_bridge.cpp"
  "embedded/hue_sensor_command.cpp"
  "hue_sensor_command_posix.cpp"
  "debug_posix.cpp"
  # Embedded sources
  "embedded/embedded_main.cpp"
  "embedded/enocean_serial_esp8266.cpp"
  "embedded/debug_stream_embedded.cpp"
  "embedded/hue_sensor_command_embedded.cpp"
)
