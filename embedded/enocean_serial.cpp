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

#include "enocean_serial.hpp"
#include "crc8.hpp"
#include "debug.hpp"

namespace
{
  static void hexdump(debug_stream& stream, const void* ptr, size_t size) noexcept
  {
    stream << showbase << hex;
    auto p = reinterpret_cast<const uint8_t*>(ptr);
    while (size--)
      stream << ' ' << *p++;
  }
}

enocean_serial::~enocean_serial() noexcept
{}

void enocean_serial::push(uint8_t b)
{
  switch (state_)
  {
  case state::wait_sync:
  default:
    if (b == 0x55)
    {
      state_ = state::wait_header;
      receive_size_ = sizeof(event_.hdr);
      receive_ptr_ = reinterpret_cast<uint8_t*>(&event_.hdr);
    }
    break;

  case state::wait_header:
    *receive_ptr_++ = b;
    if (--receive_size_ == 0)
    {
      // process header
      auto cksum = crc8::checksum(&event_.hdr, sizeof(event_.hdr));
      if (cksum != 0)
      {
        cksum = event_.hdr.header_crc8;
        event_.hdr.header_crc8 = 0;
        auto exp_cksum = crc8::checksum(&event_.hdr, sizeof(event_.hdr));
        event_.hdr.header_crc8 = cksum;
        auto& str = debug_stream::instance();
        str << "Header checksum error: expected: " <<
                     hex << showbase <<
                     exp_cksum << ", found: " << cksum <<
                     ", header data:";
        hexdump(str, &event_.hdr, sizeof(event_.hdr));
        str << endl;
        state_ = state::wait_sync;
        break;
      }

      // Checksum OK, receive data
      state_ = state::wait_data;
      receive_ptr_ = event_.buffer;
      receive_size_ = event_.hdr.total_size();
      if (receive_size_ > enocean_event::BUF_SIZE) {
        debug_stream::instance() << "Header with too big size " << dec << receive_size_ <<
                     ", trying to resync" << endl;
        state_ = state::wait_sync;
        break;
      }

    }
    break;

  case state::wait_data:
    *receive_ptr_++ = b;
    if (--receive_size_ == 0)
    {
      // got entire data
      state_ = state::wait_sync;

      auto read_size = event_.hdr.total_size();
      auto cksum = crc8::checksum(&event_.buffer, read_size);
      if (cksum != 0)
      {
        cksum = event_.buffer[read_size - 1];
        event_.buffer[read_size - 1] = 0;
        auto exp_cksum = crc8::checksum(&event_.buffer, read_size);
        event_.buffer[read_size - 1] = cksum;
        auto& str = debug_stream::instance();
        str << "Data checksum error: expected: " <<
                     hex << showbase <<
                     exp_cksum << ", found: " << cksum <<
                     ", data:";
        hexdump(str, &event_.buffer, read_size);
        str << endl;
      }
      else
      {
        // all OK
        handle_event(event_);
      }
    }
    break;
  }
}
