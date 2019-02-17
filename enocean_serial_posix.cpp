/*
 * Copyright (C) 2018-2019 Ivan Schréter (schreter@gmx.net)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This copyright notice MUST APPEAR in all copies of the software!
 */

#include "enocean_serial_posix.hpp"

#include <system_error>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

enocean_serial_posix::enocean_serial_posix(const char* device_path)
{
  int fd = ::open(device_path, O_RDWR | O_NONBLOCK);
  if (fd < 0)
    throw std::system_error(
        std::error_code(errno, std::generic_category()),
        std::string("Cannot open device '") + device_path + '\'');

  struct termios tio;

  if (tcgetattr(fd, &tio) < 0)
    throw std::system_error(
        std::error_code(errno, std::generic_category()),
        std::string("Cannot get attributes of serial device '") + device_path + '\'');
  cfmakeraw(&tio);
  if (cfsetspeed(&tio, B57600) < 0)
    throw std::system_error(
        std::error_code(errno, std::generic_category()),
        std::string("Cannot set speed of serial device '") + device_path + '\'');
  if (tcsetattr(fd, TCSANOW, &tio) < 0)
    throw std::system_error(
        std::error_code(errno, std::generic_category()),
        std::string("Cannot set attributes of serial device '") + device_path + '\'');

  fd_ = fd;
}

enocean_serial_posix::~enocean_serial_posix() noexcept
{
  if (fd_ >= 0)
    ::close(fd_);
}

void enocean_serial_posix::poll()
{
  for (;;)
  {
    uint8_t buffer[256];
    auto res = ::read(fd_, buffer, sizeof(buffer));
    if (res == 0)
      throw std::runtime_error("EOF on serial line");
    if (res < 0)
    {
      auto err = errno;
      if (err == EAGAIN)
        return;   // spurious event or end of data
      else if (err == EINTR)
        continue; // signal, retry
      throw std::system_error(
          std::error_code(err, std::generic_category()),
          "Error reading data on serial line");
    }
    // got data, push them
    auto p = buffer;
    while (res--)
      push(*p++);
  }
}

