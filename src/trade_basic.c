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
#include "json_parser.h"
#include "trade.h"
#include "trade_basic.h"

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
		}
	}
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
