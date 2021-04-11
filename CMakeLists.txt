cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
project(enocean_to_hue)
include_directories("${CMAKE_SOURCE_DIR}/embedded/include")
add_executable(${PROJECT_NAME}
  # POSIX sources
  main.cpp
  command_mapping.cpp
  enocean_serial_posix.cpp
  enocean_to_hue_bridge.cpp
  hue_sensor_command_posix.cpp
  debug_posix.cpp
  # Common sources
  embedded/src/crc8.cpp
  embedded/src/enocean_serial.cpp
  embedded/src/hue_sensor_command.cpp
  # Embedded-only sources
  embedded/src/embedded_main.cpp
  embedded/src/enocean_serial_esp8266.cpp
  embedded/src/debug_stream_embedded.cpp
  embedded/src/hue_sensor_command_embedded.cpp
  embedded/src/embedded_syslog.cpp
)
