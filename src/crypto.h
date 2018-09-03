/*
 *  Coincer
 *  Copyright (C) 2017-2018  Coincer Developers
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CRYPTO_H
#define CRYPTO_H

#include <sodium.h>
#include <stdint.h>

/** A Pair of public and secret key. */
typedef struct s_keypair {
	/** Public key. */
	unsigned char	public_key[crypto_box_PUBLICKEYBYTES];
	/** Secret key. */
	unsigned char	secret_key[crypto_box_SECRETKEYBYTES];
} keypair_t;

void generate_keypair(keypair_t *keypair);

uint32_t get_random_uint32_t(uint32_t upper_bound);

uint64_t get_random_uint64_t(void);

int hash_message(const char		*string_message,
		 unsigned char		hash[crypto_generichash_BYTES]);

void sign_message(const char		*string_message,
		  const unsigned char	secret_key[crypto_box_SECRETKEYBYTES],
		  unsigned char		signature[crypto_sign_BYTES]);

int verify_signature(const char		*string_message,
		     const unsigned char public_key[crypto_box_PUBLICKEYBYTES],
		     const unsigned char signature[crypto_sign_BYTES]);

#endif /* CRYPTO_H */
