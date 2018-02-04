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

#include <event2/bufferevent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "neighbours.h"

void neighbours_init(struct s_neighbours *neighbours)
{
	neighbours->first = neighbours->last = NULL;
	neighbours->size  = 0;
}

struct s_neighbour *find_neighbour(const struct s_neighbours *neighbours,
				   const struct bufferevent  *bev)
{
	struct s_neighbour *current = neighbours->first;

	while (current != NULL) {
		if (current->buffer_event == bev) {
			return current;
		}
		current = current->next;
	}

	return NULL;
}

struct s_neighbour *find_neighbour_by_ip(const struct s_neighbours *neighbours,
					 const char *ip_addr)
{
	struct s_neighbour *current = neighbours->first;

	while (current != NULL) {
		if (strcmp(current->ip_addr, ip_addr) == 0) {
			return current;
		}
		current = current->next;
	}

	return NULL;
}

struct s_neighbour *add_new_neighbour(struct s_neighbours *neighbours,
				      const char	  *ip_addr,
				      struct bufferevent  *bev)
{
	struct s_neighbour *new_neighbour;
	
	new_neighbour = (struct s_neighbour *)
				malloc(sizeof(struct s_neighbour));
	if (new_neighbour == NULL) {
		/* WIP */
		perror("malloc for a new neighbour");
                return NULL;
	}

	/* don't add duplicates */
	if (find_neighbour_by_ip(neighbours, ip_addr)) {
		return NULL;
	}

	/* TODO: comment why it is safe to use strcpy */
	strcpy(new_neighbour->ip_addr, ip_addr);
	new_neighbour->failed_pings = 0;
	new_neighbour->next = NULL;
	new_neighbour->buffer_event = bev;

	/* TODO: comment */
	if (neighbours->last == NULL) {
		neighbours->first = neighbours->last = new_neighbour;
	} else {
		neighbours->last->next = new_neighbour;
		neighbours->last = new_neighbour;
	}

	neighbours->size += 1;

	return new_neighbour;
}

void delete_neighbour(struct s_neighbours *neighbours, struct bufferevent *bev)
{
	struct s_neighbour *current   = neighbours->first;
	struct s_neighbour *neighbour = find_neighbour(neighbours, bev);

	if (current == NULL || neighbour == NULL) {
		return;
	}

	if (neighbour->buffer_event != NULL) {
		bufferevent_free(bev);
	}

	if (neighbours->first == neighbour) {
		if (neighbours->first == neighbours->last) {
			neighbours->first = neighbours->last = NULL;
		} else {
			neighbours->first = neighbours->first->next;
		}

		free(neighbour);
		neighbours->size = 0;
		return;
	}

	while (current != NULL) {
		if (current->next == neighbour) {
			current->next = neighbour->next;

			if (current->next == NULL) {
				neighbours->last = current;
			}

			free(neighbour);
			neighbours->size -= 1;
			break;
		}
		current = current->next;
	}
}

void clear_neighbours(struct s_neighbours *neighbours)
{
	struct s_neighbour *current = neighbours->first;

	while (current != NULL) {

		struct s_neighbour *temp = current;
		current = current->next;

		if (temp->buffer_event != NULL) {
			bufferevent_free(temp->buffer_event);
		}

		free(temp);
	}

	neighbours_init(neighbours);
}
