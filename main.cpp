#include "enocean_serial.hpp"
#include "hue_sensor_command.hpp"

#include <system_error>

#include <sys/types.h>
#include <unistd.h>
#include <cstdint>

#include <poll.h>
#include <stdio.h>
#include <errno.h>

static constexpr const char* PORT = "/dev/ttyUSB1";//"/dev/tty.usbserial-FT2LRL2C";

static void hexdump(const void* ptr, size_t size) noexcept
{
  auto p = reinterpret_cast<const uint8_t*>(ptr);
  while (size--)
    printf(" %02x", *p++);
}

class handler : public enocean_serial
{
public:
  explicit handler(hue_sensor_command& cmd) : enocean_serial(PORT), cmd_(cmd) {}

private:
  virtual void handle_event(const enocean_event& event)
  {
    printf("Got event, type=%d:", event.hdr.packet_type);
    hexdump(&event.buffer, event.hdr.total_size());
    printf("\n");
    if (event.hdr.packet_type == enocean_packet_type::RADIO_ERP1 &&
        event.erp1.event_type == enocean_erp1_type::SWITCH)
    {
      auto& e = event.erp1.switch_event;
      printf("Button %d of %02x:%02x:%02x:%02x\n", e.button_id(),
             e.sender.addr[0], e.sender.addr[1], e.sender.addr[2], e.sender.addr[3]);
      if (e.is_pressed())
        cmd_.post(10);
    }
  }

  hue_sensor_command& cmd_;
};

int main()
{
  hue_sensor_command cmd(
        192 + 256 * 168 + 65536 * 178 + 256 * 65536 * uint32_t(44),
        "<key>",
        36);
  handler h(cmd);
  printf("Initialized serial communication at fd=%d\n", h.get_fd());
  for (;;)
  {
    struct pollfd fds[2];
    nfds_t cnt = 1;
    fds[0].fd = h.get_fd();
    fds[0].events = POLLERR | POLLIN;
    fds[0].revents = 0;
    fds[1].fd = cmd.get_fd();
    if (fds[1].fd >= 0) {
      fds[1].events = cmd.get_events() | POLLERR;
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
      h.poll();
    if (fds[1].revents)
      cmd.poll();
  }

}
