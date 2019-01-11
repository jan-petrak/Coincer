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

#define _POSIX_SOURCE /* fileno */

#include <dirent.h>
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "crypto.h"
#include "global_state.h"
#include "json_parser.h"
#include "log.h"
#include "routing.h"
#include "trade.h"
#include "trade_basic.h"

#define COMMITTED_HEX_LEN	2 * TRADE_BASIC_COMMITTED_SIZE
#define ORDER_ID_HEX_LEN	2 * SHA3_256_SIZE

static int trade_script_generate(trade_t *trade, global_state_t *global_state);
static int trade_script_originator(const trade_t *trade);
static int trade_script_validate(const trade_t *trade, const char *script);

/**
 * Cancel a trade.
 *
 * @param	trade		Trade to be cancelled.
 */
void trade_basic_cancel(trade_t *trade)
{
	trade_basic_t *trade_data;

	trade_data = (trade_basic_t *) trade->data;

	/* if we're still in the phase 0 */
	if (!trade_data->my_script || !trade_data->cp_script) {
		linkedlist_delete_safely(trade->node, trade_clear);
		return;
	}

	/* TODO: refund/claim coins here */
}

/**
 * Clear data of a trade.
 *
 * @param	data	Data of the trade.
 */
void trade_basic_clear(trade_basic_t *data)
{
	if (data->my_script) {
		free(data->my_script);
		data->my_script = NULL;
	}
	if (data->cp_script) {
		free(data->cp_script);
		data->cp_script = NULL;
	}
}

/**
 * Initialize data of a basic trade.
 *
 * @param	data	The trade data.
 */
void trade_basic_init_data(trade_basic_t *data)
{
	memset(data->x, 0x0, TRADE_BASIC_X_SIZE);
	memset(data->hx, 0x0, RIPEMD_160_SIZE);

	memset(data->my_commitment, 0x0, SHA3_256_SIZE);
	memset(data->my_committed, 0x0, TRADE_BASIC_COMMITTED_SIZE);
	data->my_script = NULL;

	memset(data->cp_commitment, 0x0, SHA3_256_SIZE);
	memset(data->cp_committed, 0x0, TRADE_BASIC_COMMITTED_SIZE);
	data->cp_script = NULL;
}

/**
 * Save data of a trade.
 *
 * @param	trade		The trade.
 * @param	trades_dir	Directory with trades of type basic.
 *
 * @return	0		Sucessfully saved.
 * @return	1		Failure.
 */
static int trade_basic_save(const trade_t *trade, const char *trades_dir)
{
	char		*file_path;
	FILE		*fp;
	char		*json_trade;
	char		order_id_hex[ORDER_ID_HEX_LEN + 1];

	/* sodium_bin2hex appends '\0' */
	sodium_bin2hex(order_id_hex,
		       sizeof(order_id_hex),
		       trade->order->id,
		       SHA3_256_SIZE);

	/* .../trades/basic/<order_id> */
	file_path = (char *) malloc(strlen(trades_dir) * sizeof(char) +
				    sizeof(order_id_hex));
	if (!file_path) {
		log_error("Creating file name for a trade");
		return 1;
	}

	strcpy(file_path, trades_dir);
	strcat(file_path, order_id_hex);

	/* make sure the file doesn't exist yet */
	fp = fopen(file_path, "r");
	if (fp) {
		log_error("Trade saving of possibly existing trade");
		free(file_path);
		fclose(fp);
		return 1;
	}
	fp = fopen(file_path, "w");
	if (!fp) {
		log_error("Creating file for a trade");
		free(file_path);
		return 1;
	}

	if (encode_trade(trade, &json_trade)) {
		log_error("Encoding a trade before saving");
		fclose(fp);
		remove(file_path);
		free(file_path);
		return 1;
	}

	if (fwrite(json_trade,
		   (strlen(json_trade) + 1) * sizeof(char),
		   1,
		   fp) != 1) {
		log_error("Saving a trade");
		fclose(fp);
		remove(file_path);
		free(file_path);
		free(json_trade);
		return 1;
	}

	if (fflush(fp) || fsync(fileno(fp))) {
		log_error("Incomplete trade saving");
		fclose(fp);
		remove(file_path);
		free(file_path);
		free(json_trade);
		return 1;
	}

	free(file_path);
	free(json_trade);
	fclose(fp);
	return 0;
}

