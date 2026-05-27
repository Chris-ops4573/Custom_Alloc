#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "alloc.h"

#define ALLOC_N 1000000

int main(){
    init();

    clock_t start = clock();
    for(int i = 1; i < ALLOC_N; i++){
        void* p = custom_malloc(i);
        custom_free(p);
    }
    clock_t end = clock();

    double elapsed = (double) (end - start)/CLOCKS_PER_SEC;
    printf("Custom_allocator: %f\n", elapsed);

    start = clock();
    for(int i = 1; i < ALLOC_N; i++){
        void* p = malloc(i);
        free(p);
    }
    end = clock();

    elapsed = (double) (end - start)/CLOCKS_PER_SEC;
    printf("LibC allocator: %f\n", elapsed);
}