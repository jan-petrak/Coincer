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
#include "trade.h"
#include "trade_basic.h"

/**
 * Delete trade.execution message.
 *
 * @param	trade_execution		The message.
 * @param	type			Trading type of the trade.execution.
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