/**
 * Update data of a basic trade.
 *
 * @param	trade		The trade.
 * @param	next_step	Next step of a trade.
 * @param	data		The next step's data.
 * @param	sender_id	The data received from this id.
 *
 * @return	0		Successfully updated.
 * @return	1		Incorrect input data.
 */
int trade_basic_update(trade_t		   *trade,
		       enum trade_step	   next_step,
		       const void	   *data,
		       const unsigned char *sender_id)
{
	enum trade_step		cur_step;
	trade_execution_basic_t *execution;
	trade_proposal_t	*proposal;
	trade_basic_t		*trade_data;

	cur_step = trade->step;
	trade_data = (trade_basic_t *) trade->data;

	if (next_step == TS_PROPOSAL) {
		proposal = (trade_proposal_t *) data;
	} else {
		execution = (trade_execution_basic_t *) data;
	}

	switch (next_step) {
		case TS_PROPOSAL:
			memcpy(trade_data->cp_commitment,
			       proposal->commitment,
			       SHA3_256_SIZE);
			break;
		case TS_COMMITMENT:
			memcpy(trade_data->cp_commitment,
			       execution->commitment,
			       SHA3_256_SIZE);
			memcpy(trade->cp_identifier,
			       sender_id,
			       PUBLIC_KEY_SIZE);
			break;
		case TS_KEY_AND_COMMITTED_EXCHANGE:
			memcpy(trade->cp_pubkey,
			       execution->pubkey,
			       PUBLIC_KEY_SIZE);
			if (hash_check(HT_SHA3_256,
				       trade_data->cp_commitment,
				       execution->committed,
				       TRADE_BASIC_COMMITTED_SIZE)) {
				return 1;
			}
			memcpy(trade_data->cp_committed,
			       execution->committed,
			       TRADE_BASIC_COMMITTED_SIZE);
			/* if the cp included the script in their message and if
			 * we've already sent our 'committed' and if
			 * it is the cp who should send the script first */
			if (execution->script != NULL &&
			    cur_step == TS_KEY_AND_COMMITTED_EXCHANGE &&
			    trade_script_originator(trade)) {
				if (trade_script_validate(trade,
							  execution->script)) {
					log_error("Counterparty's script "
						  "is invalid [step "
						  "TS_KEY_AND_"
						  "COMMITTED_EXCHANGE");
					return 1;
				}
				trade_data->cp_script = execution->script;
				execution->script = NULL;

				memcpy(trade_data->hx,
				       execution->hx,
				       RIPEMD_160_SIZE);
			}
			break;
		case TS_SCRIPT_ORIGIN:
			if (trade_script_validate(trade, execution->script)) {
				log_error("Counterparty's script is invalid "
					  "[step TS_SCRIPT_ORIGIN]");
				return 1;
			}
			trade_data->cp_script = execution->script;
			execution->script = NULL;

			memcpy(trade_data->hx, execution->hx, RIPEMD_160_SIZE);
			break;
		case TS_SCRIPT_RESPONSE:
			if (trade_script_validate(trade, execution->script)) {
				log_error("Counterparty's script is invalid "
					  "[step TS_SCRIPT_RESPONSE]");
				return 1;
			}
			trade_data->cp_script = execution->script;
			execution->script = NULL;
			break;
	}

	trade->step = next_step;
	return 0;
}

/**
 * Delete trade.execution of a basic trade.
 *
 * @param	execution	The execution to be deleted.
 * @param	step		The execution's step.
 */
void trade_execution_basic_delete(trade_execution_basic_t *execution,
				  enum trade_step	  step)
{
	if (step == TS_SCRIPT_ORIGIN || step == TS_SCRIPT_RESPONSE ||
	    step == TS_KEY_AND_COMMITTED_EXCHANGE) {
		if (execution->script) {
			free(execution->script);
			execution->script = NULL;
		}
	}
}

