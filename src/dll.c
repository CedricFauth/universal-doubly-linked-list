#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "dll.h"

typedef struct __dll_node_internal dll_node_t;

struct __dll_node_internal {
    dll_node_t *prev; // link to previous node
    dll_node_t *next; // link to next node

    void *data; // link to data
};

struct __dll_internal {
    dll_node_t end;
    size_t size;
};

/**
 * @brief prints an error and details; useful to print error messages
 *
 * @param location name of the caller function
 * @param msg description of the error
 */
static void error(char *location, char *msg) {
    fprintf(stderr, "Error [%s] : %s.", location, msg);
}

// see dll.h
dll_t *dll_new() {
    dll_t *list = malloc(sizeof(dll_t));
    if (!list) {
        error("dll_new", "Could not allocate enough memory");
        return NULL;
    }
    list->end.next = &list->end;
    list->end.prev = &list->end;
    list->size = 0;
    return list;
}

/**
 * @brief allocates new node containing data
 *
 * @param data user data
 * @return new node
 */
static dll_node_t *_dll_new_node(void *data) {
    dll_node_t *node = malloc(sizeof(dll_node_t));
    if (!node) {
        error("dll_new_node", "Could not allocate enough memory");
        return NULL;
    }
    node->prev = NULL;
    node->next = NULL;
    node->data = data;
    return node;
}

/**
 * @brief deletes a node and optionally its data
 * 
 * @param node node to delete
 * @param func user data delete function
 */
static void dll_delete_node(dll_node_t *node, delete_data_fun func) {
    if (!node) return;
    printf("fn ");
    if (func) (*func)(node->data);
    free(node);
}

// see dll.h
void dll_delete(dll_t *list, delete_data_fun func) {
    if (!list) return;
    dll_node_t *end = &list->end;
    dll_node_t *curr = end->next;
    dll_node_t *tmp;
    while (curr != end) {
        tmp = curr;
        curr = curr->next;
        dll_delete_node(tmp, func);
    }
    free(list);
}

// see dll.h
/**
 * @todo refactor
 */
void dll_display(dll_t *list, display_data_fun func) {
    if (!list) {
        printf("null\n");
        return;
    }
    dll_node_t *end = &list->end;
    dll_node_t *curr = end->next;
    if (curr == end) {
        printf("empty1\n");
        return;
    }
    while (1) {
        printf("[");
        if (func) (*func)(curr->data);
        printf("]");
        if ((curr = curr->next) != end) printf("<=>");
        else break;
    }
    printf(" rev: ");
    curr = end->prev;
    if (curr == end) {
        printf("empty2\n");
        return;
    }
    while (1) {
        printf("[");
        if (func) (*func)(curr->data);
        printf("]");
        if ((curr = curr->prev) != end) printf("<=>");
        else break;
    }
    
    printf("\n");
}

/**
 * @brief internal function that inserts data (counts from begin)
 * pos=0 : frist element
 */
static void _dll_insert_from_begin(dll_t *list, int pos, void *data) {
    dll_node_t *new_node = _dll_new_node(data);
    dll_node_t *end = &list->end;
    dll_node_t **insert_pointer = &list->end.next;
    while (*insert_pointer != end && pos) {
        insert_pointer = &(*insert_pointer)->next;
        --pos;
    }
    new_node->next = (*insert_pointer);
    new_node->prev = (*insert_pointer)->prev;
    (*insert_pointer)->prev = new_node;
    *insert_pointer = new_node;
}

/**
 * @brief internal function that inserts data (counts from end)
 * pos=0 : last element
 */
static void _dll_insert_from_end(dll_t *list, int pos, void *data) {
    dll_node_t *new_node = _dll_new_node(data);
    dll_node_t *end = &list->end;
    dll_node_t **insert_pointer = &list->end.prev;
    while (*insert_pointer != end && pos) {
        insert_pointer = &(*insert_pointer)->prev;
        --pos;
    }
    new_node->prev = (*insert_pointer);
    new_node->next = (*insert_pointer)->next;
    (*insert_pointer)->next = new_node;
    *insert_pointer = new_node;
}

// see dll.h
void dll_insert(dll_t *list, int pos, void *data) {
    if(!list) {
        error("dll_insert", "list is null");
        return;
    }
    if (pos < 0) {
        _dll_insert_from_end(list, -pos-1, data);
    } else {
        _dll_insert_from_begin(list, pos, data);
    }
    list->size++;
}

// see dll.h
void dll_push_front(dll_t *list, void *data) {
    if(!list) {
        error("dll_push_front", "list is null");
        return;
    }
    _dll_insert_from_begin(list, 0, data);
}

// see dll.h
void dll_push_back(dll_t *list, void *data) {
    if(!list) {
        error("dll_push_back", "list is null");
        return;
    }
    _dll_insert_from_end(list, 0, data);
}
