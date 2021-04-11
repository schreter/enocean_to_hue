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
 * @brief Enocean CRC8 implementation.
 */
#pragma once

#include <cstdint>
#include <cstddef>

/*!
 * @brief Enocean CRC8 implementation.
 */
class crc8
{
public:
  /*!
   * @brief Compute CRC8 checksum of a block.
   * @param ptr,size block to checksum.
   * @return CRC8 checksum.
   */
  static uint8_t checksum(const void* ptr, size_t size) noexcept;
};
