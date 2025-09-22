#include "types/list.h"
#include "types/string.h"

void list_push_head(list_t *list, list_node_t *node) {
    list->size++;
    node->list = list;
    if (list->head == NULL) {
        list->head = node;
        list->tail = node;
        node->next = node->prev = list->circular ? node : NULL;
        return;
    }

    node->next = list->head;
    node->prev = list->circular ? list->tail : NULL;
    list->head->prev = node;
    list->head = node;
    if (list->circular) {
        list->tail->next = list->head;
        list->head->prev = list->tail;
    } else {
        list->tail->next = NULL;
        list->head->prev = NULL;
    }
}

void list_push_tail(list_t *list, list_node_t *node) {
    list->size++;
    node->list = list;
    if (list->head == NULL) {
        list->head = node;
        list->tail = node;
        node->next = node->prev = list->circular ? node : NULL;
        return;
    }

    node->prev = list->tail;
    node->next = list->circular ? list->head : NULL;
    list->tail->next = node;
    list->tail = node;
    if (list->circular) {
        list->tail->next = list->head;
        list->head->prev = list->tail;
    } else {
        list->tail->next = NULL;
        list->head->prev = NULL;
    }
}

list_node_t *list_pop_head(list_t *list) {
    list_node_t *node = list->head;
    if (!node) return NULL;
    list->head = node->next;

    if (list->head == node) {
        list->head = list->tail = NULL;
        goto node_cleanup;
    }

    if (list->head)
        list->head->prev = list->circular ? list->tail : NULL;
    else
        list->tail = NULL;

    node_cleanup:
    node->next = NULL;
    node->prev = NULL;
    node->list = NULL;
    list->size--;
    return node;
}

list_node_t *list_pop_tail(list_t *list) {
    list_node_t *node = list->tail;
    if (!node) return NULL;
    list->tail = node->prev;

    if (list->tail == node) {
        list->head = list->tail = NULL;
        goto node_cleanup;
    }

    if (list->tail)
        list->tail->next = list->circular ? list->head : NULL;
    else
        list->head = NULL;

    node_cleanup:
    node->next = NULL;
    node->prev = NULL;
    node->list = NULL;
    list->size--;
    return node;
}

void list_remove(list_node_t *node) {
    if (node->prev == node || (node->next == NULL && node->prev == NULL)) {
        node->list->head = node->list->tail = NULL;
        goto node_cleanup;
    }

    if (node->prev)
        node->prev->next = node->next;

    if (node->next)
        node->next->prev = node->prev;

    if (node == node->list->head)
        node->list->head = node->next;

    if (node == node->list->tail)
        node->list->tail = node->prev;

    node_cleanup:
    node->list->size--;
    node->next = NULL;
    node->prev = NULL;
    node->list = NULL;
}

list_node_t *list_find(list_t *list, void *addr) {
    list_node_t *current = list->head;
    if (!current) return NULL;
    do {
        if (current == addr) {
            return current;
        }
        current = current->next;
    } while (current != NULL && current != list->head);
    return NULL;
}

int list_size(list_t *list) {
    return list->size;
}
