#include "node.h"


void add(struct node *head, int fd, struct sockaddr_in addr) {
	struct node *new_node;

	new_node = (struct node *)malloc(sizeof(struct node));
	new_node->message = NULL;
	new_node->fd = fd;
	new_node->client_addr = addr;
	new_node->operation_type = IS_RECEIVING;
	new_node->processed_message = 0;
	new_node->pending_message = 0;

	new_node->next = head->next;

	head->next = new_node;
}

void dump(struct node *head, int fd) {
	struct node *current, *temp;

	current = head;
	while (current->next) {
		if (current->next->fd == fd) {
			temp = current->next;
			current->next = temp->next;

			if (temp->message != NULL) {
				free(temp->message);
				temp->message = NULL;
			}

			free(temp);

			return;
		} else {
			current = current->next;
		}
	}
}
