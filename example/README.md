This folder contains some examples:
- `enocean_to_hue_gw` - sample init.d script for Linux systems to start/stop Enocean to Hue gateway
- `install_enocean.sh` - installation script on destination system to update the executable and configuration (in conjunction with `make_arm.sh`)
- `make_arm.sh` - local script for building the gateway for ARM systems (see also `toolchain_arm.cmake`), expects the cross-compile build to be configured in `../build.arm`
- `toolchain_arm.cmake` - sample toolchain definition file to cross-compile for ARM architecture
- `example.conf` - example configuration for switches to forward to Hue gateway