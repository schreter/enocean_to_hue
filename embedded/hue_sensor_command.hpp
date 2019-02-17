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
 * @brief Command handler to post commands to Hue bridge via virtual sensor.
 */
#pragma once

#include <cstdint>

/*!
 * @brief Command handler to post commands to Hue bridge via virtual sensor.
 *
 * Create a sensor by posting to /api/&lt;api_key&gt;/sensors:
 * <pre>
 * {
 *   "state": {
 *     "status": 0
 *   },
 *   "config": {
 *     "on": true
 *   },
 *   "name": "ExternalInput",
 *   "type": "CLIPGenericStatus",
 *   "modelid": "GenericCLIP",
 *   "manufacturername": "Philips",
 *   "swversion": "1.0",
 *   "uniqueid": "external_input",
 *   "recycle": false
 * }
 * </pre>
 *
 * Use created sensor ID as constructor argument. Posting to the command
 * handler, you can change value of the sensor. Rules can then react on
 * the change of the sensor value.
 */
class hue_sensor_command
{
public:
#ifdef ARDUINO
  using timestamp_t = int32_t;
#else
  using timestamp_t = int64_t;
#endif

  /*!
   * @brief Construct command hander to send commands to the Hue bridge via a sensor.
   *
   * @param ip IP address of the bride.
   * @param api_key API key assigned by the bridge.
   * @param sensor_id sensor ID assigned by the bridge.
   *
   * The command handler PUTs to the Sensor URL in form
   * <tt>http://&lt;ip&gt;/api/&lt;api_key&gt;/sensors/&lt;sensor_id&gt;</tt>.
   * It PUTs JSON document in the form:
   * <tt>{"state":{"status": &lt;value&gt;}}</tt>
   */
  explicit hue_sensor_command(uint32_t ip, const char* api_key, int sensor_id) noexcept :
    ip_(ip), api_key_(api_key), sensor_id_(sensor_id)
  {}

  virtual ~hue_sensor_command() noexcept {}

  /*!
   * @brief Post a value to the sensor.
   *
   * The queue with values is not arbitrarily long. Only recent values
   * will be actually posted, too old values will be lost, if not confirmed
   * by the bridge in time.
   *
   * @param value value to post.
   */
  void post(int32_t value);

  /// Process events on file descriptor.
  virtual void poll() = 0;

private:
  /// Get current timestamp.
  virtual timestamp_t timestamp() noexcept = 0;

  /// Start connecting to the remote side. Returns true if connected immediately.
  virtual bool start_connect() = 0;

protected:
  /// Current state of the handler.
  enum class state
  {
    idle,           ///< No connection and queue empty.
    connecting,     ///< Connecting to the remote side.
    sending,        ///< Sending data to the remote side.
    receiving,      ///< Receiving reply from the remote side.
    unknown         ///< Unknown state.
  };

  /// Element of the queue.
  struct queue_element
  {
    int32_t value;          ///< Value to post.
    timestamp_t timestamp;  ///< Milliseconds since some common point in time.
  };

  /// Get current request state.
  state get_state() const noexcept { return state_; }

  /// Prepare buffer with the first command in queue.
  bool prepare_buffer(timestamp_t timestamp) noexcept;

  /// The connection is established, send data now.
  void connected() noexcept
  {
    state_ = state::sending;
  }

  /// Request data has been sent, now receiving response.
  void request_sent() noexcept
  {
    state_ = state::receiving;
  }

  /// Inform the handler that the command has been sent and confirmed.
  void request_finished() noexcept;

  /// Inform the handler that the request failed.
  void request_failed() noexcept { request_finished(); }

  /// Maximum 4 commands in the queue.
  static constexpr auto MAX_QUEUE_SIZE = 4;
  /// Maximum 0.5 seconds for the bridge to accept the command.
  static constexpr timestamp_t MAX_EVENT_AGE = 500000;

  /// Remote IP address.
  const uint32_t ip_;
  /// API key.
  const char* const api_key_;
  /// Sensor ID to post to.
  const int sensor_id_;

  /// Send pointer.
  const uint8_t* send_ptr_ = nullptr;
  /// Outstanding send size.
  uint16_t send_outstanding_size_ = 0;

  /// Send/receive buffer with current command or response.
  char buffer_[512];

private:
  /// Current queue size.
  uint8_t queue_size_ = 0;
  /// Queue with commands to send.
  queue_element queue_[MAX_QUEUE_SIZE];

  /// Current state of the connection.
  state state_ = state::idle;
};
