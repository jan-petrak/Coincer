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

#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdint.h>

/**
 * Node of the linked list structure.
 */
typedef struct s_linkedlist_node {
	struct s_linkedlist_node	*prev; /**< Previous node. */
	struct s_linkedlist_node	*next; /**< Next node. */
	void				*data; /**< Data of the node. */
} linkedlist_node_t;

/**
 * Linked list root structure.
 */
typedef struct s_linkedlist {
	struct s_linkedlist_node	first; /**< Auxiliary first node. */
	struct s_linkedlist_node	last; /**< Auxiliary last node. */
} linkedlist_t;

linkedlist_node_t *linkedlist_append(linkedlist_t *root, void *data);

void linkedlist_apply(linkedlist_t *root,
		      void	  (*data_func) (void *data),
		      void	  (*node_func) (linkedlist_node_t *node));

void linkedlist_apply_if(linkedlist_t	*root,
			 void		*attribute,
			 int	       (*pred) (void *node_data,
						void *attribute),
			 void	       (*data_func) (void *data),
			 void	       (*node_func) (linkedlist_node_t *node));

void linkedlist_delete(linkedlist_node_t *node);

void linkedlist_delete_safely(linkedlist_node_t *node,
			      void	       (*clear_data) (void *data));

void linkedlist_destroy(linkedlist_t	*root,
			void	       (*clear_data) (void *data));

int linkedlist_empty(const linkedlist_t *root);

linkedlist_node_t *linkedlist_find(const linkedlist_t *root, const void *data);

linkedlist_node_t *linkedlist_get_first(const linkedlist_t *root);

linkedlist_node_t *linkedlist_get_last(const linkedlist_t *root);

linkedlist_node_t *linkedlist_get_next(const linkedlist_t	*root,
				       const linkedlist_node_t	*node);

linkedlist_node_t *linkedlist_get_prev(const linkedlist_t	*root,
				       const linkedlist_node_t	*node);

void linkedlist_init(linkedlist_t *root);

linkedlist_node_t *linkedlist_insert_after(linkedlist_t		*root,
					   linkedlist_node_t	*node,
					   void			*data);

linkedlist_node_t *linkedlist_insert_before(linkedlist_t	*root,
					    linkedlist_node_t	*node,
					    void		*data);

void linkedlist_move(linkedlist_node_t *node, linkedlist_t *dest);

void linkedlist_remove(linkedlist_node_t *node);

void linkedlist_remove_all(linkedlist_t *root);

size_t linkedlist_size(const linkedlist_t *root);

#endif /* LINKEDLIST_H */
