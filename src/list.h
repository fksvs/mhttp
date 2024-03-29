#ifndef MHTTP_LIST_H
#define MHTTP_LIST_H

#define LIST_EMPTY(list) ((list)->head == NULL || (list)->tail == NULL)
#define LIST_HEAD(list) ((list)->head)
#define LIST_TAIL(list) ((list)->tail)
#define LIST_SIZE(list) ((list)->list_size)

struct node_t {
	void *data;
	struct node_t *prev;
	struct node_t *next;
};

typedef struct {
	unsigned int list_size;
	void (*destroy)(void *data);
	struct node_t *head;
	struct node_t *tail;
} list_t;

list_t *init_list(void (*destroy)(void *data));
struct node_t *list_insert_data(list_t *l, void *data);
void list_destroy(list_t *l);
void list_remove_data(list_t *l, struct node_t *p);

#endif
