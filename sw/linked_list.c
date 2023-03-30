/*-----------------------------------------------------------------------------/
 /	!nso - Generic Doubly Linked List										   /
 /-----------------------------------------------------------------------------/
 /	William Zhang
 /	Bowie Gian
 /	04/03/2023
 /	linked_list.c
 /
 /	Generic doubly linked list data structure
 /----------------------------------------------------------------------------*/

#include "xil_printf.h"
#include "linked_list.h"

Node_t *ll_append(Node_t **tail, size_t dataSize)
{
	Node_t *newNode = (Node_t *)malloc(sizeof(Node_t));
	if (newNode == NULL) {
		xil_printf("ERROR: could not malloc memory for linked list\r\n");
		while(1) {}
	}

	newNode->data = malloc(dataSize);
	if (newNode->data == NULL) {
		xil_printf("ERROR: could not malloc memory for linked list data\r\n");
		while (1) {}
	}

	newNode->prev = *tail;
	newNode->next = NULL;

	if (*tail != NULL)
		(*tail)->next = newNode;

	*tail = newNode;
	return newNode;
}

void ll_delete(Node_t *head)
{
	Node_t *currNode = head;

	while (currNode != NULL) {
		Node_t *nextNode = head->next;

		free(currNode->data);
		free(currNode);

		currNode = nextNode;
	}
}