/**
 * Verify trade.execution of a basic trade.
 *
 * @param	execution	trade.execution to be verified.
 * @param	trade		trade.execution for this trade.
 * @param	sender_id	trade.execution received from this id.
 *
 * @return	0		trade.execution is legit.
 * @return	1		trade.execution is not legit and the trade
 *				should be aborted.
 * @return	2		trade.execution is not legit but the trade
 *				does not need to be aborted.
 */
int trade_execution_basic_verify(const trade_execution_t *execution,
				 const trade_t		 *trade,
				 const unsigned char	 *sender_id)
{
	const trade_execution_basic_t *exec_data;
	char			      new_id_hex[2 * PUBLIC_KEY_SIZE + 1];

	exec_data = (trade_execution_basic_t *) execution->data;

	/* counterparty's identifier must remain the same, unless this is
	 * the trade acceptance */
	if (memcmp(trade->cp_identifier, sender_id, PUBLIC_KEY_SIZE)) {
		if (trade->step != TS_PROPOSAL) {
			log_debug("trade_execution_basic_verify - received "
				  "trade.execution from a wrong peer");
			return 1;
		}
		sodium_bin2hex(new_id_hex,
			       sizeof(new_id_hex),
			       sender_id,
			       PUBLIC_KEY_SIZE);
		if (verify_signature(new_id_hex,
				     trade->cp_identifier,
				     exec_data->idsig)) {
			log_debug("trade_execution_basic_verify - received "
				  "trade.execution from a wrong peer");
			/* possible MITM attempt */
			return 2;
		}
	}

	if (memcmp(trade->order->id, execution->order, SHA3_256_SIZE)) {
		log_debug("trade_execution_basic_verify - counterparty's "
			  "trade.execution referring to a different order");
		return 1;
	}

	return 0;
}

/**
 * Initialize trade.proposal of a basic trade.
 *
 * @param	trade_proposal	Initialize this trade.proposal.
 * @param	trade_data	Get trade data from this.
 */
void trade_proposal_basic_init(trade_proposal_t		*trade_proposal,
			       const trade_basic_t	*trade_data)
{
	memcpy(trade_proposal->commitment,
	       trade_data->my_commitment,
	       SHA3_256_SIZE);
}

/**
 * Generate a trading script.
 *
 * @param	trade		Trade script for this trade.
 * @param	global_state	The global state.
 *
 * @return	0		Success.
 * @return	1		Failure.
 */
static int trade_script_generate(trade_t *trade, global_state_t *global_state)
{
	/* TODO: Implement */
	return 0;
}

/**
 * Determine who should be the first to generate a trading script.
 *
 * @param	trade	Trade script of this trade.
 *
 * @return	0	We are the one to create the first script.
 * @return	1	The trading counterparty creates the first script.
 */
static int trade_script_originator(const trade_t *trade)
{
	char		cp_committed_hex[COMMITTED_HEX_LEN + 1];
	char		my_committed_hex[COMMITTED_HEX_LEN + 1];
	char		order_id_hex[ORDER_ID_HEX_LEN + 1];

	unsigned char	cp_hash[SHA3_256_SIZE];
	unsigned char	my_hash[SHA3_256_SIZE];

	unsigned char	buf[ORDER_ID_HEX_LEN + 2 * COMMITTED_HEX_LEN];

	trade_basic_t *data = (trade_basic_t *) trade->data;

	/* sodium_bin2hex appends '\0' */
	sodium_bin2hex(order_id_hex,
		       sizeof(order_id_hex),
		       trade->order->id,
		       SHA3_256_SIZE);
	sodium_bin2hex(cp_committed_hex,
		       sizeof(cp_committed_hex),
		       data->cp_committed,
		       TRADE_BASIC_COMMITTED_SIZE);
	sodium_bin2hex(my_committed_hex,
		       sizeof(my_committed_hex),
		       data->my_committed,
		       TRADE_BASIC_COMMITTED_SIZE);

	memcpy(buf, order_id_hex, ORDER_ID_HEX_LEN);
	memcpy(buf + ORDER_ID_HEX_LEN,
	       cp_committed_hex,
	       COMMITTED_HEX_LEN);
	memcpy(buf + ORDER_ID_HEX_LEN + COMMITTED_HEX_LEN,
	       my_committed_hex,
	       COMMITTED_HEX_LEN);

	hash_message(HT_SHA3_256, buf, sizeof(buf), cp_hash);

	memcpy(buf, order_id_hex, ORDER_ID_HEX_LEN);
	memcpy(buf + ORDER_ID_HEX_LEN,
	       my_committed_hex,
	       COMMITTED_HEX_LEN);
	memcpy(buf + ORDER_ID_HEX_LEN + COMMITTED_HEX_LEN,
	       cp_committed_hex,
	       COMMITTED_HEX_LEN);

	hash_message(HT_SHA3_256, buf, sizeof(buf), my_hash);

	return memcmp(my_hash, cp_hash, SHA3_256_SIZE) > 0;
}

