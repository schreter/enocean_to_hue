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
 * @brief Minimum set of Enocean data structures to parse packet data.
 */
#pragma once

#include <cstdint>
#include <cstring>

/*
Example packet:

0x55 sync byte
0x00 0x07 data size
0x07 optional data size
0x01 packet type (radio ERP1)
0x7a CRC8

(variable-size data)
0xf6 switch data
0x70 - 0x30 top left, 0x10 bottom left, 0x70 top right, 0x50 bottom right, 0x0 - released
    - 3/1/7/5 set in high nibble or both high and low nibble, if two buttons pressed
0xfe 0xf5 0xde 0xbd sender ID
0x30 status - 0x30 pressed, 0x20 released

0x00 subtelegram ID
0xff 0xff 0xff 0xff destination ID
0x55 dBm(?)
0x70 security level(?)

0x74 CRC8
*/

#ifdef ARDUINO
#include "lwip/def.h"
#else
#include <arpa/inet.h>
#endif

/// ID of a sensor or actor in the network.
struct enocean_id
{
  void set(uint8_t a, uint8_t b, uint8_t c, uint8_t d) noexcept
  {
    addr[0] = a;
    addr[1] = b;
    addr[2] = c;
    addr[3] = d;
  }

  uint32_t raw() const noexcept
  {
    uint32_t raw_id;
    memcpy(&raw_id, this, 4); // may be unaligned
    return ntohl(raw_id);
    //return *reinterpret_cast<const uint32_t*>(this);
  }

  friend bool operator<(const enocean_id& l, const enocean_id& r) noexcept
  {
    return l.raw() < r.raw();
  }

  friend bool operator==(const enocean_id& l, const enocean_id& r) noexcept
  {
    return l.raw() == r.raw();
  }

  friend bool operator!=(const enocean_id& l, const enocean_id& r) noexcept
  {
    return l.raw() != r.raw();
  }

  uint8_t addr[4];
};

/// Packet types.
enum class enocean_packet_type : uint8_t
{
  RADIO_ERP1          = 0x01,   ///< Radio telegram
  RESPONSE            = 0x02,   ///< Response to any packet
  RADIO_SUB_TEL       = 0x03,   ///< Radio subtelegram
  EVENT               = 0x04,   ///< Event message
  COMMON_COMMAND      = 0x05,   ///< Common command
  SMART_ACK_COMMAND   = 0x06,   ///< Smart Ack command
  REMOTE_MAN_COMMAND  = 0x07,   ///< Remote management command
  RADIO_MESSAGE       = 0x09,   ///< Radio message
  RADIO_ERP2          = 0x0a    ///< ERP2 protocol radio telegram
};

/// Packet header (past sync byte).
struct enocean_header
{
  uint8_t data_size_h;              ///< High byte of data size.
  uint8_t data_size_l;              ///< Low byte of data size.
  uint8_t optional_data_size;       ///< Size of optional data.
  enocean_packet_type packet_type;  ///< Packet type.
  uint8_t header_crc8;              ///< CRC8 of the above fields.

  /// Determine total data size after the header.
  uint16_t total_size() const noexcept
  {
    return data_size_h * 256 + data_size_l + optional_data_size + 1;
  }
};

/// Types for RADIO_ERP1 event types.
enum class enocean_erp1_type : uint8_t
{
  /// 4-button rocker switches with up to 2 actions per event.
  SWITCH = 0xf6,
  /// Contact, e.g., magnetic window/door contact.
  CONTACT = 0xd5
};

/// Subtelegram structure.
struct enocean_subtelegram
{
  /// 0-based ID.
  uint8_t subtelegram_id;
  /// Destination, typically broadcast.
  enocean_id destination;
  /// Signal strenght.
  uint8_t dbm;
  /// Security level.
  uint8_t security_level;
};

/// Structure of switch event.
struct enocean_switch_event
{
  /// Return @c true, if pressed, @c false if released.
  bool is_pressed() const noexcept { return button_state != 0; }

  /*!
   * @brief Get button ID of the pressed button.
   *
   * @return 0 if no button or released, 1-4 for single-press and 5-8 for two-button press.
   */
  int8_t button_id() const noexcept
  {
    switch (button_state)
    {
    case 0x30: return 1;  // top left
    case 0x10: return 2;  // bottom left
    case 0x70: return 3;  // top right
    case 0x50: return 4;  // bottom right
    case 0x37: return 5;  // both top buttons
    case 0x15: return 6;  // both bottom buttons
    case 0x35: return 7;  // top left and bottom right buttons
    case 0x17: return 8;  // top right and bottom left buttons
    default: return 0;    // invalid combination or no buttons
    }
  }

  /// Event type, always SWITCH.
  enocean_erp1_type event_type;
  /// Button states.
  uint8_t button_state;
  /// ID of the switch.
  enocean_id sender;
  /// Status (0x30 pressed, 0x20 released).
  uint8_t status;

  /// Subtelegrams. We expect exactly one (7B of optional data).
  enocean_subtelegram subtel[1];
};

/// Structure of contact event.
struct enocean_contact_event
{
  /// Return @c true, if closed, @c false if open.
  bool is_closed() const noexcept { return (contact_state & 1) != 0; }

  /// Event type, always CONTACT.
  enocean_erp1_type event_type;
  /// Button states.
  uint8_t contact_state;
  /// ID of the switch.
  enocean_id sender;
  /// Status (0x30 pressed, 0x20 released).
  uint8_t status;

  /// Subtelegrams. We expect exactly one (7B of optional data).
  enocean_subtelegram subtel[1];
};

/// Event structure for RADIO_ERP1
struct enocean_erp1_event
{
  union
  {
    /// Event type to differentiate.
    enocean_erp1_type event_type;
    /// Event of a switch.
    enocean_switch_event switch_event;
    /// Event of a contact.
    enocean_contact_event contact_event;
  };
};

/// Event received from the serial interface.
struct enocean_event
{
  /// Maximum size of one event data.
  static constexpr uint16_t BUF_SIZE = 512 - sizeof(enocean_header);

  /// Packet header.
  enocean_header hdr;
  union
  {
    /// Buffer with remaining data.
    uint8_t buffer[BUF_SIZE];
    /// RARIO_ERP1 event.
    enocean_erp1_event erp1;
  };
};

