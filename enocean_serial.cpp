/*
 * Copyright (C) 2018 Ivan Schréter (schreter@gmx.net)
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

#include "enocean_serial.hpp"
#include "crc8.hpp"

#include <system_error>
#include <iostream>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

namespace
{
  static void hexdump(std::ostream& stream, const void* ptr, size_t size) noexcept
  {
    stream << std::showbase << std::hex;
    auto p = reinterpret_cast<const uint8_t*>(ptr);
    while (size--)
      stream << ' ' << *p++;
  }
}

enocean_serial::enocean_serial(const char* device_path)
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

enocean_serial::~enocean_serial() noexcept
{
  if (fd_ >= 0)
    ::close(fd_);
}

void enocean_serial::poll()
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

void enocean_serial::push(uint8_t b)
{
  switch (state_)
  {
  case state::wait_sync:
  default:
    if (b == 0x55)
    {
      state_ = state::wait_header;
      receive_size_ = sizeof(event_.hdr);
      receive_ptr_ = reinterpret_cast<uint8_t*>(&event_.hdr);
    }
    break;

  case state::wait_header:
    *receive_ptr_++ = b;
    if (--receive_size_ == 0)
    {
      // process header
      auto cksum = crc8::checksum(&event_.hdr, sizeof(event_.hdr));
      if (cksum != 0)
      {
        cksum = event_.hdr.header_crc8;
        event_.hdr.header_crc8 = 0;
        auto exp_cksum = crc8::checksum(&event_.hdr, sizeof(event_.hdr));
        event_.hdr.header_crc8 = cksum;
        std::cerr << "Header checksum error: expected: " <<
                     std::hex << std::showbase <<
                     exp_cksum << ", found: " << cksum <<
                     ", header data:";
        hexdump(std::cerr, &event_.hdr, sizeof(event_.hdr));
        std::cerr << std::endl;
        state_ = state::wait_sync;
        break;
      }

      // Checksum OK, receive data
      state_ = state::wait_data;
      receive_ptr_ = event_.buffer;
      receive_size_ = event_.hdr.total_size();
      if (receive_size_ > enocean_event::BUF_SIZE) {
        std::cerr << "Header with too big size " << std::dec << receive_size_ <<
                     ", trying to resync" << std::endl;
        state_ = state::wait_sync;
        break;
      }

    }
    break;

  case state::wait_data:
    *receive_ptr_++ = b;
    if (--receive_size_ == 0)
    {
      // got entire data
      state_ = state::wait_sync;

      auto read_size = event_.hdr.total_size();
      auto cksum = crc8::checksum(&event_.buffer, read_size);
      if (cksum != 0)
      {
        cksum = event_.buffer[read_size - 1];
        event_.buffer[read_size - 1] = 0;
        auto exp_cksum = crc8::checksum(&event_.buffer, read_size);
        event_.buffer[read_size - 1] = cksum;
        std::cerr << "Data checksum error: expected: " <<
                     std::hex << std::showbase <<
                     exp_cksum << ", found: " << cksum <<
                     ", data:";
        hexdump(std::cerr, &event_.buffer, read_size);
        std::cerr << std::endl;
      }
      else
      {
        // all OK
        handle_event(event_);
      }
    }
    break;
  }
}