/**
 * Validate a trading script from our trading counterparty.
 *
 * @param	script	The script to be validated.
 *
 * @return	0	The script is valid.
 * @return	1	The script is invalid.
 */
static int trade_script_validate(const trade_t *trade, const char *script)
{
	/* TODO: Implement */
	return 0;
}

/**
 * Get the next step of a trade.
 *
 * @param	trade		The trade.
 *
 * @return	trade_step	The next trade step.
 */
enum trade_step trade_step_basic_get_next(const trade_t *trade)
{
	trade_basic_t *trade_data = (trade_basic_t *) trade->data;

	switch (trade->step) {
		case TS_PROPOSAL:
			return TS_COMMITMENT;
		case TS_COMMITMENT:
			return TS_KEY_AND_COMMITTED_EXCHANGE;
		case TS_KEY_AND_COMMITTED_EXCHANGE:
			if (identifier_empty(trade->my_keypair.public_key) ||
			    identifier_empty(trade->cp_pubkey)) {
				return TS_KEY_AND_COMMITTED_EXCHANGE;
			}
			if (trade_data->cp_script || trade_data->my_script) {
				return TS_SCRIPT_RESPONSE;
			}
			return TS_SCRIPT_ORIGIN;
		case TS_SCRIPT_ORIGIN:
			return TS_SCRIPT_RESPONSE;
		case TS_SCRIPT_RESPONSE:
			return TS_COINS_COMMITMENT;
		case TS_COINS_COMMITMENT:
			return TS_COINS_CP_COMMITMENT;
		case TS_COINS_CP_COMMITMENT:
			return TS_COINS_CLAIM;
		case TS_COINS_CLAIM:
			return TS_DONE;
		case TS_DONE:
			return TS_DONE;
	}
}

/**
 * Perform current trade step.
 *
 * @param	trade		Perform the step on this trade.
 * @param	global_state	The global state.
 *
 * @return	0		Success.
 * @return	1		Failure.
 */
