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

#include "command_mapping.hpp"

#include <fstream>
#include <sstream>
#include <system_error>

#include <arpa/inet.h>

command_mapping::command_mapping()
{
  // NOP for now
}

uint32_t command_mapping::map(const enocean_event& e)
{
  if (e.hdr.packet_type == enocean_packet_type::RADIO_ERP1) {
    switch (e.erp1.event_type) {
      case enocean_erp1_type::SWITCH:
      {
        auto button = e.erp1.switch_event.button_id();
        auto id = e.erp1.switch_event.sender;
        auto i = mapping_.find(std::make_pair(id, button));
        if (i != mapping_.end())
          return i->second;
        break;
      }
      case enocean_erp1_type::CONTACT:
      {
        bool closed = e.erp1.contact_event.is_closed();
        auto id = e.erp1.contact_event.sender;
        auto i = mapping_.find(std::make_pair(id, closed ? 1 : 0));
        if (i != mapping_.end())
          return i->second;
        break;
      }
      default:
        break;
    }
  }
  return 0;
}

void command_mapping::add_mapping(enocean_id id, int8_t button, int32_t value, uint32_t group)
{
  if (!value)
    throw std::runtime_error("Value must be specified");
  if (button < -2 || button > 8)
    throw std::runtime_error("Button ID must be in range [-2,8]");

  if (button < 0) {
    for (button = ((button == -2) ? 0 : 1); button <= 8; ++button)
      add_mapping(id, button, value + button, group);
    return;
  }
  mapping_.emplace(std::make_pair(id, button), uint32_t(value) | group);
  printf("Added mapping for %x: %d -> %d/%u\n", ntohl(id.raw()), button, value, group);
}

void command_mapping::load(const char* filename)
{
  std::ifstream infile(filename, std::ios_base::in);
  if (!infile.is_open())
    throw std::runtime_error("Cannot open mapping file");
  std::string line;
  uint32_t group = 0;
  while (std::getline(infile, line)) {
    if (line.length() == 0 || line[0] == '#')
      continue;
    unsigned a, b, c, d;
    int button, value;
    auto str = line.c_str();
    uint32_t new_group;
    if (1 == sscanf(str, "group %u", &new_group))
    {
      // group set
      group = new_group << 24;
      continue;
    }
    auto res = sscanf(str, "%x:%x:%x:%x %d %d", &a, &b, &c, &d, &button, &value);
    if (res != 6)
      throw std::runtime_error("Expected line in form XX:XX:XX:XX # #####");
    if (a > 255 || b > 255 || c > 255 || d > 255)
      throw std::runtime_error("ID must contain only hexadecimal values up to 0xff");
    if (button < -2 || button > 8)
      throw std::runtime_error("Button ID must be in range [-2,8]");
    enocean_id id;
    id.set(uint8_t(a), uint8_t(b), uint8_t(c), uint8_t(d));
    add_mapping(id, int8_t(button), value, group);
  }
}
