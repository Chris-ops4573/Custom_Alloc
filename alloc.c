#include <stdio.h>
#include "alloc.h"

#define heap_size 1024*1024
#define num_bins 10
#define largest_block 2048

static char heap[heap_size]; 
static int ptr = 0;

typedef struct Header{
    int size;
    int free;

    struct Header* free_next;
    struct Header* free_prev;
} Header;

typedef struct Footer{
    int size;
    int free;
} Footer;

typedef struct Bin{
    Header head;
    Header tail;
} Bin;

static Bin size_bins[num_bins];

void init(){
    for(int i = 0; i < num_bins; i++){
        size_bins[i].head.size = -1;
        size_bins[i].head.free = -1;
        size_bins[i].tail.size = -1;
        size_bins[i].tail.free = -1;

        size_bins[i].head.free_prev = NULL;
        size_bins[i].head.free_next = &size_bins[i].tail;

        size_bins[i].tail.free_next = NULL;
        size_bins[i].tail.free_prev = &size_bins[i].head;
    }
}

void show_heap(){
    char* trav = heap;
    
    printf("-----*-----\n");
    while(trav < heap + ptr){
        Header* header = (Header*)trav;

        printf("[H]__%d__[F]\n", header->size);

        trav += sizeof(Header) + header->size + sizeof(Footer);
    }
}

// Coalazcing with immediate next and prev allocated regions
// is enough for all freed regions to become continuous
void helper_coalasce(void* p){
    Header* curr_header = (Header*)((char*)p - sizeof(Header));

    Footer* prev_footer = NULL;
    Header* next_header = NULL;

    if((char*)curr_header != heap){
        prev_footer = (Footer*)((char*)curr_header - sizeof(Footer));
    }
    if((char*)curr_header + sizeof(Header) + curr_header->size + sizeof(Footer) < heap + ptr){
        next_header = (Header*)((char*)curr_header + sizeof(Header) + curr_header->size + sizeof(Footer));
    }

    if(prev_footer && prev_footer->free == 1){
        Header* prev_header = (Header*)((char*)prev_footer - prev_footer->size - sizeof(Header)); 
        prev_header->size = prev_footer->size + sizeof(Footer) + sizeof(Header) + curr_header->size;

        Footer* curr_footer = (Footer*)((char*)curr_header + sizeof(Header) + curr_header->size);
        curr_footer->size = prev_footer->size + sizeof(Footer) + sizeof(Header) + curr_header->size;

        curr_header = prev_header;
    }

    if(next_header && next_header->free == 1){
        Footer* next_footer = (Footer*)((char*)next_header + sizeof(Header) + next_header->size);
        next_footer->size = curr_header->size + sizeof(Footer) + sizeof(Header) + next_header->size;

        curr_header->size = curr_header->size + sizeof(Footer) + sizeof(Header) + next_header->size;
    }
}

void* custom_malloc(int req_size){
    req_size = (req_size + 7) & ~7;

    for(char* it = heap; it < heap + ptr;){
        Header* trav = (Header*) it;
        int old_size = trav->size;

        if(trav->free == 1 && trav->size == req_size){
            trav->free = 0;
            it += sizeof(Header);

            char* return_it = it;
            it += req_size;

            Footer* foot_trav = (Footer*) it;
            foot_trav->free = 0;

            return return_it;
        }

        if(trav->free == 1 && trav->size >= req_size + sizeof(Header) + sizeof(Footer)){
            trav->free = 0;
            trav->size = req_size;

            char* return_it = it + sizeof(Header);

            it += sizeof(Header) + req_size;

            Footer* foot_trav = (Footer*) it;
            foot_trav->free = 0;
            foot_trav->size = req_size;
            it += sizeof(Footer);

            Header* remain_head = (Header*) it;
            remain_head->free = 1;
            remain_head->size = old_size - req_size - sizeof(Header) - sizeof(Footer);

            it += sizeof(Header) + remain_head->size;

            Footer* remain_foot = (Footer*) it;
            remain_foot->free = 1;
            remain_foot->size = old_size - req_size - sizeof(Header) - sizeof(Footer);

            return return_it;
        } else {
            it += sizeof(Header) + trav->size + sizeof(Footer);
        }
    }

    if(ptr + sizeof(Header) + req_size + sizeof(Footer) >= heap_size){
        return NULL;
    }

    Header* header = (Header*) &heap[ptr];
    header->size = req_size;
    header->free = 0;
    ptr += sizeof(Header);

    char* p = &heap[ptr];
    ptr += req_size;

    Footer* footer = (Footer*) &heap[ptr];
    footer->size = req_size;
    footer->free = 0;
    ptr += sizeof(Footer);

    return (void*) p;
}

void custom_free(void* p){
    Header* header = (Header*)((char*)p - sizeof(Header));
    header->free = 1;

    Footer* foot_trav = (Footer*)((char*)p + header->size);
    foot_trav->free = 1;

    helper_coalasce(p);
}