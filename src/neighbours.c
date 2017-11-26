#include <assert.h>
#include <event2/bufferevent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "neighbours.h"

void neighbours_init(struct Neighbours *neighbours)
{
	neighbours->first = neighbours->last = NULL;
	neighbours->size  = 0;
}

struct Neighbour *find_neighbour(const struct Neighbours 	*neighbours,
				 const struct bufferevent 	*bev)
{

	struct Neighbour *current = neighbours->first;

	while (current != NULL) {
		if (current->buffer_event == bev) {
			return current;
		}
		current = current->next;
	}

	return NULL;
}

struct Neighbour *find_neighbour_by_ip(const struct Neighbours	*neighbours,
				       const char 		*ip_addr)
{
	struct Neighbour *current = neighbours->first;

	while (current != NULL) {
		if (strcmp(current->ip_addr, ip_addr) == 0) {
			return current;
		}
		current = current->next;
	}

	return NULL;

}

struct Neighbour *add_new_neighbour(struct Neighbours 	*neighbours, 
				    const char 		*ip_addr,
				    struct bufferevent 	*bev)
{
	struct Neighbour *new_neighbour = (struct Neighbour *)
		malloc(sizeof(struct Neighbour));

	if (find_neighbour_by_ip(neighbours, ip_addr)) {
		return NULL;
	}

	assert(new_neighbour != NULL);

	strcpy(new_neighbour->ip_addr, ip_addr);
	new_neighbour->failed_pings = 0;
	new_neighbour->next = NULL;
	new_neighbour->buffer_event = bev;


	if (neighbours->last == NULL) {
		neighbours->first = neighbours->last = new_neighbour;
	} else {
		neighbours->last->next = new_neighbour;
		neighbours->last = new_neighbour;
	}

	neighbours->size += 1;

	return new_neighbour;
}

void delete_neighbour(struct Neighbours *neighbours, struct bufferevent *bev)
{
	struct Neighbour *current = neighbours->first;
	struct Neighbour *neighbour = find_neighbour(neighbours, bev);

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

void clear_neighbours(struct Neighbours *neighbours)
{
	struct Neighbour *current = neighbours->first;

	while (current != NULL) {

		struct Neighbour *temp = current;
		current = current->next;

		if (temp->buffer_event != NULL) {
			bufferevent_free(temp->buffer_event);
		}

		free(temp);
	}

	neighbours_init(neighbours);
}






