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

#include "nettle/esp8266.h"

#include "nettle/bignum.h"
#include "nettle/rsa.h"
#include "nettle/sha2.h"

void nettle_sha256_init(struct sha256_ctx *ctx) { return; }
void nettle_sha256_update(struct sha256_ctx *ctx, size_t length,
                          const uint8_t *data) {
  return;
}

void rsa_public_key_init(struct rsa_public_key *key) { return; }
int nettle_rsa_public_key_prepare(struct rsa_public_key *key) { return 0; }

void nettle_mpz_set_str_256_u(mpz_t x, size_t length, const uint8_t *s) {
  return;
}
void nettle_mpz_init_set_str_256_u(mpz_t x, size_t length, const uint8_t *s) {
  return;
}
void mpz_set_ui(mpz_t x, unsigned long int y) { return; }
void mpz_init(mpz_t x) { return; }

int rsa_sha256_verify(const struct rsa_public_key *key, struct sha256_ctx *hash,
                      const mpz_t signature) {
  return 0;
}

void mpz_clear(mpz_t x) { return; }
void rsa_public_key_clear(struct rsa_public_key *key) { return; }
