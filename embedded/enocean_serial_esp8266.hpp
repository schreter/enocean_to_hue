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
 * @brief Serial port handling using software serial on ESP8266 via port D6/D7.
 */
#pragma once

#include "enocean_serial.hpp"

// Define to remap Serial0 to RX/TX on D7/D8 to use HW serial
#define ENOCEAN_USE_SERIAL0

#ifndef ENOCEAN_USE_SERIAL0
#include <SoftwareSerial.h>
#endif

class enocean_serial_esp final : public enocean_serial
{
public:
  /*!
   * @brief Initialize serial port.
   */
  explicit enocean_serial_esp(void (*handler)(const enocean_event& event));

  /// Handle any received data by pushing them to event state machine.
  virtual void poll() override;

private:
  virtual void handle_event(const enocean_event& event) override;

#ifndef ENOCEAN_USE_SERIAL0
  SoftwareSerial port_;
#endif
  void (*handler_)(const enocean_event& event);
};
