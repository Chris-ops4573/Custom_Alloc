#ifndef ALLOC_H
#define ALLOC_H

void init();
void show_heap();

void* custom_malloc(int req_size);
void custom_free(void* p);

#endif