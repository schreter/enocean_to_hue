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

#include "debug.hpp"
#include "enocean_serial_esp8266.hpp"

#include <Arduino.h>

static bool s_hex = false;
static bool s_base = false;

#ifdef ENOCEAN_USE_SERIAL0
#define DebugSerial Serial1
#else
#define DebugSerial Serial
#endif

debug_stream& debug_stream::instance() noexcept
{
  static debug_stream instance;
  return instance;
}

debug_stream& debug_stream::operator<<(const char* value)
{
  DebugSerial.print(value);
  return *this;
}

debug_stream& debug_stream::operator<<(const __FlashStringHelper* value)
{
  DebugSerial.print(value);
  return *this;
}

debug_stream& debug_stream::operator<<(char value)
{
  DebugSerial.print(value);
  return *this;
}

debug_stream& debug_stream::operator<<(unsigned char value)
{
  DebugSerial.print(char(value));
  return *this;
}
debug_stream& debug_stream::operator<<(int32_t value)
{
  if (value < 0) {
    DebugSerial.print('-');
    *this << uint32_t(-value);
  } else {
    *this << uint32_t(value);
  }
  return *this;
}

debug_stream& debug_stream::operator<<(uint32_t value)
{
  if (s_hex) {
    if (s_base)
      DebugSerial.print("0x");
    DebugSerial.print(value, HEX);
  } else {
    DebugSerial.print(value);
  }
  return *this;
}

#if 0
debug_stream& debug_stream::operator<<(int64_t value)
{
  if (value < 0) {
    DebugSerial.print('-');
    *this << uint64_t(-value);
  } else {
    *this << uint64_t(value);
  }
  return *this;
}

debug_stream& debug_stream::operator<<(uint64_t value)
{
  if (s_hex) {
    if (s_base)
      DebugSerial.print("0x");
    DebugSerial.print(value, HEX);
  } else {
    DebugSerial.print(value);
  }
  return *this;
}
#endif

void debug_stream::write(const void* ptr, size_t size)
{
  DebugSerial.write(reinterpret_cast<const unsigned char*>(ptr), size);
}

void debug_stream::set_showbase(bool s) noexcept
{
  s_base = s;
}

void debug_stream::set_hex(bool s) noexcept
{
  s_hex = s;
}

void debug_stream::endl()
{
  DebugSerial.println();
}

#endif
