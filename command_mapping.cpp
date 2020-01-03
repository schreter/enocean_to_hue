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

std::pair<int32_t, uint8_t> command_mapping::map(const enocean_event& e)
{
  if (e.hdr.packet_type == enocean_packet_type::RADIO_ERP1) {
    switch (e.erp1.event_type) {
      case enocean_erp1_type::SWITCH:
      {
        auto button = e.erp1.switch_event.button_id();
        auto id = e.erp1.switch_event.sender;
        auto i = mapping_.find(std::make_pair(id, button));
        if (i != mapping_.end()) {
          auto res = i->second;
          auto& last = last_value_[id];
          if (i->second.first == RELEASE) {
            // special handling for button release - send last negated
            res.first = -last;
            last = 0;
          } else {
            // store value for button release
            last = res.first;
          }
          return res;
        }
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
  return std::make_pair(0, 0);
}

void command_mapping::add_mapping(enocean_id id, int8_t button, int32_t value, uint8_t bridge_set)
{
  if (value <= 0 && value != RELEASE && !(value == -1 && button == 0))
    throw std::runtime_error("Value to send must be positive");
  if (button < -3 || button > 8)
    throw std::runtime_error("Button ID must be in range [-3,8]");

  if (button < 0) {
    if (button == -3)
      add_mapping(id, 0, RELEASE, bridge_set);
    for (button = ((button == -2) ? 0 : 1); button <= 8; ++button)
      add_mapping(id, button, value + button, bridge_set);
    return;
  }
  if (button == 0 && value == -1)
    value = RELEASE;
  mapping_.emplace(std::make_pair(id, button), std::make_pair(value, bridge_set));
  last_value_[id] = 0;
  printf("Added mapping for %x: %d -> %u/%x\n", ntohl(id.raw()), button, value, bridge_set);
}

void command_mapping::load(const char* filename)
{
  std::ifstream infile(filename, std::ios_base::in);
  if (!infile.is_open())
    throw std::runtime_error("Cannot open mapping file");
  std::string line;
  uint32_t bridge_set = 1U; // defaults to single bridge #0
  while (std::getline(infile, line)) {
    if (line.length() == 0 || line[0] == '#')
      continue;
    unsigned a, b, c, d;
    int button, value;
    auto str = line.c_str();
    uint32_t br[8];
    int count;
    if ((count = sscanf(str, "bridge %u %u %u %u %u %u %u %u",
                        &br[0], &br[1], &br[2], &br[3], &br[4], &br[5], &br[6], &br[7])) > 0)
    {
      // new bridge set
      uint32_t new_bridge_set = 0;
      for (uint32_t i = 0; i < uint32_t(count); ++i) {
        if (br[i] == 0 || br[i] > 8)
          throw std::runtime_error("Expected bridge number to be between 1 and 8");
        uint32_t bit = uint32_t(1) << (br[i] - 1);
        if (new_bridge_set & bit)
            throw std::runtime_error("Expected bridge numbers to be unique");
        new_bridge_set |= bit;
      }
      bridge_set = new_bridge_set;
      continue;
    }
    auto res = sscanf(str, "%x:%x:%x:%x %d %d", &a, &b, &c, &d, &button, &value);
    if (res != 6)
      throw std::runtime_error("Expected line in form XX:XX:XX:XX # #####");
    if (a > 255 || b > 255 || c > 255 || d > 255)
      throw std::runtime_error("ID must contain only hexadecimal values up to 0xff");
    if (button < -3 || button > 8)
      throw std::runtime_error("Button ID must be in range [-3,8]");
    enocean_id id;
    id.set(uint8_t(a), uint8_t(b), uint8_t(c), uint8_t(d));
    add_mapping(id, int8_t(button), value, uint8_t(bridge_set));
  }
}
