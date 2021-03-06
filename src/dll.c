#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <memory.h>
#include <stdint.h>
#include "dll.h"

typedef struct _dll_node_internal dll_node_t;

struct _dll_node_internal {
    dll_node_t *prev; // points to previous node
    dll_node_t *next; // points to next node

    unsigned char data[]; // contains data
};

struct _dll_internal {
    ssize_t size; // number of elements

    dll_node_t *end; // points to initial element
    size_t data_size; // stores size of data in bytes
    op_mode op_mode;
};

struct _dll_iterator {
    dll_t *list;
    dll_node_t *curr;
};

/**
 * @brief prints an error and details; useful to print error messages
 *
 * @param location name of the caller function
 * @param msg description of the error
 */
static void error(char *location, char *msg) {
    fprintf(stderr, "Error [%s] : %s.\n", location, msg);
}

// see dll.h
dll_t *dll_new(op_mode mode, size_t data_size) {
    if (mode == VALUE && data_size <= 0) {
        error("dll_new", "data_size needs to be larger than 0 in VALUE mode");
        return NULL;
    }
    dll_t *list = malloc(sizeof(*list));
    if (!list) {
        error("dll_new", "Could not allocate enough memory");
        return NULL;
    }
    list->end = malloc(sizeof(*list->end));
    if (!list->end) {
        error("dll_new", "Could not allocate enough memory");
        free(list);
        return NULL;
    }
    list->end->next = list->end;
    list->end->prev = list->end;

    list->size = 0;
    list->op_mode = mode;
    if (mode == REFERENCE) {
        list->data_size = sizeof(void *);
    } else { // VALUE
        list->data_size = data_size;
    }
    return list;
}

dll_t *dll_from_value_array(void *array, int len, op_mode mode, size_t elem_size) {
    if (!array || !len) {
        error("dll_from_array", "array not valid");
    }
    dll_t *list = dll_new(mode, elem_size);
    unsigned char *base = (unsigned char*)array;
    for (int i = 0; i < len; ++i)
        dll_push_back(list, base + i * elem_size);
    return list;
}

/**
 * @brief internal function; allocates new node and copying data
 * 
 * @param data_size size of the data in bytes
 * @param data data to insert (copy)
 * @return (dll_node_t *) node pointer
 */
static dll_node_t *_dll_new_node(op_mode mode, size_t data_size, void *data) {
    dll_node_t *node = malloc(sizeof(*node) + data_size);
    if (!node) {
        error("_dll_new_node", "Could not allocate memory");
        return NULL;
    }
    node->prev = NULL;
    node->next = NULL;
    if (mode == REFERENCE) {
        //memcpy(node->data, &data, data_size);
        *(void **)node->data = data;
        //*(void **)node->data = data;
    } else { // VALUE
        memcpy(node->data, data, data_size);
    }
    
    return node;
}

/**
 * @brief deletes a node and optionally its data
 * 
 * @param node node to delete
 * @param func user data delete function
 */
static void _dll_delete_node(op_mode m, dll_node_t *node, delete_data_fun func) {
    if (!node) {
        error("_dll_delete_node", "node is null");
        return;
    }
    printf("fn ");
    if (func) {
        if (m == REFERENCE) {
            (*func)(*(void **)node->data);
        } else { // VALUE
            (*func)(node->data);
        }
    }
    free(node);
}

/**
 * @brief gets called when a node gets removed from the list and returns data
 * 
 * @param m 
 * @param node 
 * @return void* 
 */
static void *_dll_remove_node(dll_t *list, dll_node_t *node, void *dest) {
    if (!list || !node) {
        error("_dll_remove_node", "node is null");
        return NULL;
    }
    op_mode m = list->op_mode;
    if (m == REFERENCE) {
        void *ref = *(void **)node->data;
        _dll_delete_node(m, node, NULL); // TODO remove != delete fun
        return ref;
    } else {
        if (dest) {
            memcpy(dest, node->data, list->data_size);
        }
        _dll_delete_node(m, node, NULL); // TODO remove != delete fun
        return dest;
    }
}

// see dll.h
void dll_delete(dll_t *list, delete_data_fun func) {
    if (!list) return;
    dll_node_t *end = list->end;
    dll_node_t *curr = end->next;
    dll_node_t *tmp;
    while (curr != end) {
        tmp = curr;
        curr = curr->next;
        _dll_delete_node(list->op_mode, tmp, func);
    }
    free(end);
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
    dll_node_t *end = list->end;
    dll_node_t *curr = end->next;
    op_mode mode = list->op_mode;
    if (curr == end) {
        printf("empty1\n");
        return;
    }
    while (1) {
        printf("[");
        if (func) {
            if (mode == REFERENCE) {
                (*func)(*(void **)curr->data);
            } else { // VALUE
                (*func)(curr->data);
            }
        }
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
        if (func) {
            if (mode == REFERENCE) {
                (*func)(*(void **)curr->data);
            } else { // VALUE
                (*func)(curr->data);
            }
        }
        printf("]");
        if ((curr = curr->prev) != end) printf("<=>");
        else break;
    }
    
    printf("\n");
}

