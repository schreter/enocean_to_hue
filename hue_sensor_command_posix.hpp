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
 * @brief Command handler to post commands to Hue bridge via virtual sensor using POSIX sockets.
 */
#pragma once

#include "embedded/hue_sensor_command.hpp"

class hue_sensor_command_posix : public hue_sensor_command
{
public:
  /*!
   * @brief Construct command hander to send commands to the Hue bridge via a sensor.
   *
   * @param ip IP address of the bride.
   * @param api_key API key assigned by the bridge.
   * @param sensor_id sensor ID assigned by the bridge.
   * @param group_id group ID on which to react.
   *
   * The command handler PUTs to the Sensor URL in form
   * <tt>http://&lt;ip&gt;/api/&lt;api_key&gt;/sensors/&lt;sensor_id&gt;</tt>.
   * It PUTs JSON document in the form:
   * <tt>{"state":{"status": &lt;value&gt;}}</tt>
   */
  explicit hue_sensor_command_posix(uint32_t ip, const char* api_key, int sensor_id, int group_id) noexcept :
    hue_sensor_command(ip, api_key, sensor_id, group_id)
  {}

  /// Get FD to poll on, if any.
  int get_fd() const noexcept { return fd_; }

  /// Get events to poll for.
  short get_events() const noexcept;

  /// Process events on file descriptor.
  virtual void poll() override;

private:
  /// Get current timestamp.
  virtual int64_t timestamp() noexcept override;

  /// Start connecting to the remote side.
  virtual bool start_connect() override;

  /// File descriptor of the current connection, if any.
  int fd_ = -1;
};
