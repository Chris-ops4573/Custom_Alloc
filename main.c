#include "alloc.h"

int main(){
    init();

    void* a = custom_malloc(10);
    void* b = custom_malloc(30);
    void* c = custom_malloc(20);
    show_heap();

    custom_free(b);
    custom_free(a);
    show_heap();

    void* d = custom_malloc(1);
    void* e = custom_malloc(2);
    void* f = custom_malloc(3);
    show_heap();

    d = custom_realloc(3, d);
    d = custom_realloc(9, d);

    void* big = custom_malloc(1024*1024);
    show_heap();
}