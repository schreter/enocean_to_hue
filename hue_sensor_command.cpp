#include "hue_sensor_command.hpp"

#include <system_error>
#include <cstring>

#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

void hue_sensor_command::post(int32_t value)
{
  if (queue_size_ == MAX_QUEUE_SIZE) {
    // drop the oldest
    memmove(&queue_[0], &queue_[1], sizeof(queue_[0]) * (MAX_QUEUE_SIZE - 1));
    --queue_size_;
  }
  auto& q = queue_[queue_size_++];
  q.value = value;
  q.timestamp = timestamp();
  if (state_ == state::idle) {
    if (prepare_buffer(q.timestamp))
      start_connect();
  }
}

short hue_sensor_command::get_events() const noexcept
{
  if (state_ != state::receiving)
    return POLLOUT;
  else
    return POLLIN;
}

void hue_sensor_command::poll()
{
  for (;;) {
    switch (state_) {
    case state::connecting:
    case state::sending:
    {
      // socket writable, write remaining stuff
      auto res = ::write(fd_, send_ptr_, send_outstanding_size_);
      if (res < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
          // cannot write any more data
          state_ = state::sending;
          return; // will retry later
        } else if (errno == EINTR) {
          continue;
        }
        throw std::system_error(
            std::error_code(errno, std::generic_category()),
            "Error writing data to socket");
      } else if (res == send_outstanding_size_) {
        // all written
        state_ = state::receiving;
        continue;
      } else if (res < send_outstanding_size_) {
        send_outstanding_size_ -= res;
        send_ptr_ += res;
        continue;
      } else {
        // impossible
        throw std::runtime_error("Wrote too much data to the socket");
      }
    }
    case state::receiving:
    {
      auto res = ::read(fd_, buffer_, sizeof(buffer_));
      if (res < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
          // will retry later
          return;
        } else if (errno == EINTR) {
          continue;
        }
        throw std::system_error(
            std::error_code(errno, std::generic_category()),
            "Error reading data from socket");
      } else if (res == 0) {
        // EOF
        close(fd_);
        state_ = state::idle;
        fd_ = -1;
        if (prepare_buffer(timestamp()))
          start_connect();  // next request pending
        return;
      } else {
        // more data pending
        continue;
      }
    }
    default:
      // ignore
      return;
    }
  }
}

int64_t hue_sensor_command::timestamp() noexcept
{
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return tp.tv_nsec / 1000000 + tp.tv_sec * 1000;
}

bool hue_sensor_command::prepare_buffer(int64_t timestamp) noexcept
{
  // pick first valid element and prepare buffer with it
  // return true, if valid, false on empty queue
  for (uint8_t i = 0; i < queue_size_; ++i) {
    auto& q = queue_[i];
    if (timestamp - q.timestamp <= MAX_EVENT_AGE) {
      // use this one
      char tmp[64];
      auto cl = snprintf(tmp, sizeof(tmp),
            "{\"state\":{\"status\": %d}}", q.value);
      send_outstanding_size_ = uint16_t(snprintf(
            buffer_, sizeof(buffer_),
            "PUT /api/%s/sensors/%d HTTP/1.1\r\n"
            "Host: %d.%d.%d.%d\r\n"
            "Accept: */*\r\n"
            "User-Agent: enocean-gw/0.1\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n%s",
            api_key_, sensor_id_,
            ip_ & 0xff, (ip_ >> 8) & 0xff, (ip_ >> 16) & 0xff, ip_ >> 24,
            cl, tmp));
      send_ptr_ = reinterpret_cast<const uint8_t*>(buffer_);
      if (++i < queue_size_)
        memmove(&queue_[0], &queue_[i], sizeof(queue_[0]) * (queue_size_ - i));
      queue_size_ -= i;
      return true;
    }
  }

  // didn't find any usable
  queue_size_ = 0;
  return false;
}

void hue_sensor_command::start_connect()
{
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_ < 0)
    throw std::system_error(
        std::error_code(errno, std::generic_category()),
        "Cannot create socket for sending Hue request");

  int one = 1;
  if (setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) < 0)
    throw std::system_error(
        std::error_code(errno, std::generic_category()),
        "Cannot set socket's TCP_NODELAY option");

  auto flags = fcntl(fd_, F_GETFL);
  if (flags < 0)
    throw std::system_error(
        std::error_code(errno, std::generic_category()),
        "Cannot get socket's flags");

  if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0)
    throw std::system_error(
        std::error_code(errno, std::generic_category()),
        "Cannot set socket's flags");

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);
  addr.sin_addr.s_addr = ip_;
  if (connect(fd_, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) < 0) {
    if (errno != EINPROGRESS)
      throw std::system_error(
          std::error_code(errno, std::generic_category()),
          "Error connecting the socket");
    state_ = state::connecting;
  } else {
    state_ = state::sending;
    // immediately connected, next poll will send
  }
}
