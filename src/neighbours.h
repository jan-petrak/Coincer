#ifndef NEIGHBOURS_H
#define NEIGHBOURS_H

#include <event2/bufferevent.h>
#include <stddef.h>

/* data type for the linked list of neighbours */
struct Neighbour {

	/* neighbours's IPv6 address */
	char ip_addr[46];
	/* bufferevent belonging to this neighbour */
	struct bufferevent *buffer_event;
	/* number of failed ping attempts - max 3, then disconnect */
	size_t failed_pings;

	/* next Neighbour in the linked list - at struct Neighbours */
	struct Neighbour *next;
};

/* linked list of Neighbours */
struct Neighbours {

	/* number of neighbours we are connected to */	
	size_t size;
	struct Neighbour *first;
	struct Neighbour *last;
};

/* set linked list variables to their default values */
void neighbours_init(struct Neighbours *neighbours);

/* find neighbour in neighbours based on their buffer_event,
   returns NULL if not found */
struct Neighbour *find_neighbour(const struct Neighbours 	*neighbours,
				 const struct bufferevent 	*bev);

/* find neighbour in neighbours based on their ip_addr,
   returns NULL if not found */
struct Neighbour *find_neighbour_by_ip(const struct Neighbours	*neighbours,
				       const char 		*ip_addr);
/* add new neighbour into neighbours
   returns NULL if neighbour already exists */
struct Neighbour* add_new_neighbour(struct Neighbours	*neighbours,
				    const char		*ip_addr,
				    struct bufferevent	*bev);

/* delete neighbour from neighbours */
void delete_neighbour(struct Neighbours		*neighbours,
		      struct bufferevent	*bev);

/* delete all neighbours */
void clear_neighbours(struct Neighbours *neighbours);

#endif
