/* mini-gmp, a minimalistic implementation of a GNU GMP subset.

Copyright 2011-2014 Free Software Foundation, Inc.

This file is part of the GNU MP Library.

The GNU MP Library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The GNU MP Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the GNU MP Library.  If not,
see https://www.gnu.org/licenses/.  */

/* About mini-gmp: This is a minimal implementation of a subset of the
   GMP interface. It is intended for inclusion into applications which
   have modest bignums needs, as a fallback when the real GMP library
   is not installed.

   This file defines the public interface. */

#ifndef __MINI_GMP_H__
#define __MINI_GMP_H__

/* For size_t */
#include <stddef.h>

#if defined (__cplusplus)
extern "C" {
#endif

void ICACHE_FLASH_ATTR mp_set_memory_functions (void *(*) (size_t),
			      void *(*) (void *, size_t, size_t),
			      void (*) (void *, size_t));

void ICACHE_FLASH_ATTR mp_get_memory_functions (void *(**) (size_t),
			      void *(**) (void *, size_t, size_t),
			      void (**) (void *, size_t));

typedef unsigned long mp_limb_t;
typedef long mp_size_t;
typedef unsigned long mp_bitcnt_t;

typedef mp_limb_t *mp_ptr;
typedef const mp_limb_t *mp_srcptr;

typedef struct
{
  int _mp_alloc;		/* Number of *limbs* allocated and pointed
				   to by the _mp_d field.  */
  int _mp_size;			/* abs(_mp_size) is the number of limbs the
				   last field points to.  If _mp_size is
				   negative this is a negative number.  */
  mp_limb_t *_mp_d;		/* Pointer to the limbs.  */
} __mpz_struct;

typedef __mpz_struct mpz_t[1];

typedef __mpz_struct *mpz_ptr;
typedef const __mpz_struct *mpz_srcptr;

extern const int mp_bits_per_limb;

void ICACHE_FLASH_ATTR mpn_copyi (mp_ptr, mp_srcptr, mp_size_t);
void ICACHE_FLASH_ATTR mpn_copyd (mp_ptr, mp_srcptr, mp_size_t);
void ICACHE_FLASH_ATTR mpn_zero (mp_ptr, mp_size_t);

int ICACHE_FLASH_ATTR mpn_cmp (mp_srcptr, mp_srcptr, mp_size_t);

mp_limb_t ICACHE_FLASH_ATTR mpn_add_1 (mp_ptr, mp_srcptr, mp_size_t, mp_limb_t);
mp_limb_t ICACHE_FLASH_ATTR mpn_add_n (mp_ptr, mp_srcptr, mp_srcptr, mp_size_t);
mp_limb_t ICACHE_FLASH_ATTR mpn_add (mp_ptr, mp_srcptr, mp_size_t, mp_srcptr, mp_size_t);

mp_limb_t ICACHE_FLASH_ATTR mpn_sub_1 (mp_ptr, mp_srcptr, mp_size_t, mp_limb_t);
mp_limb_t ICACHE_FLASH_ATTR mpn_sub_n (mp_ptr, mp_srcptr, mp_srcptr, mp_size_t);
mp_limb_t ICACHE_FLASH_ATTR mpn_sub (mp_ptr, mp_srcptr, mp_size_t, mp_srcptr, mp_size_t);

mp_limb_t ICACHE_FLASH_ATTR mpn_mul_1 (mp_ptr, mp_srcptr, mp_size_t, mp_limb_t);
mp_limb_t ICACHE_FLASH_ATTR mpn_addmul_1 (mp_ptr, mp_srcptr, mp_size_t, mp_limb_t);
mp_limb_t ICACHE_FLASH_ATTR mpn_submul_1 (mp_ptr, mp_srcptr, mp_size_t, mp_limb_t);

mp_limb_t ICACHE_FLASH_ATTR mpn_mul (mp_ptr, mp_srcptr, mp_size_t, mp_srcptr, mp_size_t);
void ICACHE_FLASH_ATTR mpn_mul_n (mp_ptr, mp_srcptr, mp_srcptr, mp_size_t);
void ICACHE_FLASH_ATTR mpn_sqr (mp_ptr, mp_srcptr, mp_size_t);
int ICACHE_FLASH_ATTR mpn_perfect_square_p (mp_srcptr, mp_size_t);
mp_size_t ICACHE_FLASH_ATTR mpn_sqrtrem (mp_ptr, mp_ptr, mp_srcptr, mp_size_t);

mp_limb_t ICACHE_FLASH_ATTR mpn_lshift (mp_ptr, mp_srcptr, mp_size_t, unsigned int);
mp_limb_t ICACHE_FLASH_ATTR mpn_rshift (mp_ptr, mp_srcptr, mp_size_t, unsigned int);

mp_bitcnt_t ICACHE_FLASH_ATTR mpn_scan0 (mp_srcptr, mp_bitcnt_t);
mp_bitcnt_t ICACHE_FLASH_ATTR mpn_scan1 (mp_srcptr, mp_bitcnt_t);

mp_bitcnt_t ICACHE_FLASH_ATTR mpn_popcount (mp_srcptr, mp_size_t);

mp_limb_t ICACHE_FLASH_ATTR mpn_invert_3by2 (mp_limb_t, mp_limb_t);
#define mpn_invert_limb(x) mpn_invert_3by2 ((x), 0)

size_t ICACHE_FLASH_ATTR mpn_get_str (unsigned char *, int, mp_ptr, mp_size_t);
mp_size_t ICACHE_FLASH_ATTR mpn_set_str (mp_ptr, const unsigned char *, size_t, int);

void ICACHE_FLASH_ATTR mpz_init (mpz_t);
void ICACHE_FLASH_ATTR mpz_init2 (mpz_t, mp_bitcnt_t);
void ICACHE_FLASH_ATTR mpz_clear (mpz_t);

#define mpz_odd_p(z)   (((z)->_mp_size != 0) & (int) (z)->_mp_d[0])
#define mpz_even_p(z)  (! mpz_odd_p (z))

int ICACHE_FLASH_ATTR mpz_sgn (const mpz_t);
int ICACHE_FLASH_ATTR mpz_cmp_si (const mpz_t, long);
int ICACHE_FLASH_ATTR mpz_cmp_ui (const mpz_t, unsigned long);
int ICACHE_FLASH_ATTR mpz_cmp (const mpz_t, const mpz_t);
int ICACHE_FLASH_ATTR mpz_cmpabs_ui (const mpz_t, unsigned long);
int ICACHE_FLASH_ATTR mpz_cmpabs (const mpz_t, const mpz_t);
int ICACHE_FLASH_ATTR mpz_cmp_d (const mpz_t, double);
int ICACHE_FLASH_ATTR mpz_cmpabs_d (const mpz_t, double);

void ICACHE_FLASH_ATTR mpz_abs (mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_neg (mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_swap (mpz_t, mpz_t);

void ICACHE_FLASH_ATTR mpz_add_ui (mpz_t, const mpz_t, unsigned long);
void ICACHE_FLASH_ATTR mpz_add (mpz_t, const mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_sub_ui (mpz_t, const mpz_t, unsigned long);
void ICACHE_FLASH_ATTR mpz_ui_sub (mpz_t, unsigned long, const mpz_t);
void ICACHE_FLASH_ATTR mpz_sub (mpz_t, const mpz_t, const mpz_t);

void ICACHE_FLASH_ATTR mpz_mul_si (mpz_t, const mpz_t, long int);
void ICACHE_FLASH_ATTR mpz_mul_ui (mpz_t, const mpz_t, unsigned long int);
void ICACHE_FLASH_ATTR mpz_mul (mpz_t, const mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_mul_2exp (mpz_t, const mpz_t, mp_bitcnt_t);
void ICACHE_FLASH_ATTR mpz_addmul_ui (mpz_t, const mpz_t, unsigned long int);
void ICACHE_FLASH_ATTR mpz_addmul (mpz_t, const mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_submul_ui (mpz_t, const mpz_t, unsigned long int);
void ICACHE_FLASH_ATTR mpz_submul (mpz_t, const mpz_t, const mpz_t);

void ICACHE_FLASH_ATTR mpz_cdiv_qr (mpz_t, mpz_t, const mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_fdiv_qr (mpz_t, mpz_t, const mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_tdiv_qr (mpz_t, mpz_t, const mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_cdiv_q (mpz_t, const mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_fdiv_q (mpz_t, const mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_tdiv_q (mpz_t, const mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_cdiv_r (mpz_t, const mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_fdiv_r (mpz_t, const mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_tdiv_r (mpz_t, const mpz_t, const mpz_t);

void ICACHE_FLASH_ATTR mpz_cdiv_q_2exp (mpz_t, const mpz_t, mp_bitcnt_t);
void ICACHE_FLASH_ATTR mpz_fdiv_q_2exp (mpz_t, const mpz_t, mp_bitcnt_t);
void ICACHE_FLASH_ATTR mpz_tdiv_q_2exp (mpz_t, const mpz_t, mp_bitcnt_t);
void ICACHE_FLASH_ATTR mpz_cdiv_r_2exp (mpz_t, const mpz_t, mp_bitcnt_t);
void ICACHE_FLASH_ATTR mpz_fdiv_r_2exp (mpz_t, const mpz_t, mp_bitcnt_t);
void ICACHE_FLASH_ATTR mpz_tdiv_r_2exp (mpz_t, const mpz_t, mp_bitcnt_t);

void ICACHE_FLASH_ATTR mpz_mod (mpz_t, const mpz_t, const mpz_t);

void ICACHE_FLASH_ATTR mpz_divexact (mpz_t, const mpz_t, const mpz_t);

int ICACHE_FLASH_ATTR mpz_divisible_p (const mpz_t, const mpz_t);
int ICACHE_FLASH_ATTR mpz_congruent_p (const mpz_t, const mpz_t, const mpz_t);

unsigned long ICACHE_FLASH_ATTR mpz_cdiv_qr_ui (mpz_t, mpz_t, const mpz_t, unsigned long);
unsigned long ICACHE_FLASH_ATTR mpz_fdiv_qr_ui (mpz_t, mpz_t, const mpz_t, unsigned long);
unsigned long ICACHE_FLASH_ATTR mpz_tdiv_qr_ui (mpz_t, mpz_t, const mpz_t, unsigned long);
unsigned long ICACHE_FLASH_ATTR mpz_cdiv_q_ui (mpz_t, const mpz_t, unsigned long);
unsigned long ICACHE_FLASH_ATTR mpz_fdiv_q_ui (mpz_t, const mpz_t, unsigned long);
unsigned long ICACHE_FLASH_ATTR mpz_tdiv_q_ui (mpz_t, const mpz_t, unsigned long);
unsigned long ICACHE_FLASH_ATTR mpz_cdiv_r_ui (mpz_t, const mpz_t, unsigned long);
unsigned long ICACHE_FLASH_ATTR mpz_fdiv_r_ui (mpz_t, const mpz_t, unsigned long);
unsigned long ICACHE_FLASH_ATTR mpz_tdiv_r_ui (mpz_t, const mpz_t, unsigned long);
unsigned long ICACHE_FLASH_ATTR mpz_cdiv_ui (const mpz_t, unsigned long);
unsigned long ICACHE_FLASH_ATTR mpz_fdiv_ui (const mpz_t, unsigned long);
unsigned long ICACHE_FLASH_ATTR mpz_tdiv_ui (const mpz_t, unsigned long);

unsigned long ICACHE_FLASH_ATTR mpz_mod_ui (mpz_t, const mpz_t, unsigned long);

void ICACHE_FLASH_ATTR mpz_divexact_ui (mpz_t, const mpz_t, unsigned long);

int ICACHE_FLASH_ATTR mpz_divisible_ui_p (const mpz_t, unsigned long);

unsigned long ICACHE_FLASH_ATTR mpz_gcd_ui (mpz_t, const mpz_t, unsigned long);
void ICACHE_FLASH_ATTR mpz_gcd (mpz_t, const mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_gcdext (mpz_t, mpz_t, mpz_t, const mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_lcm_ui (mpz_t, const mpz_t, unsigned long);
void ICACHE_FLASH_ATTR mpz_lcm (mpz_t, const mpz_t, const mpz_t);
int ICACHE_FLASH_ATTR mpz_invert (mpz_t, const mpz_t, const mpz_t);

void ICACHE_FLASH_ATTR mpz_sqrtrem (mpz_t, mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_sqrt (mpz_t, const mpz_t);
int ICACHE_FLASH_ATTR mpz_perfect_square_p (const mpz_t);

void ICACHE_FLASH_ATTR mpz_pow_ui (mpz_t, const mpz_t, unsigned long);
void ICACHE_FLASH_ATTR mpz_ui_pow_ui (mpz_t, unsigned long, unsigned long);
void ICACHE_FLASH_ATTR mpz_powm (mpz_t, const mpz_t, const mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_powm_ui (mpz_t, const mpz_t, unsigned long, const mpz_t);

void ICACHE_FLASH_ATTR mpz_rootrem (mpz_t, mpz_t, const mpz_t, unsigned long);
int ICACHE_FLASH_ATTR mpz_root (mpz_t, const mpz_t, unsigned long);

void ICACHE_FLASH_ATTR mpz_fac_ui (mpz_t, unsigned long);
void ICACHE_FLASH_ATTR mpz_bin_uiui (mpz_t, unsigned long, unsigned long);

int ICACHE_FLASH_ATTR mpz_probab_prime_p (const mpz_t, int);

int ICACHE_FLASH_ATTR mpz_tstbit (const mpz_t, mp_bitcnt_t);
void ICACHE_FLASH_ATTR mpz_setbit (mpz_t, mp_bitcnt_t);
void ICACHE_FLASH_ATTR mpz_clrbit (mpz_t, mp_bitcnt_t);
void ICACHE_FLASH_ATTR mpz_combit (mpz_t, mp_bitcnt_t);

void ICACHE_FLASH_ATTR mpz_com (mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_and (mpz_t, const mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_ior (mpz_t, const mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_xor (mpz_t, const mpz_t, const mpz_t);

mp_bitcnt_t ICACHE_FLASH_ATTR mpz_popcount (const mpz_t);
mp_bitcnt_t ICACHE_FLASH_ATTR mpz_hamdist (const mpz_t, const mpz_t);
mp_bitcnt_t ICACHE_FLASH_ATTR mpz_scan0 (const mpz_t, mp_bitcnt_t);
mp_bitcnt_t ICACHE_FLASH_ATTR mpz_scan1 (const mpz_t, mp_bitcnt_t);

int ICACHE_FLASH_ATTR mpz_fits_slong_p (const mpz_t);
int ICACHE_FLASH_ATTR mpz_fits_ulong_p (const mpz_t);
long int ICACHE_FLASH_ATTR mpz_get_si (const mpz_t);
unsigned long int ICACHE_FLASH_ATTR mpz_get_ui (const mpz_t);
double ICACHE_FLASH_ATTR mpz_get_d (const mpz_t);
size_t ICACHE_FLASH_ATTR mpz_size (const mpz_t);
mp_limb_t ICACHE_FLASH_ATTR mpz_getlimbn (const mpz_t, mp_size_t);

void ICACHE_FLASH_ATTR mpz_realloc2 (mpz_t, mp_bitcnt_t);
mp_srcptr ICACHE_FLASH_ATTR mpz_limbs_read (mpz_srcptr);
mp_ptr ICACHE_FLASH_ATTR mpz_limbs_modify (mpz_t, mp_size_t);
mp_ptr ICACHE_FLASH_ATTR mpz_limbs_write (mpz_t, mp_size_t);
void ICACHE_FLASH_ATTR mpz_limbs_finish (mpz_t, mp_size_t);
mpz_srcptr ICACHE_FLASH_ATTR mpz_roinit_n (mpz_t, mp_srcptr, mp_size_t);

#define MPZ_ROINIT_N(xp, xs) {{0, (xs),(xp) }}

void ICACHE_FLASH_ATTR mpz_set_si (mpz_t, signed long int);
void ICACHE_FLASH_ATTR mpz_set_ui (mpz_t, unsigned long int);
void ICACHE_FLASH_ATTR mpz_set (mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_set_d (mpz_t, double);

void ICACHE_FLASH_ATTR mpz_init_set_si (mpz_t, signed long int);
void ICACHE_FLASH_ATTR mpz_init_set_ui (mpz_t, unsigned long int);
void ICACHE_FLASH_ATTR mpz_init_set (mpz_t, const mpz_t);
void ICACHE_FLASH_ATTR mpz_init_set_d (mpz_t, double);

size_t ICACHE_FLASH_ATTR mpz_sizeinbase (const mpz_t, int);
char * ICACHE_FLASH_ATTR mpz_get_str (char *, int, const mpz_t);
int ICACHE_FLASH_ATTR mpz_set_str (mpz_t, const char *, int);
int ICACHE_FLASH_ATTR mpz_init_set_str (mpz_t, const char *, int);

void ICACHE_FLASH_ATTR mpz_import (mpz_t, size_t, int, size_t, int, size_t, const void *);
void * ICACHE_FLASH_ATTR mpz_export (void *, size_t *, int, size_t, int, size_t, const mpz_t);

#if defined (__cplusplus)
}
#endif
#endif /* __MINI_GMP_H__ */