int trade_step_basic_perform(trade_t *trade, global_state_t *global_state)
{
	unsigned char	tmp_sha_hash[SHA3_256_SIZE];
	trade_basic_t	*trade_data;
	const char	*trades_basic_dir;

	trade_data = (trade_basic_t *) trade->data;
	trades_basic_dir = global_state->filepaths.trades_basic_dir;

	switch (trade->step) {
		case TS_PROPOSAL:
			fetch_random_value(trade_data->my_committed,
					   TRADE_BASIC_COMMITTED_SIZE);
			hash_message(HT_SHA3_256,
				     trade_data->my_committed,
				     TRADE_BASIC_COMMITTED_SIZE,
				     trade_data->my_commitment);
			break;
		case TS_COMMITMENT:
			fetch_random_value(trade_data->my_committed,
					   TRADE_BASIC_COMMITTED_SIZE);
			hash_message(HT_SHA3_256,
				     trade_data->my_committed,
				     TRADE_BASIC_COMMITTED_SIZE,
				     trade_data->my_commitment);

			if (send_trade_execution(&global_state->routing_table,
						 trade)) {
				log_error("Sending trade.execution "
					  "[step TS_COMMITMENT]");
				return 1;
			}
			break;
		case TS_KEY_AND_COMMITTED_EXCHANGE:
			generate_keypair(&trade->my_keypair);
			if (!identifier_empty(trade->cp_pubkey) &&
			    !trade_script_originator(trade)) {
				if (trade_script_generate(trade,
							  global_state)) {
					log_error("Creating a trade script "
						  "[step TS_KEY_AND_"
						  "COMMITTED_EXCHANGE");
					return 1;
				}
			}

			if (send_trade_execution(&global_state->routing_table,
						 trade)) {
				log_error("Sending trade.execution "
					  "[step TS_KEY_AND_"
					  "COMMITTED_EXCHANGE]");
				return 1;
			}
			break;
		case TS_SCRIPT_ORIGIN:
			fetch_random_value(trade_data->x, TRADE_BASIC_X_SIZE);
			hash_message(HT_SHA3_256,
				     trade_data->x,
				     TRADE_BASIC_X_SIZE,
				     tmp_sha_hash);
			hash_message(HT_RIPEMD_160,
				     tmp_sha_hash,
				     SHA3_256_SIZE,
				     trade_data->hx);
			if (trade_script_generate(trade, global_state)) {
				log_error("Creating a trade script "
					  "[step TS_SCRIPT_ORIGIN]");
				return 1;
			}

			if (send_trade_execution(&global_state->routing_table,
						 trade)) {
				log_error("Sending trade.execution "
					  "[step TS_SCRIPT_ORIGIN]");
				return 1;
			}
			break;
		case TS_SCRIPT_RESPONSE:
			if (trade_script_generate(trade, global_state)) {
				log_error("Creating a trade script "
					  "[step TS_SCRIPT_RESPONSE]");
				return 1;
			}

			if (send_trade_execution(&global_state->routing_table,
						 trade)) {
				log_error("Sending trade.execution "
					  "[step TS_SCRIPT_RESPONSE]");
				return 1;
			}
			break;
		case TS_COINS_COMMITMENT:
			if (trade_basic_save(trade, trades_basic_dir)) {
				log_error("Saving a trade");
				return 1;
			}

			/* TODO: Commit coins */

			if (!(trade->order->flags & ORDER_FOREIGN)) {
				send_market_cancel(&global_state->neighbours,
						   trade->order);
			}
			break;
	}

	return 0;
}

/**
 * Load saved basic trades into memory.
 *
 * @param	trades			Store loaded trades in here.
 * @param	trades_basic_dir	Path to directory with basic trades.
 *
 * @return	0			Successfully loaded.
 * @return	1			Failure.
 */
int trades_basic_load(linkedlist_t	*trades,
		      const char	*trades_basic_dir)
{
	char		buffer[65535];
	DIR		*dir;
	struct dirent	*dir_entry;
	size_t		length;
	trade_t		*trade;
	FILE		*trade_file;
	const char	*trade_file_name;

	if (!(dir = opendir(trades_basic_dir))) {
		log_error("Opening dir with basic trades");
		return 1;
	}

	while ((dir_entry = readdir(dir))) {
		trade_file_name = dir_entry->d_name;
		if (strlen(trade_file_name) != ORDER_ID_HEX_LEN) {
			continue;
		}
		if (!(trade_file = fopen(trade_file_name, "r"))) {
			log_error("Failed to open trade %s", trade_file_name);
			continue;
		}

		fseek(trade_file, 0, SEEK_END);
		length = ftell(trade_file);
		if (length < 50 || length > 65000) {
			log_debug("trades_basic_load - attempt to load trade "
				  "of %lu chars, the file name: %s",
				  length, trade_file_name);
			fclose(trade_file);
			continue;
		}

		rewind(trade_file);
		if (fread(buffer,
			  sizeof(char),
			  length,
			  trade_file) != length) {
			log_error("Reading a trade file %s failed",
				  trade_file_name);
			fclose(trade_file);
			continue;
		}
		buffer[length - 1] = '\0';

		if (decode_trade(buffer, &trade)) {
			log_error("Decoding trade %s", trade_file_name);
			fclose(trade_file);
			continue;
		}
		if (!(trade->node = linkedlist_append(trades, trade))) {
			log_error("Decoding trade %s", trade_file_name);
			trade_clear(trade);
			fclose(trade_file);
			continue;
		}
		log_info("Trade %s successfully loaded", trade_file_name);
		fclose(trade_file);
	}

	closedir(dir);
	return 0;
}
