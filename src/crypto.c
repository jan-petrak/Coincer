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
#include "log.h"

/**
 * Decrypt a message into a parameter that is dynamically allocated in here.
 *
 * @param	message		The message to be decrypted.
 * @param	public_key	The public key used for decryption.
 * @param	secret_key	The secret key used for decryption.
 * @param	decrypted	Dynamically allocated decrypted message.
 *
 * @return	0		Successfully decrypted.
 * @return	1		Failure.
 */
int decrypt_message(const char		*message,
		    const unsigned char public_key[PUBLIC_KEY_SIZE],
		    const unsigned char	secret_key[SECRET_KEY_SIZE],
		    char		**decrypted)
{
	size_t		decrypted_len;
	unsigned char	*msg_bin;
	size_t		msg_bin_len;
	size_t		msg_len;
	int		res;

	msg_len	      = strlen(message);
	msg_bin_len   = msg_len >> 1;
	decrypted_len = msg_bin_len - crypto_box_SEALBYTES;

	msg_bin = (unsigned char *) malloc(msg_bin_len * sizeof(unsigned char));
	if (!msg_bin) {
		log_error("Allocation of an encrypted bin message");
		return 1;
	}
	*decrypted = (char *) malloc((decrypted_len + 1) * sizeof(char));
	if (!*decrypted) {
		log_error("Allocation of a decrypted message");
		free(msg_bin);
		return 1;
	}

	if (sodium_hex2bin(msg_bin,
			   (msg_bin_len * sizeof(char)),
			   message,
			   strlen(message),
			   NULL,
			   NULL,
			   NULL)) {
		log_error("Converting hex to bin during message decryption");
		free(msg_bin);
		free(*decrypted);
		return 1;
	}

	if (!(res = crypto_box_seal_open((unsigned char *) *decrypted,
					 msg_bin,
					 msg_bin_len,
					 public_key,
					 secret_key))) {
		(*decrypted)[decrypted_len] = '\0';
	} else {
		free(*decrypted);
	}

	free(msg_bin);

	return res;
}

/**
 * Encrypt a message into a parameter that is dynamically allocated in here.
 *
 * @param	message		The message to be encrypted.
 * @param	public_key	The public key used for encryption.
 * @param	encrypted	Dynamically allocated encrypted hex message.
 *
 * @return	0		Successfully encrypted.
 * @return	1		Failure.
 */
int encrypt_message(const char		*message,
		    const unsigned char public_key[PUBLIC_KEY_SIZE],
		    char		**encrypted)
{
	unsigned char	*encrypted_bin;
	size_t		encrypted_bin_len;
	size_t		encrypted_len;
	size_t		msg_len;

	msg_len		  = strlen(message);
	encrypted_bin_len = crypto_box_SEALBYTES + msg_len;
	encrypted_len	  = encrypted_bin_len << 1;

	encrypted_bin = (unsigned char *) malloc(encrypted_bin_len *
						 sizeof(unsigned char));
	if (!encrypted_bin) {
		log_error("Allocation of an encrypted bin buffer");
		return 1;
	}
	*encrypted = (char *) malloc((encrypted_len + 1) * sizeof(char));
	if (!*encrypted) {
		log_error("Allocation of an encrypted hex message");
		free(encrypted_bin);
		return 1;
	}

	crypto_box_seal(encrypted_bin,
			(const unsigned char *) message,
			msg_len,
			public_key);

	sodium_bin2hex(*encrypted,
		       ((encrypted_len + 1) * sizeof(char)),
		       encrypted_bin,
		       (encrypted_bin_len * sizeof(char)));

	free(encrypted_bin);
	return 0;
}

/**
 * Fetch random value.
 *
 * @param	value		Store the random value in here.
 * @param	value_size	Size of the value in bytes.
 */
void fetch_random_value(void *value, size_t value_size)
{
	randombytes_buf(value, value_size);
}

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
 * Check hash of a message.
 *
 * @param	type		Hashing function.
 * @param	hash		Hash to be checked.
 * @param	message		Hash of this message.
 * @param	message_size	Size of the message.
 *
 * @return	0		The hashes match.
 * @return	1		The hashes don't match.
 */
int hash_check(enum hash_type		type,
	       const unsigned char	*hash,
	       const unsigned char	*message,
	       size_t			message_size)
{
	/* TODO: Implement using wolfssl */

	switch (type) {
		case HT_SHA3_256:
			break;
		case HT_RIPEMD_160:
			break;
	}

	return 0;
}

/**
 * Hash a message.
 *
 * @param	type		Hashing function.
 * @param	message		Hash this message.
 * @param	message_size	Size of the message.
 * @param	hash		The result hash.
 */
void hash_message(enum hash_type	type,
		  const unsigned char	*message,
		  size_t		message_size,
		  unsigned char		*hash)
{
	/* TODO: Implement using wolfssl */

	switch (type) {
		case HT_SHA3_256:
			break;
		case HT_RIPEMD_160:
			break;
	}
}

/**
 * Sign a string message with a secret key.
 *
 * @param	string_message		Sign this message.
 * @param	secret_key		With this key.
 * @param	signature		Store the result in here.
 */
void sign_message(const char		*string_message,
		  const unsigned char	secret_key[SECRET_KEY_SIZE],
		  unsigned char		signature[SIGNATURE_SIZE])
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
		     const unsigned char public_key[PUBLIC_KEY_SIZE],
		     const unsigned char signature[SIGNATURE_SIZE])
{
	return crypto_sign_verify_detached(signature,
					(const unsigned char *) string_message,
					crypto_sign_BYTES,
					public_key);
}
