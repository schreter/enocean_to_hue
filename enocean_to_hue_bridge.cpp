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

#include <system_error>
#include <poll.h>
#include <syslog.h>

enocean_to_hue_bridge::enocean_to_hue_bridge(
    const char* port,
    uint32_t bridge_ip,
    const char* api_key,
    int sensor_id,
    const char* map_file,
    int group_id) :
  cmd_(bridge_ip, api_key, sensor_id, group_id),
  hnd_(port, *this)
{
  map_.load(map_file);
}

void enocean_to_hue_bridge::run_poll_loop()
{
  for (;;)
  {
    struct pollfd fds[2];
    nfds_t cnt = 1;
    fds[0].fd = hnd_.get_fd();
    fds[0].events = POLLERR | POLLIN;
    fds[0].revents = 0;
    fds[1].fd = cmd_.get_fd();
    if (fds[1].fd >= 0) {
      fds[1].events = cmd_.get_events() | POLLERR;
      fds[1].revents = 0;
      cnt = 2;
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
    if (fds[1].revents)
      cmd_.poll();
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

void enocean_to_hue_bridge::handle_event(const enocean_event& event)
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

  syslog(LOG_INFO, "EnOcean event, addr %x, button %d => ID %d@%d/%d, RSSI -%u, data %s",
      addr, button, value, group, cmd_.get_group_id(), event.erp1.contact_event.subtel[0].dbm,
      data);

  //printf("\aGot event %d, type=%d: %s", ++count, int(event.hdr.packet_type), data);
  if (value) {
    syslog(LOG_INFO, "Post command: %d, group %d", value, group);
    cmd_.post(value, group);
  }
}
