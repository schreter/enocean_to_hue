#pragma once

#include "enocean.hpp"

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
  int32_t map(const enocean_event& e);

  /*!
   * @brief Add a new mapping.
   *
   * @param id Enocean ID to map.
   * @param button button pressed to map (1-8; 0 for release, -1 for all
   *    buttons as value + button).
   * @param value value to send for the button.
   */
  void add_mapping(enocean_id id, int8_t button, int32_t value);

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
   * the specified value). Value specifies value to send when this
   * button is detected (or base for value range if mapping all buttons).
   *
   * The file can contain empty lines and comments starting with '#'.
   *
   * @param filename file to read.
   */
  void load(const char* filename);

private:
  /// Mapping to use.
  std::map<std::pair<enocean_id, uint8_t>, int32_t> mapping_;
};