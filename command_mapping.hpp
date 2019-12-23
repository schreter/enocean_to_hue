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

#pragma once

#include "embedded/enocean.hpp"

#include <map>

/*!
 * @brief Command mapping class from Enocean events to Hue sensor value.
 */
class command_mapping
{
public:
  command_mapping();

  /*!
   * @brief Map an event to a value.
   *
   * @param e received event.
   * @return command value to send to Hue bridge or 0 if no mapping.
   */
  uint32_t map(const enocean_event& e);

  /*!
   * @brief Add a new mapping.
   *
   * @param id Enocean ID to map.
   * @param button button pressed to map (1-8; 0 for release, -1 for all
   *    buttons as value + button, -2 as -1 + button release as value).
   * @param value value to send for the button.
   * @param bridge_set set of bridges to send button value to (high 8 bits, as bitmask).
   */
  void add_mapping(enocean_id id, int8_t button, int32_t value, uint32_t bridge_set);

  /*!
   * @brief Load mappings from a file.
   *
   * The file is in the format:
   * <pre>
   * ID button value
   * </pre>
   *
   * ID is Enocean ID in form aa:bb:cc:dd and button is button pressed
   * (1-8; 0 for release or -1 to map all buttons to values starting with
   * the specified value + button number, -2 as -1, but also map button
   * release to the specified value). Value specifies value to send when this
   * button is detected (or base for value range if mapping all buttons).
   *
   * The file can contain empty lines and comments starting with '#'.
   *
   * @param filename file to read.
   */
  void load(const char* filename);

private:
  /// Mapping to use.
  std::map<std::pair<enocean_id, uint8_t>, uint32_t> mapping_;
};
