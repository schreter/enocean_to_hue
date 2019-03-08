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

#include "enocean_to_hue_bridge.hpp"

// Define to prevent answering proxy calls and handle only own group ID.
//#define NO_PROXY

#include <system_error>
#include <poll.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>

enocean_to_hue_bridge::enocean_to_hue_bridge(
    const char* port,
    uint32_t bridge_ip,
    const char* api_key,
    int sensor_id,
    const char* map_file,
    int group_id) :
  cmd_(bridge_ip, api_key, sensor_id, -1),
  hnd_(port, *this),
  own_group_(group_id)
{
  map_.load(map_file);
#ifndef NO_PROXY
  proxy_server_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (proxy_server_fd_ < 0)
    throw std::system_error(errno, std::generic_category(), "Cannot open proxy socket");

  auto flags = fcntl(proxy_server_fd_, F_GETFL);
  if (flags < 0) {
    auto err = errno;
    close(proxy_server_fd_);
    proxy_server_fd_ = -1;
    throw std::system_error(
        std::error_code(err, std::generic_category()),
        "Cannot get proxy socket's flags");
  }
  if (fcntl(proxy_server_fd_, F_SETFL, flags | O_NONBLOCK) < 0) {
    auto err = errno;
    close(proxy_server_fd_);
    proxy_server_fd_ = -1;
    throw std::system_error(
        std::error_code(err, std::generic_category()),
        "Cannot set proxy socket's flags");
  }

  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family    = AF_INET; // IPv4
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(22554);
  if (bind(proxy_server_fd_, reinterpret_cast<const struct sockaddr*>(&servaddr), sizeof(servaddr)) < 0)
  {
    auto err = errno;
    close(proxy_server_fd_);
    proxy_server_fd_ = -1;
    throw std::system_error(err, std::generic_category(), "Cannot bind proxy socket");
  }
#endif
}

void enocean_to_hue_bridge::run_poll_loop()
{
  for (;;)
  {
    struct pollfd fds[3];
    nfds_t cnt = 1;
    fds[0].fd = hnd_.get_fd();
    fds[0].events = POLLERR | POLLIN;
    fds[0].revents = 0;
#ifndef NO_PROXY
    fds[1].fd = proxy_server_fd_;
    fds[1].events = POLLERR | POLLIN;
    fds[1].revents = 0;
    ++cnt;
#endif
    fds[cnt].fd = cmd_.get_fd();
    if (fds[cnt].fd >= 0) {
      fds[cnt].events = cmd_.get_events() | POLLERR;
      fds[cnt].revents = 0;
      ++cnt;
    }
    auto res = poll(fds, cnt, -1);
    if (res < 0) {
      if (errno == EINTR || errno == EAGAIN)
        continue; // interrupted by signal or out of resources, retry
      throw std::system_error(
          std::error_code(errno, std::generic_category()),
          "Error polling file descriptors");
    }
    if (fds[0].revents)
      hnd_.poll();
#ifndef NO_PROXY
    if (fds[1].revents)
      proxy_poll();
    if (cnt > 2 && fds[2].revents)
      cmd_.poll();
#else
    if (cnt > 1 && fds[1].revents)
      cmd_.poll();
#endif
  }
}

void enocean_to_hue_bridge::handler::handle_event(const enocean_event& event)
{
  parent_.handle_event(event);
}

static void hexdump(char* dest, size_t dest_rem, const void* ptr, size_t size) noexcept
{
  if (!size || dest_rem < 4) {
    dest[0] = 0;
    return;
  }
  auto p = reinterpret_cast<const uint8_t*>(ptr);
  while (size-- && dest_rem >= 3) {
    auto c = *p++;
    static constexpr char hexchar[17] = "0123456789ABCDEF";
    dest[0] = hexchar[c >> 4];
    dest[1] = hexchar[c & 15];
    dest[2] = ' ';
    dest += 3;
    dest_rem -= 3;
  }
  dest[-1] = 0;
}

void enocean_to_hue_bridge::handle_event(const enocean_event& event, uint32_t remote_ip)
{
  uint32_t addr = 0;
  int8_t button = 0;
  switch (event.erp1.event_type) {
    case enocean_erp1_type::CONTACT:
      addr = event.erp1.contact_event.sender.raw();
      button = event.erp1.contact_event.is_closed() ? 1 : 0;
      break;
    case enocean_erp1_type::SWITCH:
      addr = event.erp1.switch_event.sender.raw();
      button = event.erp1.switch_event.button_id();
      break;
  }

  auto value = map_.map(event);
  auto group = value >> 24;
  value &= 0xffffff;

  char data[128];
  hexdump(data, sizeof(data), &event.buffer, event.hdr.total_size());

  ++total_event_count_;
  if (own_group_ == int32_t(group))
    ++our_event_count_;
  auto ip_addr = reinterpret_cast<const unsigned char*>(&remote_ip);
  auto dbm = event.erp1.contact_event.subtel[0].dbm;
  auto ts = cmd_.timestamp();
  syslog(LOG_INFO,
      "EnOcean event, addr %x, button %d => ID %d@%d/%d, RSSI -%u, index %lu/%lu, ts %lld, source %u.%u.%u.%u, data %s",
      addr, button, value, group, cmd_.get_group_id(), dbm, our_event_count_, total_event_count_,
      ts, ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], data);

  //printf("\aGot event %d, type=%d: %s", ++count, int(event.hdr.packet_type), data);
  if (value) {
#ifdef NO_PROXY
    syslog(LOG_INFO, "EnOcean post command: %d, group %d", value, group);
    cmd_.post(value, group);
#else
    auto id = int32_t(value);
    // The same command can be received by multiple receivers. So we are posting only
    // in case enough time has passed since the occurrence of the same command.
    // Typically, all commands arrive within a millisecond or so, so let's test for
    // 200 ms time difference to out filter duplicates. This gives us 5 commands/second
    // from the same switch. Nobody is going to press the switch that fast :-).
    auto res = command_states_.emplace(addr, std::make_pair(ts, id));
    bool do_send = false;
    if (res.second) {
      // new entry
      do_send = true;
    } else {
      // check ID or time difference
      auto last_id = res.first->second.second;
      auto last_ts = res.first->second.first;
      if (uint64_t(ts - last_ts) >= 200 || last_id != id) {
        do_send = true;
      }
      res.first->second.second = last_id;
      res.first->second.first = ts;
    }
    if (do_send) {
      syslog(LOG_INFO, "EnOcean post command: %d, group %d", value, group);
      cmd_.post(id, 0);
    }
#endif
  }
}

void enocean_to_hue_bridge::proxy_poll()
{
  uint64_t buffer[128];
  struct sockaddr_in remote;
  for (;;) {
    socklen_t addr_len = sizeof(remote);
    auto msg_len = recvfrom(proxy_server_fd_, &buffer, sizeof(buffer),
        MSG_WAITALL, reinterpret_cast<struct sockaddr*>(&remote), &addr_len);
    if (msg_len < 0) {
      if (errno == EAGAIN)
        return; // no data
      throw std::system_error(
          std::error_code(errno, std::generic_category()),
          "Error polling data from proxy socket");
    }
    handle_event(*reinterpret_cast<enocean_event*>(&buffer), remote.sin_addr.s_addr);
  }
}
