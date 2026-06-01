#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "alloc.h"

#define OPS 10000000

int main(){
    init();

    clock_t start = clock();

    void* a = custom_malloc(64);
    custom_free(a);

    void* b = custom_malloc(200);
    custom_free(b);
    void* c = custom_malloc(64);
    custom_free(c);

    void* d1 = custom_malloc(64);
    void* d2 = custom_malloc(64);
    custom_free(d2);
    custom_free(d1);

    void* e1 = custom_malloc(64);
    void* e2 = custom_malloc(64);
    void* e3 = custom_malloc(64);
    custom_free(e1);
    custom_free(e3);
    custom_free(e2);

    char* f1 = custom_malloc(64);
    char* f2 = custom_malloc(128);
    strcpy(f1, "hello");
    custom_free(f2);
    f1 = custom_realloc(150, f1);
    printf("%s\n", f1);
    custom_free(f1);

    char* g1 = custom_malloc(64);
    char* g2 = custom_malloc(64);
    strcpy(g1, "world");
    g1 = custom_realloc(5000, g1);
    printf("%s\n", g1);
    custom_free(g1);
    custom_free(g2);

    void* arr[1000] = {0};
    
    for(int i = 0; i < OPS; i++){
        int idx = rand() % 1000;
        if(arr[idx]){
            custom_free(arr[idx]);
            arr[idx] = NULL;
        } else {
            arr[idx] = custom_malloc(rand() % 2048 + 1);
        }
    }
    for(int i = 0; i < 1000; i++){
        if(arr[i]) custom_free(arr[i]);
    }
    
    clock_t end = clock();
    printf("Custom_allocator: %f\n", (double)(end - start) / CLOCKS_PER_SEC);

    start = clock();

    a = malloc(64);
    free(a);

    b = malloc(200);
    free(b);
    c = malloc(64);
    free(c);

    d1 = malloc(64);
    d2 = malloc(64);
    free(d2);
    free(d1);

    e1 = malloc(64);
    e2 = malloc(64);
    e3 = malloc(64);
    free(e1);
    free(e3);
    free(e2);

    f1 = malloc(64);
    f2 = malloc(128);
    strcpy(f1, "hello");
    free(f2);
    f1 = realloc(f1, 150);
    printf("%s\n", f1);
    free(f1);

    g1 = malloc(64);
    g2 = malloc(64);
    strcpy(g1, "world");
    g1 = realloc(g1, 5000);
    printf("%s\n", g1);
    free(g1);
    free(g2);

    memset(arr, 0, sizeof(arr));
    
    for(int i = 0; i < OPS; i++){
        int idx = rand() % 1000;
        if(arr[idx]){
            free(arr[idx]);
            arr[idx] = NULL;
        } else {
            arr[idx] = malloc(rand() % 2048 + 1);
        }
    }
    for(int i = 0; i < 1000; i++){
        if(arr[i]) free(arr[i]);
    }
    
    end = clock();
    printf("LibC allocator: %f\n", (double)(end - start) / CLOCKS_PER_SEC);
}