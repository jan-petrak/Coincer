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

#include <stdio.h>
#include <stdlib.h>

#include "linkedlist.h"
#include "log.h"

/**
 * Append new node at the end of the linked list.
 *
 * @param	root			Root of the linked list.
 * @param	data			Data of the new node.
 *
 * @return	linkedlist_node_t	Pointer to created node if succeeded.
 * @return	NULL			If failure.
 */
linkedlist_node_t *linkedlist_append(linkedlist_t *root, void *data)
{
	return linkedlist_insert_after(root,
				       root->last.prev,
				       data);
}

/**
 * Delete node from the linked list.
 *
 * @param	node	Node to be deleted from the linked list.
 */
void linkedlist_delete(linkedlist_node_t *node)
{
	/* fix the links of neighbour nodes */
	node->prev->next = node->next;
	node->next->prev = node->prev;

	/* node's data deletion part */
	if (node->data != NULL) {
		free(node->data);
	}

	/* node deletion part */
	free(node);
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
		if (tmp->prev->data != NULL) {
			free(tmp->prev->data);
		}
		free(tmp->prev);
	}
}

/**
 * Find node in the linked list by data.
 *
 * @param	root			Root of the linked list.
 * @param	data			Data of the requested node.
 *
 * @return	linkedlist_node_t	Node containing 'data'.
 * @return	NULL			If the node doesn't exist.
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

/**
 * Get the first node of the linked list.
 *
 * @param	root			Root of the linked list.
 *
 * @return	linkedlist_node_t	First node of the linked list.
 * @return	NULL 			If the linkedlist is empty.
 */
linkedlist_node_t *linkedlist_get_first(const linkedlist_t *root)
{
	if (root->first.next == &root->last) {
		return NULL;
	}

	return root->first.next;
}

/**
 * Get the last node of the linkedlist.
 *
 * @param	root			Root of the linked list.
 *
 * @return	linkedlist_node_t	Last node of the linked list.
 * @return	NULL			If linkedlist is empty.
 */
linkedlist_node_t *linkedlist_get_last(const linkedlist_t *root)
{
	if (root->last.prev == &root->first) {
		return NULL;
	}

	return root->last.prev;
}

/**
 * Get a node that is right after 'node' in the linked list.
 *
 * @param	root			Root of the linked list.
 * @param	node			A node right after this node.
 *
 * @return	linkedlist_node_t	The node we requested.
 * @return	NULL			If 'node' was the last or 'root'
 *					is empty.
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
 * Get a node that is right before 'node' in the linked list.
 *
 * @param	root			Root of the linked list.
 * @param	node			A node right before this node.
 *
 * @return	linkedlist_node		The node we requested.
 * @return	NULL 			If 'node' was the first or 'root'
 *					is empty.
 */
linkedlist_node_t *linkedlist_get_prev(const linkedlist_t	*root,
				       const linkedlist_node_t	*node)
{
	if (node == NULL || node->prev == &root->first) {
		return NULL;
	}

	return node->prev;
}

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

	root->first.data = NULL;
	root->last.data	 = NULL;
}

/**
 * Insert new node into linkedlist after 'node'. In case of 'node' being the
 * last (stub) node, insert before it.
 *
 * @param	root			Root of the linked list.
 * @param	node			Insert new node right after this node.
 * @param	data			Data of the new node.
 *
 * @return	linkedlist_node_t	Pointer to the new node.
 * @return	NULL			If failure.
 */
linkedlist_node_t *linkedlist_insert_after(linkedlist_t		*root,
					   linkedlist_node_t	*node,
					   void			*data)
{
	linkedlist_node_t *new_node;

	if (node == &root->last) {
		node = root->last.prev;
	}

	new_node = (linkedlist_node_t *) malloc(sizeof(linkedlist_node_t));
	if (new_node == NULL) {
		log_error("linkedlist_insert_after - malloc");
		return NULL;
	}

	new_node->data = data;

	new_node->prev = node;
	new_node->next = node->next;
	node->next->prev = new_node;
	node->next = new_node;

	return new_node;
}

/**
 * Insert new node into linkedlist before 'node'. In case of 'node' being
 * the first (stub) node, insert after it.
 *
 * @param	root			Root of the linked list.
 * @param	node			Insert new node right before this node.
 * @param	data			Data of the new node.
 *
 * @return	linkedlist_node_t	Pointer to the new node.
 * @return	NULL			If failure.
 */
linkedlist_node_t *linkedlist_insert_before(linkedlist_t	*root,
					    linkedlist_node_t	*node,
					    void		*data)
{
	if (node == &root->first) {
		node = root->first.next;
	}
	return linkedlist_insert_after(root, node->prev, data);
}

/**
 * Get the number of elements in the linked list.
 *
 * @param	root	Root of the linked list.
 *
 * @return	n	Number of linked list elements.
 */
size_t linkedlist_size(const linkedlist_t *root)
{
	linkedlist_node_t	*tmp;
	size_t			n = 0;

	tmp = root->first.next;
	while (tmp->next != NULL) {
		n++;
		tmp = tmp->next;
	}

	return n;
}
