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

#ifndef TRADE_H
#define TRADE_H

#include "crypto.h"
#include "global_state.h"
#include "linkedlist.h"
#include "market.h"
#include "paths.h"
#include "peers.h"

/**
 * Trading protocol type.
 */
enum trade_type {
	TT_BASIC
};

/**
 * Step of a trade.
 */
enum trade_step {
	TS_PROPOSAL, /**< Received or sent a trade.proposal. */
	TS_COMMITMENT, /**< Received or sent hashed 'committed'. */
	TS_KEY_AND_COMMITTED_EXCHANGE, /**< Pubkey and 'committed' exchange. */
	TS_SCRIPT_ORIGIN, /**< Received or sent the first script. */
	TS_SCRIPT_RESPONSE, /**< Received or sent the second script. */
	TS_COINS_COMMITMENT, /**< Committing coins into a blackchain. */
	/** Waiting for the counterparty to commit their coins. */
	TS_COINS_CP_COMMITMENT,
	TS_COINS_CLAIM, /**< Coins claim. */
	TS_DONE /**< Trade successfully completed. */
};

/**
 * Trade data holder.
 */
typedef struct s_trade {
	order_t *order; /**< Trade refering to this order. */

	enum trade_type type; /**< Type of the trade. */
	enum trade_step step; /**< Current step of the trade. */

	identity_t *my_identity; /**< Process the trade under this identity. */
	keypair_t my_keypair; /**< Our trading keypair. */

	/** Counterparty's message sending identifier. */
	unsigned char cp_identifier[PUBLIC_KEY_SIZE];
	/** Counterparty's trading public key. */
	unsigned char cp_pubkey[PUBLIC_KEY_SIZE];

	void *data; /**< Trade data. */
	linkedlist_node_t *node; /**< Trade's node in the LL of trades. */
} trade_t;

/**
 * trade.execution data holder.
 */
typedef struct s_trade_execution {
	/** trade.execution related to this order. */
	unsigned char order[SHA3_256_SIZE];
	/** trade.execution's data. */
	void *data;
} trade_execution_t;

/**
 * trade.proposal data holder.
 */
typedef struct s_trade_proposal {
	enum trade_type protocol; /**< Type of the trading protocol. */
	unsigned char order[SHA3_256_SIZE]; /**< Order id. */
	unsigned char commitment[SHA3_256_SIZE]; /**< Optional commitment. */
} trade_proposal_t;

/**
 * trade.reject data holder.
 */
typedef struct s_trade_reject {
	unsigned char order[SHA3_256_SIZE]; /**< Order id. */
} trade_reject_t;

void trade_cancel(trade_t *trade);
void trade_clear(trade_t *trade);
int trade_cmp_identity(const trade_t *trade, const identity_t *identity);
int trade_cmp_order_id(const trade_t *trade, const unsigned char *order_id);
trade_t *trade_create(linkedlist_t	  *trades,
		      linkedlist_t	  *identities,
		      order_t		  *order,
		      const unsigned char *cp_id,
		      enum trade_type	  type);
trade_t *trade_find(const linkedlist_t	*trades,
		    const void		*attribute,
		    int (*cmp_func) (const trade_t	*trade,
				    const void		*attribute));
int trade_update(trade_t *trade, enum trade_step next_step, const void *data);

void trade_execution_delete(trade_execution_t	*data,
			    enum trade_type	type,
			    enum trade_step	step);

void trade_proposal_init(trade_proposal_t	*trade_proposal,
			 const trade_t		*trade);

enum trade_step trade_step_get_next(const trade_t *trade);
int trade_step_perform(trade_t *trade, global_state_t *global_state);

int trades_load(linkedlist_t *trades, filepaths_t *paths);

#endif /* TRADE_H */
