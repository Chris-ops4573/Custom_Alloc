#include "alloc.h"
#include <stdio.h>

int main(){
    init();

    void* a = custom_malloc(10);
    void* b = custom_malloc(30);
    void* c = custom_malloc(1);
    show_heap();

    custom_free(a);
    custom_free(b);
    show_heap();

    c = custom_realloc(3, c);
    c = custom_realloc(9, c);

    void* big = custom_malloc(1024);
    show_heap();

    char* e = custom_malloc(2);

    // Memory curroption/invalid access
    e[49] = 'a';
    custom_free(e);
    show_heap();
}