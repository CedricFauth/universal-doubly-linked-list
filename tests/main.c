#include <stdio.h>
#include "dll.h"

typedef struct tmp {
    int i;
    double d;
} tmp;

// tests
void test_display(void *data) {
    tmp *t = (tmp *)data;
    printf("%d, %.2f", t->i, t->d);
}

int main(int argc, char const *argv[]) {

    dll_t *list = dll_new(sizeof(tmp));

    dll_display(list, test_display);

    tmp t;
    t.i = 123;
    t.d = 1.23;
    dll_push_back(list, (void *)&t);dll_display(list, test_display);

    t.i = 11;
    t.d = 1.1;
    dll_insert(list, -1, (void *)&t);dll_display(list, test_display);

    t.i = 99;
    t.d = 9.9;
    dll_insert(list, -2, (void *)&t);dll_display(list, test_display);

    t.i = 0;
    t.d = 0.0;
    dll_push_back(list, (void *)&t);dll_display(list, test_display);

    dll_delete(list, NULL);
}
