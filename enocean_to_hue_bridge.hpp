/*!
 * @file
 * @brief Bridge to translate Enocean sensors to Hue bridge's internal sensor.
 */
#pragma once

#include "enocean_serial.hpp"
#include "hue_sensor_command.hpp"
#include "command_mapping.hpp"

/*!
 * @brief Bridge to translate Enocean sensors to Hue bridge's internal sensor.
 */
class enocean_to_hue_bridge
{
public:
  /// Construct the bridge object.
  enocean_to_hue_bridge(
      const char* port,
      uint32_t bridge_ip,
      const char* api_key,
      int sensor_id,
      const char* map_file);

  /// Run poll loop forever.
  [[noreturn]] void run_poll_loop();

private:
  /// Handler for Enocean events.
  class handler : public enocean_serial
  {
  public:
    handler(const char* port, enocean_to_hue_bridge& parent) :
      enocean_serial(port), parent_(parent)
    {}

  private:
    virtual void handle_event(const enocean_event& event) override;

    enocean_to_hue_bridge& parent_;
  };

  void handle_event(const enocean_event& event);

  command_mapping map_;
  hue_sensor_command cmd_;
  handler hnd_;
};
