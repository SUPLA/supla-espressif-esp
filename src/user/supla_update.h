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

#ifdef __FOTA

#ifndef SUPLA_ESP_UPDATE_H_
#define SUPLA_ESP_UPDATE_H_

#include "supla_esp.h"

void ICACHE_FLASH_ATTR supla_esp_update_init(void);
void ICACHE_FLASH_ATTR supla_esp_check_updates(void *srpc);
void ICACHE_FLASH_ATTR
supla_esp_update_url_result(TSD_FirmwareUpdate_UrlResult *url_result);
char ICACHE_FLASH_ATTR supla_esp_update_started(void);

#else
char ICACHE_FLASH_ATTR supla_esp_update_started(void);
#endif /* SUPLA_ESP_UPDATE_H_ */

#endif
