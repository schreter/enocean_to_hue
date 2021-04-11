/*
 * Copyright (C) 2019 Ivan Schréter (schreter@gmx.net)
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

// This file is used for embedded version of the bridge running on ESP8266

#ifdef ARDUINO

#include "embedded_syslog.hpp"
#include "debug.hpp"

#include <IPAddress.h>

#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <Arduino.h>

extern "C" {
  #include "lwip/udp.h"
}

static bool s_write_to_console = false;
static bool s_open = false;
static int s_facility = 0;
static char s_ident[32];
static char s_msgbuf[400];
static uint32_t s_host;
static uint16_t s_port = 0;
static udp_pcb* s_udp = nullptr;

extern bool s_debug;

void syslog_begin(const IPAddress& addr, uint16_t port)
{
  s_host = addr;
  s_port = port;
  if (s_udp) {
    udp_remove(s_udp);
    s_udp = nullptr;
  }
  if (s_debug)
    debug_stream::instance() << F("SYSLOG: start on port ") << port << ", IP " <<
      int(addr[0]) << '.' << int(addr[1]) << '.' << int(addr[2]) << '.' << int(addr[3]) << '\n';
}

void openlog(const char* ident, int logopt, int facility)
{
  s_write_to_console = (logopt & (LOG_CONS | LOG_PERROR)) != 0;
  s_open = true;
  s_facility = logopt;
  strncpy(s_ident, ident, 32);
  s_ident[31] = 0;
  if (s_debug)
    debug_stream::instance() << F("SYSLOG: open ") << ident << '\n';
}

template<typename MessageOutput>
static void syslog_impl(int priority, MessageOutput&& out)
{
  if (!s_open || !s_port) {
    debug_stream::instance() << F("SYSLOG: try to write to closed log\n");
    return;
  }
  if (!s_udp) {
    s_udp = udp_new_ip_type(IPADDR_TYPE_V4);
    if (!s_udp) {
      debug_stream::instance() << F("SYSLOG: cannot allocate UDP socket\n");
      return;
    }

    auto err = udp_bind(s_udp, IP_ADDR_ANY, s_port);
    if (err != ERR_OK) {
      debug_stream::instance() << F("SYSLOG: cannot bind local side of UDP socket, err=") << int32_t(err) << '\n';;
      udp_remove(s_udp);
      s_udp = nullptr;
      return;
    }
    err = udp_connect(s_udp, reinterpret_cast<const ip_addr_t*>(&s_host), s_port);
    if (err != ERR_OK) {
      debug_stream::instance() << F("SYSLOG: cannot bind remote side of UDP socket, err=") << int32_t(err) << '\n';;
      udp_remove(s_udp);
      s_udp = nullptr;
      return;
    }
  }

  if ((priority >> 3) == 0)
    priority |= s_facility;

  auto len = sprintf(s_msgbuf, "<%d>1 - - %s 1 - - ", priority, s_ident);

  // append actual message
  len += out(s_msgbuf + len, sizeof(s_msgbuf) - len);
  if (len >= sizeof(s_msgbuf))
    len = sizeof(s_msgbuf) - 1;

  // send UDP message to syslog server
  if (s_write_to_console)
    debug_stream::instance() << F("SYSLOG: ") << s_msgbuf << '\n';
  auto p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
  if (!p) {
    debug_stream::instance() << F("SYSLOG: cannot allocate UDP buffer\n");
    return; // cannot allocate pbuf
  }
  pbuf_take(p, s_msgbuf, len);
  auto err = udp_send(s_udp, p);
  if (err != ERR_OK) {
    debug_stream::instance() << F("SYSLOG: cannot send UDP packet, err=") << int32_t(err) << '\n';;
    udp_remove(s_udp);
    s_udp = nullptr;
  }
  pbuf_free(p);
}

void syslog(int priority, const char* message, ...)
{
  va_list arglist;
  va_start(arglist, message);
  syslog_impl(priority, [message, &arglist](char* dest, size_t destlen) -> size_t{
    return vsnprintf(dest, destlen, message, arglist);
  });
  va_end(arglist);
}

void syslog_P(int priority, const char* message, ...)
{
  va_list arglist;
  va_start(arglist, message);
  syslog_impl(priority, [message, &arglist](char* dest, size_t destlen) -> size_t {
    return vsnprintf_P(dest, destlen, message, arglist);
  });
  va_end(arglist);
}

void closelog()
{
  s_open = false;
  if (s_udp) {
    udp_remove(s_udp);
    s_udp = nullptr;
  }
  if (s_debug)
    debug_stream::instance() << F("SYSLOG: log closed\n");
}

#endif
