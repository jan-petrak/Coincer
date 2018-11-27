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

#define PUBLIC_KEY_SIZE	crypto_box_PUBLICKEYBYTES
#define SECRET_KEY_SIZE	crypto_box_SECRETKEYBYTES
#define SIGNATURE_SIZE	crypto_sign_BYTES

/* TODO: replace with macros from wolfssl */
#define RIPEMD_160_SIZE	20
#define SHA3_256_SIZE	32

/** A Pair of public and secret key. */
typedef struct s_keypair {
	/** Public key. */
	unsigned char	public_key[PUBLIC_KEY_SIZE];
	/** Secret key. */
	unsigned char	secret_key[SECRET_KEY_SIZE];
} keypair_t;

enum hash_type {
	HT_RIPEMD_160,
	HT_SHA3_256
};

int decrypt_message(const char		*message,
		    const unsigned char public_key[PUBLIC_KEY_SIZE],
		    const unsigned char	secret_key[SECRET_KEY_SIZE],
		    char		**decrypted);
int encrypt_message(const char		*message,
		    const unsigned char public_key[PUBLIC_KEY_SIZE],
		    char		**encrypted);

void fetch_random_value(void *value, size_t value_size);

void generate_keypair(keypair_t *keypair);

uint32_t get_random_uint32_t(uint32_t upper_bound);
uint64_t get_random_uint64_t(void);

int hash_check(enum hash_type		type,
	       const unsigned char	*hash,
	       const unsigned char	*value,
	       size_t			value_size);
void hash_message(enum hash_type	type,
		  const unsigned char	*message,
		  size_t		message_size,
		  unsigned char		*hash);

void sign_message(const char		*string_message,
		  const unsigned char	secret_key[SECRET_KEY_SIZE],
		  unsigned char		signature[SIGNATURE_SIZE]);
int verify_signature(const char		*string_message,
		     const unsigned char public_key[PUBLIC_KEY_SIZE],
		     const unsigned char signature[SIGNATURE_SIZE]);

#endif /* CRYPTO_H */
