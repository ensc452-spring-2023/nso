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

void ll_append(Node_t **tail, size_t dataSize) {
	Node_t *newNode = (Node_t *) malloc(sizeof(Node_t));
	if (newNode == NULL) {
		xil_printf("ERROR: could not malloc memory for linked list\r\n");
		return NULL;
	}

	newNode->data = malloc(dataSize);
	if (newNode->data == NULL) {
		xil_printf("ERROR: could not malloc memory for linked list data\r\n");
		return NULL;
	}

	newNode->prev = *tail;
	newNode->next = NULL;

	if (*tail != NULL)
		(*tail)->next = newNode;

	*tail = newNode;
}

Node_t * ll_append_object(Node_t ** head, void* data) {
	Node_t *newNode = (Node_t *) malloc(sizeof(Node_t));
	if (newNode == NULL) {
		xil_printf("ERROR: could not malloc memory for linked list\r\n");
		return NULL;
	}

	newNode->data = data;
	newNode->next = NULL;

	// if first insertion
	if (*head == NULL) {
		*head = newNode;
		newNode->prev = NULL;
		return newNode;
	}

	// other insertions
	Node_t * current = *head;
	while (TRUE) {
		if (current->next == NULL) {
			current->next = newNode;
			newNode->prev = current;
			break;
		}
		current = current->next;
	}

	return newNode;
}

Node_t *ll_deleteNode(Node_t *currNode) {
	if (currNode == NULL)
		return NULL;

	Node_t *prevNode = currNode->prev;
	Node_t *nextNode = currNode->next;

	if (prevNode != NULL)
		prevNode->next = nextNode;

	if (nextNode != NULL)
		nextNode->prev = prevNode;

	free(currNode->data);
	free(currNode);

	return nextNode;
}

Node_t *ll_deleteNodeHead(Node_t ** head, Node_t *currNode) {
	if (currNode == NULL)
		return NULL;

	Node_t *prevNode = currNode->prev;
	Node_t *nextNode = currNode->next;

	if (prevNode != NULL)
		prevNode->next = nextNode;

	if (nextNode != NULL){
		nextNode->prev = prevNode;
		if(prevNode == NULL){
			*head = nextNode;
		}
	}

	free(currNode->data);
	free(currNode);

	return nextNode;
}

void ll_deleteList(Node_t *head) {
	Node_t *currNode = head;

	while (currNode != NULL) {
		Node_t *nextNode = head->next;

		free(currNode->data);
		free(currNode);

		currNode = nextNode;
	}
}
