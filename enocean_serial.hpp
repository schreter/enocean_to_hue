/*
 * Copyright (C) 2018 Ivan Schréter (schreter@gmx.net)
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
 * @brief Serial port handling.
 */
#pragma once

#include "enocean.hpp"
#include <cstdint>

/*!
 * @brief Serial port handling.
 */
class enocean_serial
{
public:
  /*!
   * @brief Initialize serial port and return file descriptor.
   *
   * @param device_path path to the special file representing serial device.
   */
  explicit enocean_serial(const char* device_path);

  virtual ~enocean_serial() noexcept;

  /// Return file descriptor to poll on.
  int get_fd() const noexcept { return fd_; }

  /// Handle any received data by pushing them to event state machine.
  void poll();

private:
  /// Receiver state.
  enum class state : uint8_t
  {
    wait_sync,    ///< Waiting for sync byte.
    wait_header,  ///< Waiting to receive header.
    wait_data,    ///< Waiting to receive event data.
    unknown
  };

  /*!
   * @brief Callback to handle received event.
   *
   * @param event event to handle.
   */
  virtual void handle_event(const enocean_event& event) = 0;

  /// Push a byte to process.
  void push(uint8_t b);

  /// Event to fill.
  enocean_event event_;

  /// Underlying file descriptor.
  int fd_ = -1;

  /// Current processing state.
  state state_ = state::wait_sync;
  /// Size to receive.
  uint16_t receive_size_ = 0;
  /// Pointer where to receive.
  uint8_t* receive_ptr_ = nullptr;
};

