/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "espconn.h"

sint8 espconn_sent(struct espconn *espconn, uint8 *psent, uint16 length) {
  return 0;
}

sint8 espconn_set_opt(struct espconn *espconn, uint8 opt) { return 0; }

sint8 espconn_regist_recvcb(struct espconn *espconn,
                            espconn_recv_callback recv_cb) {
  return 0;
}

sint8 espconn_regist_disconcb(struct espconn *espconn,
                              espconn_connect_callback discon_cb) {
  return 0;
}

sint8 espconn_regist_time(struct espconn *espconn, uint32 interval,
                          uint8 type_flag) {
  return 0;
}

sint8 espconn_regist_connectcb(struct espconn *espconn,
                               espconn_connect_callback connect_cb) {
  return 0;
}

sint8 espconn_accept(struct espconn *espconn) { return 0; }

sint8 espconn_secure_sent(struct espconn *espconn, uint8 *psent,
                          uint16 length) {
  return 0;
}

sint8 espconn_secure_disconnect(struct espconn *espconn) { return 0; }

uint32 espconn_port(void) { return 0; }

sint8 espconn_secure_connect(struct espconn *espconn) { return 0; }

sint8 espconn_disconnect(struct espconn *espconn) { return 0; }

sint8 espconn_connect(struct espconn *espconn) { return 0; }

err_t espconn_gethostbyname(struct espconn *pespconn, const char *hostname,
                            ip_addr_t *addr, dns_found_callback found) {
  return 0;
}

sint8 espconn_regist_reconcb(struct espconn *espconn,
                             espconn_reconnect_callback recon_cb) {
  return 0;
}
