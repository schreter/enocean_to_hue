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

static void usage(const char* name)
{
  std::cerr << "Usage: " << name <<
      " <usb300 port> <mapping file> <bridge IP> <bridge port> <API key> <sensor ID> [<bridge IP> <bridge port> <API key> <sensor ID>]...\n";
}

int main(int argc, const char** argv)
{
  auto progname = argv[0];
  if (argc < 6) {
    usage(progname);
    return 1;
  }
  const char* serial_port = argv[1];
  const char* config_file = argv[2];
  argv += 3;
  argc -= 3;
  std::deque<hue_sensor_command_posix> bridges;
  while (argc >= 4) {
    if (bridges.size() == 8) {
      std::cerr << "At most 8 bridges are supported\n";
      usage(progname);
      return 1;
    }
    struct in_addr bridge_addr;
    if (!inet_aton(argv[0], &bridge_addr)) {
      std::cerr << "Cannot parse bridge IP address '" << argv[0] << "'\n";
      usage(progname);
      return 1;
    }
    char *end;
    int bridge_port = strtol(argv[1], &end, 10);
    if (end == argv[1] || *end || bridge_port < 1 || bridge_port > 65535) {
      std::cerr << "Specified port number '" << argv[1] <<
          "' is invalid. Expected port in range [1,65535].\n";
      usage(progname);
      return 1;

    }
    int sensor_id;
    auto id = strtol(argv[3], &end, 10);
    if (end == argv[3] || *end || id < 1 || id > 255) {
      std::cerr << "Specified sensor ID '" << argv[3] <<
          "' is invalid. Expected ID in range [1,255].\n";
      usage(progname);
      return 1;
    }
    sensor_id = int(id);
    bridges.emplace_back(bridge_addr.s_addr, bridge_port, argv[2], sensor_id);
    argv += 4;
    argc -= 4;
  }
  if (argc != 0)
  {
    std::cerr << "Extraneous argument(s) on the command line\n";
    usage(progname);
    return 1;
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
      openlog("enocean_to_hue", LOG_CONS | LOG_PID | LOG_PERROR, LOG_LOCAL1);
      try {
        enocean_to_hue_bridge bridge(serial_port, bridges, config_file);
        bridge.run_poll_loop();
      } catch (std::exception& e) {
        syslog(LOG_ERR, "EnOcean ERROR: %s", e.what());
      }
      return 1;
    }

    // parent process, wait for child
    syslog(LOG_INFO, "EnOcean Started child process %d", pid);
    int status;
    auto cpid = wait(&status);
    auto end_time = timestamp();
    auto delta = end_time - start_time;
    syslog(LOG_ERR, "EnOcean Child process %d exited with status %d after %lld ms", cpid, status, delta);
    if (pid != cpid) {
      syslog(LOG_ERR, "EnOcean Wrong PID %d of terminated process, expected %d", cpid, pid);
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
