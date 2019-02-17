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
 * @brief Serial port handling using POSIX file descriptors.
 */
#pragma once

#include "embedded/enocean_serial.hpp"

class enocean_serial_posix : public enocean_serial
{
public:
  /*!
   * @brief Initialize serial port.
   *
   * @param device_path path to the special file representing serial device.
   */
  explicit enocean_serial_posix(const char* device_path);

  virtual ~enocean_serial_posix() noexcept override;

  /// Return file descriptor to poll on.
  int get_fd() const noexcept { return fd_; }

  /// Handle any received data by pushing them to event state machine.
  virtual void poll() override;

private:
  /// Underlying file descriptor.
  int fd_ = -1;
};
