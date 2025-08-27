#ifndef LIST_H
#define LIST_H

typedef struct list_node list_node_t;
typedef struct list list_t;

typedef struct list {
    list_node_t *head;
    list_node_t *tail;
    int size;
} list_t;

typedef struct list_node {
    list_node_t *next;
    list_node_t *prev;
    list_t *list;
    int priority;
    void *data;
} list_node_t;

#define LIST_INIT { 0, 0 }

void list_push_head(list_t *list, list_node_t *node);
void list_push_tail(list_t *list, list_node_t *node);
void list_push_priority(list_t *list, list_node_t *node, int priority);
list_node_t *list_pop_head(list_t *list);
list_node_t *list_pop_tail(list_t *list);
void list_remove(list_node_t *node);
int list_size(list_t *list);

#endif // LIST_H