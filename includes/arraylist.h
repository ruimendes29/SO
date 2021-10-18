#ifndef __ARRAY_LIST__
#define __ARRAY_LIST__

typedef struct arraylist {
    unsigned int size;      // número de ocupados
    unsigned int capacity;  // capacidade atual do array
    unsigned long int *values;
} * ArrayList;

ArrayList arraylist_create();

void arraylist_add(ArrayList a, unsigned long int n);

#endif
