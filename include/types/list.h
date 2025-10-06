#ifndef LIST_H
#define LIST_H

#include "types/bool.h"

typedef struct list_node list_node_t;
typedef struct list list_t;

typedef struct list {
    list_node_t *head;
    list_node_t *tail;
    int size;
    bool circular; // If true, the list is circular
} list_t;

typedef struct list_node {
    list_node_t *next;
    list_node_t *prev;
    list_t *list;
} list_node_t;

#define LIST_INIT { .head = NULL, .tail = NULL, .size = 0, .circular = false }

#define list_for_each(pos, list) \
    for (pos = (list)->head; pos != NULL && (list)->size != 0; pos = pos->next)

void list_init(list_t *list, bool circular);
void list_push_head(list_t *list, list_node_t *node);
void list_push_tail(list_t *list, list_node_t *node);
list_node_t *list_pop_head(list_t *list);
list_node_t *list_pop_tail(list_t *list);
void list_remove(list_node_t *node);
list_node_t* list_find(list_t *list, void *data);
int list_size(list_t *list);

#endif // LIST_H