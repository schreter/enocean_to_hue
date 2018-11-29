#include "enocean_to_hue_bridge.hpp"

#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <syslog.h>

static int64_t timestamp() noexcept
{
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return tp.tv_nsec / 1000000 + tp.tv_sec * 1000;
}

int main(int argc, const char** argv)
{
  if (argc != 6) {
    std::cerr << "Usage: " << argv[0] <<
        " <usb300 port> <bridge IP> <API key> <sensor ID> <mapping file>\n";
    return 1;
  }
  int sensor_id;
  {
    char* end;
    auto id = strtol(argv[4], &end, 10);
    if (end == argv[4] || *end || id < 1 || id > 63) {
      std::cerr << "Specified sensor ID '" << argv[4] <<
          "' is invalid. Expected ID in range [1,63].\n";
      return 1;
    }
    sensor_id = int(id);
  }
  struct in_addr bridge_addr;
  {
    if (!inet_aton(argv[2], &bridge_addr)) {
      std::cerr << "Cannot parse bridge IP address '" << argv[2] << "'\n";
      return 1;
    }
  }

  setlogmask(LOG_UPTO(LOG_INFO));
  openlog("enocean_to_hue", LOG_CONS | LOG_PID | LOG_PERROR, LOG_LOCAL1);

  uint32_t respawn_cnt = 0;
  for (;;)
  {
    auto start_time = timestamp();
    auto pid = fork();
    if (!pid) {
      // child process, run the bridge
      openlog("enocean_to_hue", LOG_CONS | LOG_PID, LOG_LOCAL1);
      try {
        enocean_to_hue_bridge bridge(argv[1], bridge_addr.s_addr, argv[3], sensor_id, argv[5]);
        bridge.run_poll_loop();
      } catch (std::exception& e) {
        syslog(LOG_ERR, "ERROR: %s", e.what());
      }
      return 1;
    }

    // parent process, wait for child
    syslog(LOG_INFO, "Started child process %d", pid);
    int status;
    auto cpid = wait(&status);
    auto end_time = timestamp();
    auto delta = end_time - start_time;
    syslog(LOG_ERR, "Child process %d exited with status %d after %lld ms", cpid, status, delta);
    if (pid != cpid) {
      syslog(LOG_ERR, "Wrong PID %d of terminated process, expected %d", cpid, pid);
      return 1;
    }

    if (delta >= 60000) {
      respawn_cnt = 0;
      continue;   // restart immediately, it ran long enough
    } else if (delta < 5000) {
      // short runtime, assume wrong configuration
      ++respawn_cnt;
    }

    // sleep before respawn
    if (respawn_cnt < 10)
      sleep(1);
    else if (respawn_cnt < 30)
      sleep(30);
    else
      sleep(60);
  }
}