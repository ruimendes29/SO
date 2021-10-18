#include <stdlib.h>

#include "arraylist.h"

ArrayList arraylist_create() {
    ArrayList a = malloc(sizeof(struct arraylist));
    a->size = 0;
    a->capacity = 10;
    a->values = malloc(sizeof(unsigned long int) * 10);
    return a;
}

void arraylist_add(ArrayList a, unsigned long int n) {
    if (a->size == a->capacity) {
        unsigned int new_capacity = 2 * a->capacity;
        a->values =
            realloc(a->values, sizeof(unsigned long int) * new_capacity);
        a->capacity = new_capacity;
    }
    a->values[a->size++] = n;
}
