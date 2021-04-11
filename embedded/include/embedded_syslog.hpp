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

/*!
 * @file
 * @brief Embedded syslog support.
 */

// This file is used for embedded version of the bridge running on ESP8266

#pragma once

#include <cstdint>

class IPAddress;
class __FlashStringHelper;

enum syslog_priority {
  LOG_EMERG    = 0,  /* system is unusable */
  LOG_ALERT    = 1,  /* action must be taken immediately */
  LOG_CRIT     = 2,  /* critical conditions */
  LOG_ERR      = 3,  /* error conditions */
  LOG_WARNING  = 4,  /* warning conditions */
  LOG_NOTICE   = 5,  /* normal but significant condition */
  LOG_INFO     = 6,  /* informational */
  LOG_DEBUG    = 7,  /* debug-level messages */
};

enum syslog_facility {
  LOG_KERN      = (0<<3),   /* kernel messages */
  LOG_USER      = (1<<3),   /* random user-level messages */
  LOG_MAIL      = (2<<3),   /* mail system */
  LOG_DAEMON    = (3<<3),   /* system daemons */
  LOG_AUTH      = (4<<3),   /* security/authorization messages */
  LOG_SYSLOG    = (5<<3),   /* messages generated internally by syslogd */
  LOG_LPR       = (6<<3),   /* line printer subsystem */
  LOG_NEWS      = (7<<3),   /* network news subsystem */
  LOG_UUCP      = (8<<3),   /* UUCP subsystem */
  LOG_CRON      = (9<<3),   /* clock daemon */
  LOG_AUTHPRIV  = (10<<3),  /* security/authorization messages (private) */
  LOG_FTP       = (11<<3),  /* ftp daemon */
  LOG_NETINFO   = (12<<3),  /* NetInfo */
  LOG_REMOTEAUTH= (13<<3),  /* remote authentication/authorization */
  LOG_INSTALL   = (14<<3),  /* installer subsystem */
  LOG_RAS       = (15<<3),  /* Remote Access Service (VPN / PPP) */
  LOG_LOCAL0    = (16<<3),  /* reserved for local use */
  LOG_LOCAL1    = (17<<3),  /* reserved for local use */
  LOG_LOCAL2    = (18<<3),  /* reserved for local use */
  LOG_LOCAL3    = (19<<3),  /* reserved for local use */
  LOG_LOCAL4    = (20<<3),  /* reserved for local use */
  LOG_LOCAL5    = (21<<3),  /* reserved for local use */
  LOG_LOCAL6    = (22<<3),  /* reserved for local use */
  LOG_LOCAL7    = (23<<3)   /* reserved for local use */
};

/*!
 * @brief Options for openlog().
 *
 * Currently, only LOG_CONS and LOG_PERROR are respected and cause
 * logging to serial console in addition to network log.
 */
enum syslog_options {
  LOG_PID     = 0x01, /* log the pid with each message */
  LOG_CONS    = 0x02, /* log on the console if errors in sending */
  LOG_ODELAY  = 0x04, /* delay open until first syslog() (default) */
  LOG_NDELAY  = 0x08, /* don't delay open */
  LOG_NOWAIT  = 0x10, /* don't wait for console forks: DEPRECATED */
  LOG_PERROR  = 0x20, /* log to stderr as well */
};

/*!
 * @brief Set syslog server address.
 *
 * @param addr,port address and port of the syslog server.
 */
void syslog_begin(const IPAddress& addr, uint16_t port);

/*!
 * @brief Set syslog identifier and options.
 *
 * @param ident identifier/tag to send with each message.
 * @param logopt bitmask log options, see syslog_options.
 * @param facility default facility to log on, see syslog_facility.
 */
void openlog(const char* ident, int logopt, int facility);

/*!
 * @brief Log message to syslog.
 *
 * @param priority message priority, optionally OR-ed with facility.
 * @param message message to log (printf-format).
 */
void syslog(int priority, const char* message, ...) __attribute__((format(printf, 2, 3)));

/*!
 * @brief Log message to syslog.
 *
 * @param priority message priority, optionally OR-ed with facility.
 * @param message message to log (printf-format).
 */
void syslog_P(int priority, const char* message, ...) __attribute__((format(printf, 2, 3)));

/// Stop logging to syslog.
void closelog();
