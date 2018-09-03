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
 * Apply a function to all nodes of a linked list.
 *
 * @param	root		Root of the linked list.
 * @param	data_func	Apply this function to nodes' data. This
 *				function is called before node_func and
 *				can be NULL.
 * @param	node_func	Apply this function on the nodes. It can be
 *				NULL as well.
 */
void linkedlist_apply(linkedlist_t *root,
		      void	   (*data_func) (void *data),
		      void	   (*node_func) (linkedlist_node_t *node))
{
	linkedlist_apply_if(root, NULL, NULL, data_func, node_func);
}

/**
 * Apply a function to those linked list nodes and their data, that satisfy
 * a predicate. This function degrades into linkedlist_apply() if the
 * predicate is NULL.
 *
 * @param	root		Root of the linked list.
 * @param	attribute	Predicate's attribute.
 * @param	pred		Choose nodes based on result of this function.
 *				The predicate succeeds with non-zero result.
 * @param	data_func	Apply this function to selected nodes' data.
 *				This function is called before node_func and
 *				can be NULL.
 * @param	node_func	Apply this function on selected nodes. This
 *				can also be set to NULL.
 */
void linkedlist_apply_if(linkedlist_t	*root,
			 void		*attribute,
			 int		(*pred) (void *node_data,
						void *attribute),
			 void		(*data_func) (void *data),
			 void		(*node_func) (linkedlist_node_t *node))
{
	linkedlist_node_t	*current_node;
	linkedlist_node_t	*next_node;
	void			*node_data;

	current_node = linkedlist_get_first(root);
	while (current_node != NULL) {
		node_data = current_node->data;

		if (pred == NULL || pred(node_data, attribute)) {
			if (data_func != NULL) {
				data_func(current_node->data);
			}
			next_node = linkedlist_get_next(root, current_node);
			if (node_func != NULL) {
				node_func(current_node);
			}
		}

		current_node = next_node;
	}
}

/**
 * Delete a node from a linked list including its data.
 *
 * @param	node		The node to be deleted from the linked list.
 */
void linkedlist_delete(linkedlist_node_t *node)
{
	linkedlist_delete_safely(node, NULL);
}

/**
 * Delete a node from a linked list including its data and its content.
 *
 * @param	node		The node to be deleted from the linked list.
 * @param	clear_data	A clean up function to be applied to node's
 *				data. If there are no dynamically allocated
 *				data inside of the node, fill with NULL or
 *				use linkedlist_delete() instead.
 */
void linkedlist_delete_safely(linkedlist_node_t *node,
			      void		(*clear_data) (void *data))
{
	/* fix the links of neighbour nodes */
	node->prev->next = node->next;
	node->next->prev = node->prev;

	/* node's data deletion part */
	if (node->data != NULL) {
		if (clear_data != NULL) {
			clear_data(node->data);
		}
		free(node->data);
	}

	/* node deletion part */
	free(node);
}

/**
 * Destroys a linked list including data and their content.
 *
 * @param	root		Root of the linked list to destroy.
 * @param	clear_data	A clean up function to be applied to node's
 *				data. If there are no dynamically allocated
 *				data inside of the node, fill with NULL.
 */
void linkedlist_destroy(linkedlist_t	*root,
			void		(*clear_data) (void *data))
{
	linkedlist_node_t *tmp;

	tmp = root->first.next;
	while (tmp->next != NULL) {
		tmp = tmp->next;
		if (tmp->prev->data != NULL) {
			if (clear_data != NULL) {
				clear_data(tmp->prev->data);
			}
			free(tmp->prev->data);
		}
		free(tmp->prev);
	}
}

/**
 * Determine whether a linkedlist has no elements.
 *
 * @return	1	The linkedlist is empty.
 * @return	0	The linkedlist is not empty.
 */
int linkedlist_empty(const linkedlist_t *root)
{
	return linkedlist_get_first(root) == NULL;
}

/**
 * Find a node in a linked list by data.
 *
 * @param	root			Root of the linked list.
 * @param	data			Data of the requested node.
 *
 * @return	linkedlist_node_t	Node containing 'data'.
 * @return	NULL			If the node doesn't exist.
 */
linkedlist_node_t *linkedlist_find(const linkedlist_t	*root,
				   const void		*data)
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
		log_error("Inserting a new node");
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
 * Move a node from one linkedlist into another.
 *
 * @param	node		Move this node.
 * @param	dest		Into this linked list.
 */
void linkedlist_move(linkedlist_node_t *node, linkedlist_t *dest)
{
	linkedlist_node_t *dest_last;

	/* remove the node from its LL */
	linkedlist_remove(node);

	/* insert the node right before the last (stub) node of the dest LL */
	dest_last = dest->last.prev;

	node->prev = dest_last;
	node->next = dest_last->next;
	dest_last->next->prev = node;
	dest_last->next = node;
}

/**
 * Remove a node from a linked list. Keep the node's data intact.
 *
 * @param	node	The node to be removed from the linked list.
 */
void linkedlist_remove(linkedlist_node_t *node)
{
	/* fix the links of neighbour nodes */
	node->prev->next = node->next;
	node->next->prev = node->prev;

	free(node);
}

/**
 * Remove all nodes from a linked list. Keep node's data intact.
 *
 * @param	root	The root of the linked list.
 */
void linkedlist_remove_all(linkedlist_t *root)
{
	linkedlist_node_t *tmp;

	tmp = root->first.next;
	while (tmp->next != NULL) {
		tmp = tmp->next;
		free(tmp->prev);
	}

	linkedlist_init(root);
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
