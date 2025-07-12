#include "types/list.h"

void list_push_head(list_t *list, list_node_t *node) {
    node->next = list->head;
    node->prev = 0;
    node->priority = 0;
    node->list = list;
    if (list->head) {
        list->head->prev = node;
    }
    list->head = node;
    if (!list->tail) {
        list->tail = node;
    }
    list->size++;
}

void list_push_tail(list_t *list, list_node_t *node) {
    node->next = 0;
    node->prev = list->tail;
    node->priority = 0;
    node->list = list;
    if (list->tail) {
        list->tail->next = node;
    }
    list->tail = node;
    if (!list->head) {
        list->head = node;
    }
    list->size++;
}

void list_push_priority(list_t *list, list_node_t *node, int priority) {
    node->next = 0;
    node->prev = 0;
    node->priority = priority;
    node->list = list;
    list_node_t *current = list->head;
    while (current && current->priority >= priority) {
        current = current->next;
    }
    if (current) {
        node->next = current;
        node->prev = current->prev;
        if (current->prev) {
            current->prev->next = node;
        } else {
            list->head = node;
        }
        current->prev = node;
    } else {
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    }
    list->size++;
}

list_node_t *list_pop_head(list_t *list) {
    list_node_t *node = list->head;
    if (node) {
        list->head = node->next;
        if (list->head) {
            list->head->prev = 0;
        } else {
            list->tail = 0;
        }
        node->next = 0;
        node->prev = 0;
        node->list = 0;
        list->size--;
    }
    return node;
}

list_node_t *list_pop_tail(list_t *list) {
    list_node_t *node = list->tail;
    if (node) {
        list->tail = node->prev;
        if (list->tail) {
            list->tail->next = 0;
        } else {
            list->head = 0;
        }
        node->next = 0;
        node->prev = 0;
        node->list = 0;
        list->size--;
    }
    return node;
}

void list_remove(list_node_t *node) {
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        node->list->head = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    } else {
        node->list->tail = node->prev;
    }
    node->next = 0;
    node->prev = 0;
    node->list = 0;
    node->priority = 0;
    node->list->size--;
}

int list_size(list_t *list) {
    return list->size;
}
