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

// This file is used for embedded version of the bridge running on ESP8266

#ifdef ARDUINO

#include "enocean_serial_esp8266.hpp"

#include <ESP.h>

enocean_serial_esp::enocean_serial_esp(void (*handler)(const enocean_event& event)) :
#ifndef ENOCEAN_USE_SERIAL0
  port_(D7, D8),
#endif
  handler_(handler)
{
#ifndef ENOCEAN_USE_SERIAL0
  port_.begin(57600);
#endif
  // else we assume Serial is already initialized to 57600
}

/// Handle any received data by pushing them to event state machine.
void enocean_serial_esp::poll()
{
#ifdef ENOCEAN_USE_SERIAL0
  if (Serial.available())
    push(Serial.read());
#else
  if (port_.available())
    push(port_.read());
#endif
}

void enocean_serial_esp::handle_event(const enocean_event& event)
{
  handler_(event);
}

#endif
