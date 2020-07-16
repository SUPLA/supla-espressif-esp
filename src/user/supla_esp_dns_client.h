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

#ifndef SUPLA_ESP_DNS_CLIENT_H_
#define SUPLA_ESP_DNS_CLIENT_H_

#ifndef ADDITIONAL_DNS_CLIENT_DISABLED

#include <ip_addr.h>
#include "supla_esp.h"

typedef void (*_dns_query_result_cb)(ip_addr_t *ip);
void DNS_ICACHE_FLASH_ATTR supla_esp_dns_client_init(void);
void DNS_ICACHE_FLASH_ATTR supla_esp_dns_resolve(
    const char *domain, _dns_query_result_cb dns_query_result_cb);

#endif /*ADDITIONAL_DNS_CLIENT_DISABLED*/

#endif /*SUPLA_ESP_DNS_CLIENT_H_*/
