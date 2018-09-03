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

#include <sodium.h>
#include <stdint.h>
#include <string.h>

#include "crypto.h"

/**
 * Generate a keypair.
 *
 * @param	keypair		Store the generated keypair in here.
 */
void generate_keypair(keypair_t *keypair)
{
	crypto_box_keypair(keypair->public_key, keypair->secret_key);
}

/**
 * Generate random 32-bit unsigned integer.
 *
 * @param	upper_bound		The random number upper bound
 *					(excluded).
 *
 * @return	<0; upper_bound)	The random number.
 */
uint32_t get_random_uint32_t(uint32_t upper_bound)
{
	return randombytes_uniform(upper_bound);
}

/**
 * Generate random 64-bit unsigned integer.
 *
 * @return	<0; UINT64_MAX)		The random number.
 */
uint64_t get_random_uint64_t(void)
{
	uint64_t number;

	randombytes_buf(&number, sizeof(uint64_t));

	return number;
}

/**
 * Create a hash of a string.
 *
 * @param	string_message	Create a hash of this string.
 * @param	hash		Store the hash in here.
 *
 * @return	0		Hash successfully created.
 * @return	-1		Hashing failed.
 */
int hash_message(const char		*string_message,
		 unsigned char		hash[crypto_generichash_BYTES])
{
	/* from libsodium doc: 'key can be NULL and keylen can be 0. In this
	 * case, a message will always have the same fingerprint, similar to
	 * the MD5 or SHA-1 functions for which crypto_generichash() is a
	 * faster and more secure alternative.'
	 */
	return crypto_generichash(hash,
				  crypto_generichash_BYTES,
				  (const unsigned char *) string_message,
				  strlen(string_message),
				  NULL,
				  0);
}

/**
 * Sign a string message with a secret key.
 *
 * @param	string_message		Sign this message.
 * @param	secret_key		With this key.
 * @param	signature		Store the result in here.
 */
void sign_message(const char		*string_message,
		  const unsigned char	secret_key[crypto_box_SECRETKEYBYTES],
		  unsigned char		signature[crypto_sign_BYTES])
{
	/* from libsodium doc: It is safe to ignore siglen and always consider
	 * a signature as crypto_sign_BYTES bytes long: shorter signatures
	 * will be transparently padded with zeros if necessary.
	 */
	crypto_sign_detached(signature,
			     NULL,
			     (const unsigned char *) string_message,
			     strlen(string_message) * sizeof(char),
			     secret_key);
}

/**
 * Verify string message signature.
 *
 * @param	string_message		The message to be verified.
 * @param	public_key		Verify with this public key.
 * @param	signature		The signature to be verified.
 *
 * @param	0			The signature is valid.
 * @param	1			Invalid signature.
 */
int verify_signature(const char		 *string_message,
		     const unsigned char public_key[crypto_box_PUBLICKEYBYTES],
		     const unsigned char signature[crypto_sign_BYTES])
{
	return crypto_sign_verify_detached(signature,
					(const unsigned char *) string_message,
					crypto_sign_BYTES,
					public_key);
}
