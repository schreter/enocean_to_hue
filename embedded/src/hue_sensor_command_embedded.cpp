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

#ifdef ARDUINO

#include "hue_sensor_command_embedded.hpp"
#include "debug.hpp"
#include "embedded_syslog.hpp"

#include <Arduino.h>

extern "C" {
  #include "lwip/tcp.h"
}

extern bool s_debug;

void hue_sensor_command_embedded::poll()
{
  auto s = get_state();
  if (s != state::idle && s < state::unknown && pcb_ && report_time_) {
    // check for long-running requests
    unsigned long time = millis();
    auto delta = time - connect_time_;
    if (delta >= report_time_) {
      syslog_P(LOG_WARNING, PSTR("EnOcean Request still not done after %lums, state %d, to_send %u/%u"),
        delta, int(s), unsigned(send_outstanding_size_), unsigned(send_total_size_));
      report_time_ *= 2;
      if (report_time_ > 8000) {
        // way too long request, abort it
        syslog_P(LOG_WARNING, PSTR("EnOcean Aborting request, restarting"));
        auto err = tcp_close(pcb_);
        if (err != ERR_OK) {
          if (s_debug)
            debug_stream::instance() << F("EnOcean Error closing socket, aborting socket; err=") << int(err);
          tcp_abort(pcb_);
        }
        pcb_ = nullptr;
        report_time_ = 0;
        request_failed();
        ESP.restart();  // to be on the safe side
        return;
      }
    }
  }

  switch (s) {
    case state::connecting:
      // connect is in progress
      break;

    case state::sending:
    {
      // socket writable, write remaining stuff
      auto to_send = tcp_sndbuf(pcb_);
      if (send_outstanding_size_ < to_send)
        to_send = send_outstanding_size_;
      if (to_send == 0)
        break; // no send space
      if (s_debug && send_outstanding_size_ == send_total_size_) {
        auto& stream = debug_stream::instance();
        stream << F("Sending request to Hue bridge:\n");
        stream.write(send_ptr_, send_outstanding_size_);
      }
      auto err = tcp_write(pcb_, send_ptr_, to_send, 0);
      if (err == ERR_MEM) {
        if (s_debug)
          debug_stream::instance() << F("\nOUT OF MEMORY\n");
        break; // will retry again
      } else if (err != ERR_OK) {
        // some other error
        if (s_debug)
          debug_stream::instance() << F("\nERROR");
        connection_error(this, err);
        break;
      } else if (s_debug) {
        debug_stream::instance() << '\n';
      }
      send_outstanding_size_ -= to_send;
      send_ptr_ += to_send;
      if (send_outstanding_size_ == 0) {
        // all written
        tcp_output(pcb_);
        request_sent();
      }
      break;
    }

    case state::receiving:
      // no special handling, receive callback will care for data
      break;

    default:
      // no-op
      break;
  }
}

hue_sensor_command::timestamp_t hue_sensor_command_embedded::timestamp() noexcept
{
  return millis();
}

bool hue_sensor_command_embedded::start_connect()
{
  // ensure we have a valid socket
  if (pcb_) {
    syslog_P(LOG_ERR, PSTR("EnOcean Aborting left-over socket %p at connection start"), pcb_);
    tcp_abort(pcb_);
  }
  pcb_ = tcp_new();
  if (!pcb_) {
    syslog_P(LOG_ERR, PSTR("EnOcean OUT OF MEMORY on start_connect()"));
    restart_connect_ = true;
    report_time_ = 0;
    return false;
  }
  tcp_arg(pcb_, this);
  tcp_err(pcb_, connection_error);
  tcp_recv(pcb_, data_received);

  connect_time_ = millis();
  report_time_ = 500; // give some time for proper request handling initially
  auto err = tcp_connect(pcb_, reinterpret_cast<const ip_addr_t*>(&ip_), 80, connection_established);
  if (err != ERR_OK) {
    syslog_P(LOG_ERR, PSTR("EnOcean OUT OF MEMORY on connect()"));
    tcp_abort(pcb_);
    pcb_ = nullptr;
    restart_connect_ = true;
    return false;
  }

  restart_connect_ = false;
  return false;
}

void hue_sensor_command_embedded::connection_error(void* arg, err_t err)
{
  syslog_P(LOG_ERR, PSTR("EnOcean TCP error %d"), int(err));
  auto self = reinterpret_cast<hue_sensor_command_embedded*>(arg);
  self->pcb_ = nullptr;
  self->report_time_ = 0;
  self->request_failed();
}

err_t hue_sensor_command_embedded::connection_established(void* arg, tcp_pcb* tpcb, err_t err)
{
  auto self = reinterpret_cast<hue_sensor_command_embedded*>(arg);
  if (err != ERR_OK) {
    syslog_P(LOG_ERR, PSTR("EnOcean TCP connect error %d"), int(err));
    self->pcb_ = nullptr;
    if (tpcb)
      tcp_abort(tpcb);
    self->report_time_ = 0;
    self->request_failed();
    return err;
  }
  if (s_debug)
    debug_stream::instance() << F("Connection established\n");
  self->connected();
  return ERR_OK;
}

err_t hue_sensor_command_embedded::data_received(void* arg, tcp_pcb* tpcb, pbuf* p, err_t err)
{
  auto self = reinterpret_cast<hue_sensor_command_embedded*>(arg);
  if (p == nullptr) {
    // last segment received, EOF, close connection
    if (s_debug)
      debug_stream::instance() << F("Request completed\n");
    tcp_sent(tpcb, nullptr);
    tcp_recv(tpcb, nullptr);
    tcp_err(tpcb, nullptr);
    err = tcp_close(tpcb);
    if (err != ERR_OK) {
      if (s_debug)
        debug_stream::instance() << F("Error closing socket, aborting socket; err=") << int(err);
      tcp_abort(tpcb);
    }
    self->pcb_ = nullptr;
    self->report_time_ = 0;
    self->request_finished();
  } else {
    // else just confirm data, ignore contents
    if (s_debug) {
      auto& stream = debug_stream::instance();
      stream << F("Received data:\n");
      stream.write(reinterpret_cast<const uint8_t*>(p->payload), p->len);
      stream << '\n';
    }
    tcp_recved(tpcb, p->len);
  }
}

#endif
