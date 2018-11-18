# configure with:
# CXX_FLAGS=-static C_FLAGS=-static LD_FLAGS=-static PATH=$PATH:/opt/bin cmake <path to sources>
cmake_minimum_required(VERSION 2.8)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++17 -static -Wall")
project(enocean_to_hue)
add_executable(${PROJECT_NAME}
  "main.cpp"
  "crc8.cpp"
  "enocean_serial.cpp"
  "hue_sensor_command.cpp"
  # headers not necessary, but added to make them show in Qt Creator
  "crc8.hpp"
  "enocean.hpp"
  "enocean_serial.hpp"
  "hue_sensor_command.hpp")
