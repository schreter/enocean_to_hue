#
# This toolchain was set up by pre-compiled toolchains from
# https://www.jaredwolff.com/cross-compiling-on-mac-osx-for-raspberry-pi/.
#
# Setup:
# cmake /path/to/src -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake
#
# To run the project on older kernels, you can combine Linux 3.x toolchain
# with Linux 4.x toolchain - tools from 4.x toolchain, but use sysroot
# of 3.x toolchain. This allows to run the executable for instance on
# a Dreambox.
#

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Root where toolchains and sysroots are stored
set(root "/Volumes/xtools")
set(root_3x "/Volumes/xtools 1")
# Toolchain ID for tools used for compilation
set(id "armv8-rpi3-linux-gnueabihf")
# Toolchain ID for sysroot (on destination system)
set(sysroot_id "arm-none-linux-gnueabi")

# sysroot ID from toolchain older system (for 3.x kernel)
set(CMAKE_SYSROOT "${root_3x}/${sysroot_id}/${sysroot_id}/sysroot/")

set(tools ${root}/${id})
set(CMAKE_C_COMPILER ${tools}/bin/${id}-gcc)
set(CMAKE_CXX_COMPILER ${tools}/bin/${id}-g++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
