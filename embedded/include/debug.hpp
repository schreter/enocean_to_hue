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
 * @brief Debug stream class for compatibility between embedded and normal build.
 */
#pragma once

#include <cstdint>
#include <cstddef>

#ifdef ARDUINO
#include <Arduino.h>
#endif

/*!
 * @brief Debug stream class for compatibility between embedded and normal build.
 *
 * Normal build wraps std::cerr, embedded prints to serial console.
 */
class debug_stream
{
public:
  using functor = debug_stream&(*)(debug_stream&);

  static debug_stream& instance() noexcept;

  debug_stream& operator<<(const char* value);
  debug_stream& operator<<(char value);
  debug_stream& operator<<(unsigned char value);
  debug_stream& operator<<(int32_t value);
  debug_stream& operator<<(uint32_t value);
  debug_stream& operator<<(int64_t value);
  debug_stream& operator<<(uint64_t value);
  debug_stream& operator<<(functor f) { return f(*this); }

#ifdef ARDUINO
  debug_stream& operator<<(const __FlashStringHelper* value);
#endif

  void write(const void* ptr, size_t size);

private:
  friend debug_stream& showbase(debug_stream& stream);
  friend debug_stream& hex(debug_stream& stream);
  friend debug_stream& dec(debug_stream& stream);
  friend debug_stream& endl(debug_stream& stream);

  void set_showbase(bool s) noexcept;
  void set_hex(bool s) noexcept;
  void endl();
};

inline debug_stream& showbase(debug_stream& stream)
{
  stream.set_showbase(true);
  return stream;
}

inline debug_stream& hex(debug_stream& stream)
{
  stream.set_hex(true);
  return stream;
}

inline debug_stream& dec(debug_stream& stream)
{
  stream.set_hex(false);
  return stream;
}

inline debug_stream& endl(debug_stream& stream)
{
  stream.endl();
  return stream;
}