static void _dll_insert_from_begin(dll_t *list, int pos, void *data) {
    if(!list) {
        error("dll_insert", "list is null");
        return;
    }
    if(pos > list->size || pos < 0) {
        error("dll_insert", "index out of range");
        return;
    }
    dll_node_t *new_node = _dll_new_node(list->op_mode, list->data_size, data);
    if (!new_node) return;
    dll_node_t *node = list->end->next;
    while (pos) {
        node = node->next;
        --pos;
    }
    new_node->prev = node->prev;
    new_node->next = node;
    node->prev->next = new_node;
    node->prev = new_node;
    list->size++;
}

/**
 * @brief internal function that inserts data (counts from end)
 * pos=0 : last element
 */
static void _dll_insert_from_end(dll_t *list, int pos, void *data) {
    if(!list) {
        error("dll_insert", "list is null");
        return;
    }
    if(pos > list->size || pos < 0) {
        error("dll_insert", "index out of range");
        return;
    }
    dll_node_t *new_node = _dll_new_node(list->op_mode, list->data_size, data);
    if (!new_node) return;
    dll_node_t *node = list->end->prev;
    while (pos) {
        node = node->prev;
        --pos;
    }
    new_node->prev = node;
    new_node->next = node->next;
    node->next->prev = new_node;
    node->next = new_node;
    list->size++;
}

