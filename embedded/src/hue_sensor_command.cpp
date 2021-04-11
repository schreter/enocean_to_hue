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

#include "hue_sensor_command.hpp"

#include <cstdio>
#include <cstring>

void hue_sensor_command::post(int32_t value)
{
  if (queue_size_ == MAX_QUEUE_SIZE) {
    // drop the oldest
    memmove(&queue_[0], &queue_[1], sizeof(queue_[0]) * (MAX_QUEUE_SIZE - 1));
    --queue_size_;
  }
  auto& q = queue_[queue_size_++];
  q.value = value;
  q.timestamp = timestamp();
  if (state_ == state::idle) {
    request_finished(); // will start next connect
  }
}

bool hue_sensor_command::prepare_buffer(timestamp_t timestamp) noexcept
{
  // pick first valid element and prepare buffer with it
  // return true, if valid, false on empty queue
  for (uint8_t i = 0; i < queue_size_; ++i) {
    auto& q = queue_[i];
    auto delta = timestamp - q.timestamp;
    if (delta >= 0 && delta <= MAX_EVENT_AGE) {
      // use this one
      char tmp[64];
      auto cl = snprintf(tmp, sizeof(tmp),
            "{\"state\":{\"status\": %d}}", q.value);
      send_total_size_ = send_outstanding_size_ = uint16_t(snprintf(
            buffer_, sizeof(buffer_),
            "PUT /api/%s/sensors/%d HTTP/1.1\r\n"
            "Host: %d.%d.%d.%d\r\n"
            "Accept: */*\r\n"
            "User-Agent: enocean-gw/0.1\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n%s",
            api_key_, sensor_id_,
            ip_ & 0xff, (ip_ >> 8) & 0xff, (ip_ >> 16) & 0xff, ip_ >> 24,
            cl, tmp));
      send_ptr_ = reinterpret_cast<const uint8_t*>(buffer_);
      if (++i < queue_size_)
        memmove(&queue_[0], &queue_[i], sizeof(queue_[0]) * (queue_size_ - i));
      queue_size_ -= i;
      return true;
    }
  }

  // didn't find any usable
  queue_size_ = 0;
  return false;
}

void hue_sensor_command::request_finished() noexcept
{
  state_ = state::idle;
  if (prepare_buffer(timestamp())) {
    // next request pending
    if (start_connect())
      state_ = state::sending;
    else if (state_ == state::idle)
      state_ = state::connecting;
  }
}

