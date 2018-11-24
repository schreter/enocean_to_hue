#include "enocean_to_hue_bridge.hpp"

#include <system_error>
#include <poll.h>

enocean_to_hue_bridge::enocean_to_hue_bridge(
    const char* port,
    uint32_t bridge_ip,
    const char* api_key,
    int sensor_id,
    const char* map_file) :
  cmd_(bridge_ip, api_key, sensor_id),
  hnd_(port, *this)
{
  map_.load(map_file);
}

void enocean_to_hue_bridge::run_poll_loop()
{
  for (;;)
  {
    struct pollfd fds[2];
    nfds_t cnt = 1;
    fds[0].fd = hnd_.get_fd();
    fds[0].events = POLLERR | POLLIN;
    fds[0].revents = 0;
    fds[1].fd = cmd_.get_fd();
    if (fds[1].fd >= 0) {
      fds[1].events = cmd_.get_events() | POLLERR;
      fds[1].revents = 0;
      cnt = 2;
    }
    auto res = poll(fds, cnt, -1);
    if (res < 0) {
      if (errno == EINTR || errno == EAGAIN)
        continue; // interrupted by signal or out of resources, retry
      throw std::system_error(
          std::error_code(errno, std::generic_category()),
          "Error polling file descriptors");
    }
    if (fds[0].revents)
      hnd_.poll();
    if (fds[1].revents)
      cmd_.poll();
  }
}

void enocean_to_hue_bridge::handler::handle_event(const enocean_event& event)
{
  parent_.handle_event(event);
}

static void hexdump(const void* ptr, size_t size) noexcept
{
  auto p = reinterpret_cast<const uint8_t*>(ptr);
  while (size--)
    printf(" %02x", *p++);
}

void enocean_to_hue_bridge::handle_event(const enocean_event& event)
{
  static int count = 0;
  printf("Got event %d, type=%d:", ++count, int(event.hdr.packet_type));
  hexdump(&event.buffer, event.hdr.total_size());
  printf("\n");
  auto value = map_.map(event);
  if (value)
      cmd_.post(value);
}
