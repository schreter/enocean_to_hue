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
  virtual ~enocean_serial() noexcept;

  /// Handle any received data by pushing them to event state machine.
  virtual void poll() = 0;

protected:
  /// Push a byte to process.
  void push(uint8_t b);

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

  /// Event to fill.
  enocean_event event_;

  /// Current processing state.
  state state_ = state::wait_sync;
  /// Size to receive.
  uint16_t receive_size_ = 0;
  /// Pointer where to receive.
  uint8_t* receive_ptr_ = nullptr;
};

