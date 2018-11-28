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

#ifndef TRADE_BASIC_H
#define TRADE_BASIC_H

#include "crypto.h"
#include "global_state.h"
#include "trade.h"

#define TRADE_BASIC_COMMITTED_SIZE	32
#define TRADE_BASIC_X_SIZE		32

/** Basic trade data. */
typedef struct s_trade_basic {
	unsigned char x[TRADE_BASIC_X_SIZE]; /**< Secret 'x'. */
	unsigned char hx[RIPEMD_160_SIZE]; /**< Double hash of 'x'. */

	unsigned char my_commitment[SHA3_256_SIZE]; /**< Our commitment. */
	/** Our committed. */
	unsigned char my_committed[TRADE_BASIC_COMMITTED_SIZE];
	char *my_script; /**< Our trading script. */

	/** Counterparty's commitment. */
	unsigned char cp_commitment[SHA3_256_SIZE];
	/** Counterparty's committed. */
	unsigned char cp_committed[TRADE_BASIC_COMMITTED_SIZE];
	/** Counterparty's trading script. */
	char *cp_script;
} trade_basic_t;

/**
 * trade.execution's data for the basic type of trading protocol.
 */
typedef struct s_trade_execution_basic {
	/** Hashed committed. */
	unsigned char commitment[SHA3_256_SIZE];
	/** Random number to determine who's the first to create trading
	 *  script. */
	unsigned char committed[TRADE_BASIC_COMMITTED_SIZE];
	/** Double hash of the secret 'x' from the first trading script. */
	unsigned char hx[RIPEMD_160_SIZE];
	/** Trading public key. */
	unsigned char pubkey[PUBLIC_KEY_SIZE];
	/** Trading script. */
	char *script;
} trade_execution_basic_t;

void trade_basic_cancel(trade_t *trade);
void trade_basic_clear(trade_basic_t *data);
void trade_basic_init_data(trade_basic_t *data);
int trade_basic_update(trade_t		*trade,
		       enum trade_step	next_step,
		       const void	*data);

void trade_execution_basic_delete(trade_execution_basic_t *execution,
				  enum trade_step	  step);

void trade_proposal_basic_init(trade_proposal_t		*trade_proposal,
			       const trade_basic_t	*trade_data);

enum trade_step trade_step_basic_get_next(const trade_t *trade);
int trade_step_basic_perform(trade_t *trade, global_state_t *global_state);

int trades_basic_load(linkedlist_t *trades, const char *trades_basic_dir);

#endif /* TRADE_BASIC_H */
