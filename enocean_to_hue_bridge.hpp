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

/*!
 * @file
 * @brief Bridge to translate Enocean sensors to Hue bridge's internal sensor.
 */
#pragma once

#include "enocean_serial_posix.hpp"
#include "hue_sensor_command_posix.hpp"
#include "command_mapping.hpp"

#include <map>
#include <deque>

/*!
 * @brief Bridge to translate Enocean sensors to Hue bridge's internal sensor.
 */
class enocean_to_hue_bridge
{
public:
  /// Construct the bridge object.
  enocean_to_hue_bridge(
      const char* port,
      std::deque<hue_sensor_command_posix>& bridges,
      const char* map_file);

  /// Run poll loop forever.
  [[noreturn]] void run_poll_loop();

private:
  /// Handler for Enocean events.
  class handler : public enocean_serial_posix
  {
  public:
    handler(const char* port, enocean_to_hue_bridge& parent) :
      enocean_serial_posix(port), parent_(parent)
    {}

  private:
    virtual void handle_event(const enocean_event& event) override;

    enocean_to_hue_bridge& parent_;
  };

  void handle_event(const enocean_event& event, uint32_t remote_ip = 0);

  void proxy_poll();

  command_mapping map_;
  std::deque<hue_sensor_command_posix>& bridges_;
  handler hnd_;
  std::map<int32_t, std::pair<hue_sensor_command::timestamp_t, int32_t>> command_states_;
  int proxy_server_fd_ = -1;
  unsigned long total_event_count_ = 0;
};
