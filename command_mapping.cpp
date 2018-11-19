#include "command_mapping.hpp"

#include <fstream>
#include <sstream>
#include <system_error>

#include <arpa/inet.h>

command_mapping::command_mapping()
{
  // NOP for now
}

int32_t command_mapping::map(const enocean_event& e)
{
  if (e.hdr.packet_type == enocean_packet_type::RADIO_ERP1 &&
      e.erp1.event_type == enocean_erp1_type::SWITCH) {
    // try the mapping
    auto button = e.erp1.switch_event.button_id();
    auto id = e.erp1.switch_event.sender;
    auto i = mapping_.find(std::make_pair(id, button));
    if (i != mapping_.end())
      return i->second;
  }
  return 0;
}

void command_mapping::add_mapping(enocean_id id, int8_t button, int32_t value)
{
  if (!value)
    throw std::runtime_error("Value must be specified");
  if (button < -1 || button > 8)
    throw std::runtime_error("Button ID must be in range [-1,8]");

  if (button == -1) {
    for (button = 0; button <= 8; ++button)
      add_mapping(id, button, value + button);
    return;
  }
  mapping_.emplace(std::make_pair(id, button), value);
  printf("Added mapping for %x: %d -> %d\n", ntohl(id.raw()), button, value);
}

void command_mapping::load(const char* filename)
{
  std::ifstream infile(filename, std::ios_base::in);
  if (!infile.is_open())
    throw std::runtime_error("Cannot open mapping file");
  std::string line;
  while (std::getline(infile, line)) {
    if (line.length() == 0 || line[0] == '#')
      continue;
    unsigned a, b, c, d;
    int button, value;
    auto res = sscanf(line.c_str(), "%x:%x:%x:%x %d %d", &a, &b, &c, &d, &button, &value);
    if (res != 6)
      throw std::runtime_error("Expected line in form XX:XX:XX:XX # #####");
    if (a > 255 || b > 255 || c > 255 || d > 255)
      throw std::runtime_error("ID must contain only hexadecimal values up to 0xff");
    if (button < -1 || button > 8)
      throw std::runtime_error("Button ID must be in range [-1,8]");
    enocean_id id;
    id.set(uint8_t(a), uint8_t(b), uint8_t(c), uint8_t(d));
    add_mapping(id, int8_t(button), value);
  }
}
