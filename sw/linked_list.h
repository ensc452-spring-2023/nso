/*-----------------------------------------------------------------------------/
 /	!nso - Generic Doubly Linked List										   /
 /-----------------------------------------------------------------------------/
 /	William Zhang
 /	Bowie Gian
 /	04/03/2023
 /	linked_list.h
 /
 /	Generic doubly linked list data structure
 /----------------------------------------------------------------------------*/

#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdlib.h>

typedef struct Node {
	void *data;
	struct Node *prev;
	struct Node *next;
} Node_t;

// Adds an empty new node to the end of the list, moving the tail pointer
// Returns the created node
Node_t *ll_append(Node_t **tail, size_t dataSize);
// Deletes the whole linked list
void ll_delete(Node_t *head);

#endif
