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

#include <stdlib.h>
#include <string.h>

#include "crypto.h"
#include "global_state.h"
#include "linkedlist.h"
#include "log.h"
#include "market.h"
#include "paths.h"
#include "peers.h"
#include "trade.h"
#include "trade_basic.h"

/**
 * Cancel a trade.
 *
 * @param	trade	Trade to be cancelled.
 */
void trade_cancel(trade_t *trade)
{
	switch (trade->type) {
		case TT_BASIC:
			trade_basic_cancel(trade);
			break;
	}

	order_flags_unset(trade->order, ORDER_TRADING);

	linkedlist_delete_safely(trade->node, trade_clear);
}

/**
 * Clear trade's data.
 *
 * @param	trade	Trade to be cleared.
 */
void trade_clear(trade_t *trade)
{
	switch (trade->type) {
		case TT_BASIC:
			trade_basic_clear(trade->data);
			break;
	}

	if (trade->my_identity) {
		linkedlist_delete(trade->my_identity->node);
	}
}

/**
 * Compare our trading identity to other identity.
 *
 * @param	trade		Trade with our trading identity.
 * @param	identity	Compare to this identity.
 *
 * @return	0		The identities match.
 * @return	1		The identities are different.
 */
int trade_cmp_identity(const trade_t *trade, const identity_t *identity)
{
	return trade->my_identity != identity;
}

/**
 * Compare order ID of a trade to other order ID.
 *
 * @param	trade		Order ID of this trade.
 * @param	order_id	Compare to this order ID.
 *
 * @return	0		The IDs match.
 * @return	1		The IDs are different.
 */
int trade_cmp_order_id(const trade_t *trade, const unsigned char *order_id)
{
	return memcmp(trade->order->id, order_id, SHA3_256_SIZE);
}

/**
 * Create and store a new trade including a newly generated identity.
 *
 * @param	trades		List of our trades.
 * @param	identities	List of our identities.
 * @param	order		Trade related to this order.
 * @param	cp_id		Counterparty's identifier.
 * @param	type		Type of the trade.
 *
 * @return	trade_t		Trade successfully created.
 * @return	NULL		Failure.
 */
trade_t *trade_create(linkedlist_t	  *trades,
		      linkedlist_t	  *identities,
		      order_t		  *order,
		      const unsigned char *cp_id,
		      enum trade_type	  type)
{
	trade_t *trade;

	trade = (trade_t *) malloc(sizeof(trade_t));
	if (!trade) {
		log_error("Creating a trade");
		return NULL;
	}
	if (!(trade->node = linkedlist_append(trades, trade))) {
		free(trade);
		log_error("Storing a new trade");
		return NULL;
	}
	if (!(trade->my_identity = identity_generate(identities, 0x0))) {
		linkedlist_delete(trade->node);
		log_error("Creating trade-specific identity");
		return NULL;
	}

	switch (type) {
		case TT_BASIC:
			if (!(trade->data = malloc(sizeof(trade_basic_t)))) {
				linkedlist_delete_safely(trade->node,
							 trade_clear);
				log_error("Creating trade data");
				return NULL;
			}
			trade_basic_init_data(trade->data);
			break;
	}

	trade->order = order;
	trade->type  = type;
	trade->step  = TS_PROPOSAL;
	memset(&trade->my_keypair, 0x0, sizeof(keypair_t));
	memcpy(trade->cp_identifier, cp_id, PUBLIC_KEY_SIZE);
	memset(trade->cp_pubkey, 0x0, PUBLIC_KEY_SIZE);

	return trade;
}

/**
 * Find a trade by one of its attributes.
 *
 * @param	trades		List of our trades.
 * @param	attribute	Find by this attribute.
 * @param	cmp_func	Comparing function. Returns 0 for a match.
 *
 * @return	trade_t		The trade we want.
 * @return	NULL		Trade unknown.
 */
trade_t *trade_find(const linkedlist_t	*trades,
		    const void		*attribute,
		    int (*cmp_func) (const trade_t	*trade,
				    const void		*attribute))
{
	const linkedlist_node_t *node;
	trade_t			*trade;

	node = linkedlist_get_first(trades);
	while (node) {
		trade = (trade_t *) node->data;

		if (!cmp_func(trade, attribute)) {
			return trade;
		}

		node = linkedlist_get_next(trades, node);
	}

	return NULL;
}

/**
 * Update trade's data with the next trading step.
 *
 * @param	trade		Trade to be updated.
 * @param	next_step	Next step of a trade.
 * @param	data		Next step's data.
 * @param	sender_id	The data received from this id.
 *
 * @return	0		Successfully updated.
 * @return	1		Incorrect data.
 */
int trade_update(trade_t		*trade,
		 enum trade_step	next_step,
		 const void		*data,
		 const unsigned char	*sender_id)
{
	switch (trade->type) {
		case TT_BASIC:
			return trade_basic_update(trade,
						  next_step,
						  data,
						  sender_id);
	}

	return 1;
}

/**
 * Delete trade.execution payload.
 *
 * @param	trade_execution		The payload to be deleted.
 * @param	type			Type of execution's trade.
 * @param	step			trade.execution's step.
 */
void trade_execution_delete(trade_execution_t	*trade_execution,
			    enum trade_type	type,
			    enum trade_step	step)
{
	switch (type) {
		case TT_BASIC:
			trade_execution_basic_delete(trade_execution->data,
						     step);
			break;
	}

	free(trade_execution);
}

/**
 * Verify trade.execution of a trade.
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
int trade_execution_verify(const trade_execution_t	*execution,
			   const trade_t		*trade,
			   const unsigned char		*sender_id)
{
	switch (trade->type) {
		case TT_BASIC:
			return trade_execution_basic_verify(execution,
							    trade,
							    sender_id);
	}
}

/**
 * Initialize trade.proposal of a trade.
 *
 * @param	trade_proposal	Initialize this trade.proposal.
 * @param	trade		Get data from this trade.
 */
void trade_proposal_init(trade_proposal_t *trade_proposal, const trade_t *trade)
{
	trade_proposal->protocol = trade->type;
	memcpy(trade_proposal->order, trade->order->id, SHA3_256_SIZE);
	memset(trade_proposal->commitment, 0x0, SHA3_256_SIZE);

	switch (trade->type) {
		case TT_BASIC:
			trade_proposal_basic_init(trade_proposal, trade->data);
	}
}

/**
 * Get the next step of a trade.
 *
 * @param	trade		The next step of this trade.
 *
 * @return	trade_step	The next step.
 */
enum trade_step trade_step_get_next(const trade_t *trade)
{
	switch (trade->type) {
		case TT_BASIC:
			return trade_step_basic_get_next(trade);
	}
}

/**
 * Perform the current step of a trade.
 *
 * @param	trade		Use this trade.
 * @param	global_state	The global state.
 *
 * @return	0		Success.
 * @return	1		Failure.
 */
int trade_step_perform(trade_t *trade, global_state_t *global_state)
{
	switch (trade->type) {
		case TT_BASIC:
			return trade_step_basic_perform(trade, global_state);
	}
}

/**
 * Load saved trades into memory.
 *
 * @param	trades		Store loaded trades in here.
 * @param	paths		Paths to trade directories.
 *
 * @return	0		Successfully loaded.
 * @return	1		Failure.
 */
int trades_load(linkedlist_t *trades, filepaths_t *paths)
{
	return trades_basic_load(trades, paths->trades_basic_dir);
}
