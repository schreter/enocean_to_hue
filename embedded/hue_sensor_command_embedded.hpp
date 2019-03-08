/*
 * Copyright (C) 2019 Ivan Schréter (schreter@gmx.net)
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
 * @brief Bridge connection handling.
 */
#pragma once

#include "hue_sensor_command.hpp"

struct tcp_pcb;
struct pbuf;

extern "C" {
  #include "lwip/err.h"
}

class hue_sensor_command_embedded : public hue_sensor_command
{
public:
  /*!
   * @brief Construct command hander to send commands to the Hue bridge via a sensor.
   *
   * @param ip IP address of the bride.
   * @param api_key API key assigned by the bridge.
   * @param sensor_id sensor ID assigned by the bridge.
   * @param group group for which to post to bridge.
   *
   * The command handler PUTs to the Sensor URL in form
   * <tt>http://&lt;ip&gt;/api/&lt;api_key&gt;/sensors/&lt;sensor_id&gt;</tt>.
   * It PUTs JSON document in the form:
   * <tt>{"state":{"status": &lt;value&gt;}}</tt>
   */
  explicit hue_sensor_command_embedded(uint32_t ip = 0, const char* api_key = "", int sensor_id = 0, int group_id = -1) noexcept :
    hue_sensor_command(ip, api_key, sensor_id, group_id)
  {}

  /// Process events on file descriptor.
  virtual void poll() override;

  /// Get current timestamp.
  virtual timestamp_t timestamp() noexcept override;

private:
  /// Start connecting to the remote side.
  virtual bool start_connect() override;

  /// Callback on connection error.
  static void connection_error(void* arg, err_t err);

  /// Callback on connection established.
  static err_t connection_established(void* arg, tcp_pcb* tpcb, err_t err);

  /// Callback on data received.
  static err_t data_received(void* arg, tcp_pcb* tpcb, pbuf* p, err_t err);

  /// Socket.
  tcp_pcb* pcb_ = nullptr;
  /// Force restarting connect in poll(), if it failed due to OOM.
  bool restart_connect_ = false;
  /// Time of the connect.
  unsigned long connect_time_ = 0;
  /// Time when to start reporting slow connection.
  unsigned long report_time_ = 0;
};
