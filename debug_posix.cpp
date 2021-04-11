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

#include "debug.hpp"

#include <iostream>

debug_stream& debug_stream::instance() noexcept
{
  static debug_stream instance;
  return instance;
}

debug_stream& debug_stream::operator<<(const char* value) { std::cerr << value; return *this; }
debug_stream& debug_stream::operator<<(char value) { std::cerr << value; return *this; }
debug_stream& debug_stream::operator<<(unsigned char value) { std::cerr << value; return *this; }
debug_stream& debug_stream::operator<<(int32_t value) { std::cerr << value; return *this; }
debug_stream& debug_stream::operator<<(uint32_t value) { std::cerr << value; return *this; }
debug_stream& debug_stream::operator<<(int64_t value) { std::cerr << value; return *this; }
debug_stream& debug_stream::operator<<(uint64_t value) { std::cerr << value; return *this; }

void debug_stream::write(const void* ptr, size_t size)
{
  std::cerr.write(reinterpret_cast<const char*>(ptr), size);
}

void debug_stream::set_showbase(bool s) noexcept
{
  if (s)
    std::cerr << std::showbase;
  else
    std::cerr << std::noshowbase;
}

void debug_stream::set_hex(bool s) noexcept
{
  if (s)
    std::cerr << std::hex;
  else
    std::cerr << std::dec;
}

void debug_stream::endl()
{
  std::cerr << std::endl;
}
