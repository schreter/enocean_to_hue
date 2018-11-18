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