// see dll.h
void dll_insert(dll_t *list, int pos, void *data) {
    if (pos < 0) {
        _dll_insert_from_end(list, -pos-1, data);
    } else {
        _dll_insert_from_begin(list, pos, data);
    }
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

// see dll.h
int dll_size(dll_t *list) {
    if (!list) {
        error("dll_count", "list is null");
        return -1;
    }
    return list->size;
}

// internal function
static void *_dll_remove_from_end(dll_t *list, int pos, void *dest) {
    if(!list) {
        error("dll_remove", "list is null");
        return NULL;
    }
    if(pos >= list->size || pos < 0) {
        error("dll_remove", "index out of range");
        return NULL;
    }
    dll_node_t *end = list->end;
    dll_node_t *node = end->prev;
    while (pos) {
        node = node->prev;
        --pos;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = NULL;
    node->prev = NULL;
    list->size--;
    return _dll_remove_node(list, node, dest);
}

// interal function
static void *_dll_remove_from_begin(dll_t *list, int pos, void *dest) {
    if(!list) {
        error("dll_remove", "list is null");
        return NULL;
    }
    if(pos >= list->size || pos < 0) {
        error("dll_remove", "index out of range");
        return NULL;
    }
    dll_node_t *end = list->end;
    dll_node_t *node = end->next;
    while (pos) {
        node = node->next;
        --pos;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = NULL;
    node->prev = NULL;
    list->size--;
    return _dll_remove_node(list, node, dest);
}

void *dll_remove(dll_t *list, int pos, void *dest) {
    if (pos < 0) {
        return _dll_remove_from_end(list, -pos-1, dest);
    } else {
        return _dll_remove_from_begin(list, pos, dest);
    }
}

void *dll_pop_back(dll_t *list, void *dest){
    return _dll_remove_from_end(list, 0, dest);
} 

void *dll_pop_front(dll_t *list, void *dest){
    return _dll_remove_from_begin(list, 0, dest);
}

static void *_dll_peek_from_end(dll_t *list, int pos) {
    if(!list) {
        error("dll_peek", "list is null");
        return NULL;
    }
    if(pos >= list->size || pos < 0) {
        error("dll_peek", "index out of range");
        return NULL;
    }
    dll_node_t *end = list->end;
    dll_node_t *node = end->prev;
    while (pos) {
        node = node->prev;
        --pos;
    }
    if (list->op_mode == REFERENCE) {
        return *(void **)node->data;
    } else {
        return node->data;
    }
}

static void *_dll_peek_from_begin(dll_t *list, int pos) {
    if(!list) {
        error("dll_peek", "list is null");
        return NULL;
    }
    if(pos >= list->size || pos < 0) {
        error("dll_peek", "index out of range");
        return NULL;
    }
    dll_node_t *end = list->end;
    dll_node_t *node = end->next;
    while (pos) {
        node = node->next;
        --pos;
    }
    if (list->op_mode == REFERENCE) {
        return *(void **)node->data;
    } else {
        return node->data;
    }
}

// see dll.h
void *dll_peek(dll_t *list, int pos) {
    if (pos < 0) {
        return _dll_peek_from_end(list, -pos-1);
    } else {
        return _dll_peek_from_begin(list, pos);
    }
}

// see dll.h
void dll_reverse(dll_t *list) {
    if (!list) {
        error("dll_reverse", "list is null");
        return;
    }
    dll_node_t *node = list->end;
    dll_node_t *tmp;
    dll_node_t *end = list->end;
    do {
        tmp = node->next;
        node->next = node->prev;
        node->prev = tmp;
        node = tmp;
    } while(node != end);
}

//see dll.h
void dll_clear(dll_t *list) {
    if (!list) {
        error("dll_reverse", "list is null");
        return;
    }
    while (list->size > 0) {
        _dll_remove_from_begin(list, 0, NULL);
    }
}

void dll_foreach(dll_t *list, foreach_fun func, void *usr) {
    if(!list || !func) {
        error("dll_foreach", "list or function is null");
        return;
    }
    dll_node_t *node = list->end->next;
    dll_node_t *end = list->end;
    op_mode mode = list->op_mode;
    int i = 0;
    while(node != end) {
        if (mode == REFERENCE) {
            (*func)(i, *(void **)node->data, usr);
        } else { // VALUE
            (*func)(i, node->data, usr);
        }
        ++i;
        node = node->next;
    }
}

dlli_t *dll_iter(dll_t *list) {
    if (!list) {
        error("dll_iterator", "list is null");
        return false;
    }
    dlli_t *iter = malloc(sizeof(*iter));
    if (!iter) {
        error("dll_iterator","Could not allocate enough memory");
        return NULL;
    }
    iter->list = list;
    iter->curr = list->end;
    return iter;
}

void dlli_delete(dlli_t *iter) {
    free(iter);
}

bool dlli_has_next(dlli_t *iter) {
    if (!iter) {
        error("dlli_has_next", "iterator is null");
        return false;
    }
    if (iter->curr->next != iter->list->end) {
        return true;
    }
    return false;
}

bool dlli_has_prev(dlli_t *iter) {
    if (!iter) {
        error("dlli_has_prev", "iterator is null");
        return false;
    }
    if (iter->curr->prev != iter->list->end) {
        return true;
    }
    return false;
}

void *dlli_next(dlli_t *iter) {
    if (!iter) {
        error("dlli_next", "iterator is null");
        return false;
    }
    if (iter->curr->next != iter->list->end) {
        iter->curr = iter->curr->next;
        if (iter->list->op_mode == REFERENCE) {
            return *(void **)iter->curr->data;
        } else {
            return iter->curr->data;
        }
    }
    return NULL;
}

void *dlli_prev(dlli_t *iter) {
    if (!iter) {
        error("dlli_prev", "iterator is null");
        return false;
    }
    if (iter->curr->prev != iter->list->end) {
        iter->curr = iter->curr->prev;
        if (iter->list->op_mode == REFERENCE) {
            return *(void **)iter->curr->data;
        } else {
            return iter->curr->data;
        }
    }
    return NULL;
}

static void _debug_print(dll_node_t *node) {
    printf("{ ");
    while (node) {
        printf("%d ", *(int *)node->data);
        node = node->next;
    }
    printf("}\n");
}

static void _append(dll_node_t **start, dll_node_t **stop, dll_node_t *n) {
    n->next = NULL;
    n->prev = NULL;
    if (!*start) {
        *start = n;
        *stop = n;
        return;
    }
    (*stop)->next = n;
    n->prev = *stop;
    *stop = n;
}

static dll_node_t *_merge(dll_node_t *l, dll_node_t *r, cmp c, op_mode m) {
    dll_node_t *start = NULL;
    dll_node_t *end = NULL;
    while(l || r) {
        dll_node_t *curr_l = l;
        dll_node_t *curr_r = r;
        if (l && r) {
            int res;
            if (m == REFERENCE) res = (*c)(*(void **)curr_l->data, *(void **)curr_r->data);
            else res = (*c)(curr_l->data, curr_r->data);
            if (res < 0) {
                r = r->next;
                _append(&start, &end, curr_r);
            } else {
                l = l->next;
                _append(&start, &end, curr_l);
            }
        } else if (l) {
            l = l->next;
            _append(&start, &end, curr_l);
        } else if (r) {
            r = r->next;
            _append(&start, &end, curr_r);
        }
    }

    _debug_print(start);
    return start;
}

static dll_node_t *_mergesort(dll_node_t *nodes, int size, cmp c, op_mode m) {
    //_debug_print(nodes);
    if (size < 2) return nodes;
    int l_size = size/2;
    int r_size = size - l_size;
    int i = l_size;
    dll_node_t *l = nodes;
    dll_node_t *r = l;
    while (i) {
        r = r->next;
        --i;
    }
    r->prev->next = NULL;
    r->prev = NULL;
    l = _mergesort(l, l_size, c, m);
    r = _mergesort(r, r_size, c, m);

    return _merge(l, r, c, m);
}

void dll_sort(dll_t *list, cmp c) {
    if (!list) {
        error("dll_sort", "list is null");
        return;
    }
    if (list->end->next == list->end) return;

    dll_node_t *nodes = list->end->next;
    list->end->next->prev = NULL;
    list->end->prev->next = NULL;
    
    nodes = _mergesort(nodes, list->size, c, list->op_mode);

    list->end->next = nodes;
    nodes->prev = list->end;
    while(nodes->next) nodes = nodes->next;
    list->end->prev = nodes;
    nodes->next = list->end;
}
