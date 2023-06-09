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
// to the new node
void ll_append(Node_t **tail, size_t dataSize);
// Deletes current node from the list and returns next node
Node_t *ll_deleteNode(Node_t *currNode);
// Deletes the whole linked list
void ll_deleteList(Node_t *head);
//appends already created data to the end of the linked list
Node_t * ll_append_object(Node_t ** head, void * data);
//fixed deleteding the head node
Node_t *ll_deleteNodeHead(Node_t ** head, Node_t *currNode);

#endif
