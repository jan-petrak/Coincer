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

#ifndef DAEMON_MESSAGES_PROCESSOR_H
#define DAEMON_MESSAGES_PROCESSOR_H

#include "global_state.h"
#include "neighbours.h"

/** Result of message processing. */
enum process_message_result {
	PMR_DONE,
	PMR_ERR_INTEGRITY,
	PMR_ERR_INTERNAL,
	PMR_ERR_PARSING,
	PMR_ERR_SEMANTIC,
	PMR_ERR_VERSION
};

enum process_message_result process_encoded_message(
						const char	*json_message,
						neighbour_t	*sender,
						global_state_t	*global_state);

#endif /* DAEMON_MESSAGES_PROCESSOR_H */
