#include <stdlib.h>
#include "list.h"

list_t *init_list(void (*destroy)(void *data))
{
	list_t *l = malloc(sizeof(list_t));
	if (!l) {
		return NULL;
	}
	l->list_size = 0;
	l->destroy = destroy;
	l->head = NULL;
	l->tail = NULL;
	return l;
}

static struct node_t *init_list_data(void *data)
{
	struct node_t *node = malloc(sizeof(struct node_t));
	node->data = data;
	node->prev = NULL;
	node->next = NULL;
	return node;
}

static struct node_t *list_insert_after(list_t *l, struct node_t *p, void *data)
{
	struct node_t *node = init_list_data(data);

	if (LIST_EMPTY(l)) {
		l->head = node;
		l->tail = node;
	} else if (p == l->tail) {
		node->prev = p;
		p->next = node;
		l->tail = node;
	} else {
		node->prev = p;
		node->next = p->next;
		p->next->prev = node;
		p->next = node;
	}

	l->list_size++;
	return node;
}

struct node_t *list_insert_data(list_t *l, void *data)
{
	return list_insert_after(l, l->tail, data);
}

static void remove_data(list_t *l, struct node_t *p)
{
	l->list_size--;
	l->destroy(p->data);
	free(p);
}

void list_destroy(list_t *l)
{
        struct node_t *node = l->head;

        while (node != NULL) {
                struct node_t *temp = node->next;
                remove_data(l, node);
		node = temp;
        }
        free(l);
}

static void list_remove_head(list_t *l)
{
	if (l->head != NULL) {
		struct node_t *temp = l->head;
		if (l->head->next == NULL) {
			l->head = NULL;
			l->tail = NULL;
		} else {
			l->head = l->head->next;
			l->head->prev = NULL;
		}
		remove_data(l, temp);
	}
}

static void list_remove_tail(list_t *l)
{
	if (l->tail != NULL) {
		struct node_t *temp = l->tail;
		if (l->tail->prev == NULL) {
			l->head = NULL;
			l->tail = NULL;
		} else {
			l->tail = l->tail->prev;
			l->tail->next = NULL;
		}
		remove_data(l, temp);
	}
}

void list_remove_data(list_t *l, struct node_t *p)
{
	if (l->head == p) {
		list_remove_head(l);
		return;
	} else if (l->tail == p) {
		list_remove_tail(l);
		return;
	} else if (p->prev != NULL && p->next != NULL) {
                p->prev->next = p->next;
                p->next->prev = p->prev;
		remove_data(l, p);
	}
}
