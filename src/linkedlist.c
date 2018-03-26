/*
 *  Coincer
 *  Copyright (C) 2017  Coincer Developers
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

#include <stdio.h>
#include <stdlib.h>

#include "linkedlist.h"

/**
 * Initializes linked list to default values.
 *
 * @param	root	Linked list to initialize.
 */
void linkedlist_init(linkedlist_t *root)
{
	root->first.prev = NULL;
	root->first.next = &root->last;
	root->last.prev  = &root->first;
	root->last.next  = NULL;

	root->first.data	= NULL;
	root->last.data		= NULL;
}

/**
 * Get first node of the linked list.
 *
 * @param	root	Root of the linked list.
 *
 * @return	First node of the linked list.
 * @return	NULL if linkedlist is empty.
 */
linkedlist_node_t *linkedlist_get_first(const linkedlist_t *root)
{
	if (root->first.next == &root->last) {
		return NULL;
	}

	return root->first.next;
}

/**
 * Get last node of the linkedlist.
 *
 * @param	root	Root of the linked list.
 *
 * @return	Last node of the linked list.
 * @return	NULL if linkedlist is empty.
 */
linkedlist_node_t *linkedlist_get_last(const linkedlist_t *root)
{
	if (root->last.prev == &root->first) {
		return NULL;
	}

	return root->last.prev;
}

/**
 * Get node that is right after 'node' in the linked list.
 *
 * @param	root	Root of the linked list.
 * @param	node	We want node that is right after this node.
 *
 * @return	The node we requested.
 * @return	NULL if 'node' was last in the linked list or NULL.
 */
linkedlist_node_t *linkedlist_get_next(const linkedlist_t	*root,
				       const linkedlist_node_t	*node)
{
	if (node == NULL || node->next == &root->last) {
		return NULL;
	}

	return node->next;
}

/**
 * Destroys a linked list.
 *
 * @param	root	Root of the linked list to destroy.
 */
void linkedlist_destroy(linkedlist_t *root)
{
	linkedlist_node_t *tmp;

	tmp = root->first.next;
	while (tmp->next != NULL) {
		tmp = tmp->next;
		free(tmp->prev);
	}
}

/**
 * Append new node at the end of the linked list.
 *
 * @param	root	Root of the linked list.
 * @param	data	Data to append to the list.
 *
 * @return	Pointer to created node if succeeded.
 * @return	NULL if failed.
 */
linkedlist_node_t *linkedlist_append(linkedlist_t *root, void *data)
{
	linkedlist_node_t *node;

	if ((node = (linkedlist_node_t *) malloc(sizeof(linkedlist_node_t))) ==
	    NULL) {
		perror("linkedlist_append malloc");
		return NULL;
	}

	node->data = data;

	node->prev = root->last.prev;
	node->next = &root->last;
	root->last.prev->next = node;
	root->last.prev = node;

	return node;
}

/**
 * Delete node from the linked list.
 *
 * @param	root	Root of the linked list.
 * @param	node	Node to be deleted from the linked list.
 */
void linkedlist_delete(linkedlist_t *root, linkedlist_node_t *node)
{
	/* fix the links of neighbour nodes */
	node->prev->next = node->next;
	node->next->prev = node->prev;

	/* node deletion part */
	free(node);
}

/**
 * Find node in the linked list by data.
 *
 * @param	root	Root of the linked list.
 * @param	data	Data of the requested node.
 *
 * @return	Node containing 'data'.
 * @return	NULL if the node doesn't exist.
 */
linkedlist_node_t *linkedlist_find(const linkedlist_t *root, const void *data)
{
	/* start the search from the first node of the linked list */
	linkedlist_node_t *current = linkedlist_get_first(root);

	while (current != NULL) {
		/* node found */
		if (current->data == data) {
			/* return the node containing 'data' */
			return current;
		}

		/* move to the next node and continue with the search */
		current = linkedlist_get_next(root, current);
	}

	/* node not found */
	return NULL;
}
