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

#include "hue_sensor_command_posix.hpp"

#include <system_error>

#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

short hue_sensor_command_posix::get_events() const noexcept
{
  if (get_state() != state::receiving)
    return POLLOUT;
  else
    return POLLIN;
}

void hue_sensor_command_posix::poll()
{
  for (;;) {
    switch (get_state()) {
    case state::connecting:
      connected();
      // fall throught to send first block
    case state::sending:
    {
      // socket writable, write remaining stuff
      auto res = ::write(fd_, send_ptr_, send_outstanding_size_);
      if (res < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
          // cannot write any more data
          return; // will retry later
        } else if (errno == EINTR) {
          continue;
        }
        throw std::system_error(
            std::error_code(errno, std::generic_category()),
            "Error writing data to socket");
      } else if (res == send_outstanding_size_) {
        // all written
        request_sent();
        continue;
      } else if (res < send_outstanding_size_) {
        send_outstanding_size_ -= res;
        send_ptr_ += res;
        continue;
      } else {
        // impossible
        throw std::runtime_error("Wrote too much data to the socket");
      }
    }
    case state::receiving:
    {
      auto res = ::read(fd_, buffer_, sizeof(buffer_));
      if (res < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
          // will retry later
          return;
        } else if (errno == EINTR) {
          continue;
        }
        throw std::system_error(
            std::error_code(errno, std::generic_category()),
            "Error reading data from socket");
      } else if (res == 0) {
        // EOF
        close(fd_);
        fd_ = -1;
        request_finished();
        return;
      } else {
        // more data pending
        continue;
      }
    }
    default:
      // ignore
      return;
    }
  }
}

hue_sensor_command::timestamp_t hue_sensor_command_posix::timestamp() noexcept
{
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return tp.tv_nsec / 1000000 + tp.tv_sec * 1000;
}

bool hue_sensor_command_posix::start_connect()
{
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_ < 0)
    throw std::system_error(
        std::error_code(errno, std::generic_category()),
        "Cannot create socket for sending Hue request");

  int one = 1;
  if (setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) < 0)
    throw std::system_error(
        std::error_code(errno, std::generic_category()),
        "Cannot set socket's TCP_NODELAY option");

  auto flags = fcntl(fd_, F_GETFL);
  if (flags < 0)
    throw std::system_error(
        std::error_code(errno, std::generic_category()),
        "Cannot get socket's flags");

  if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0)
    throw std::system_error(
        std::error_code(errno, std::generic_category()),
        "Cannot set socket's flags");

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);
  addr.sin_addr.s_addr = ip_;
  if (connect(fd_, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) < 0) {
    if (errno != EINPROGRESS)
      throw std::system_error(
          std::error_code(errno, std::generic_category()),
          "Error connecting the socket");
    return false;
  } else {
    // immediately connected, next poll will send
    return true;
  }
}
