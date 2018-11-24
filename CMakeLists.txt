cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
project(enocean_to_hue)
add_executable(${PROJECT_NAME}
  "main.cpp"
  "command_mapping.cpp"
  "crc8.cpp"
  "enocean_serial.cpp"
  "enocean_to_hue_bridge.cpp"
  "hue_sensor_command.cpp"
  # headers not necessary, but added to make them show in Qt Creator
  "command_mapping.hpp"
  "crc8.hpp"
  "enocean.hpp"
  "enocean_serial.hpp"
  "enocean_to_hue_bridge.hpp"
  "hue_sensor_command.hpp")
