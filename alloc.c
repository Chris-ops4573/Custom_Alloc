#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "alloc.h"

#define NUM_BINS 41
#define largest_8_block 256
#define largest_block 65536
#define MAGIC 0xCAFEBABE

#define MAGIC_VALIDATE(h)                                       \
    do {                                                        \
        if((h) == NULL || (h)->magic != MAGIC){                 \
            fprintf(stderr, "Heap curroption, magic number\n"); \
            exit(EXIT_FAILURE);                                 \
        }                                                       \
    } while (0)                                                 

static char* heap_start = NULL;
static char* heap_end = NULL; 
static int ptr = 0;

typedef struct Header{
    int size;
    int free;
    int magic;

    struct Header* free_next;
    struct Header* free_prev;
} Header;

typedef struct Footer{
    int size;
    int free;
    int magic;
} Footer;

typedef struct Bin{
    Header head;
    Header tail;
} Bin;

static Bin size_bins[NUM_BINS];

void helper_extend_heap(int size){
    sbrk(size);
    heap_end = sbrk(0);
}

// Effectively constant time
int helper_get_range(int req_size){
    if(req_size > largest_block){
        return 40;
    }

    if(req_size <= largest_8_block){
        return req_size/8 - 1;
    }

    int upper = 512;
    int index = 32;
    while(req_size >= upper){
        upper <<= 1;
        index++;
    }

    return index;
}

// Coalazcing with immediate next and prev allocated regions
// is enough for all freed regions to become continuous
void* helper_coalasce(void* p){
    Header* curr_header = (Header*)((char*)p - sizeof(Header));
    MAGIC_VALIDATE(curr_header);

    Footer* prev_footer = NULL;
    Header* next_header = NULL;

    if((char*)curr_header != heap_start){
        prev_footer = (Footer*)((char*)curr_header - sizeof(Footer));
        MAGIC_VALIDATE(prev_footer);
    }
    if((char*)curr_header + sizeof(Header) + curr_header->size + sizeof(Footer) < heap_start + ptr){
        next_header = (Header*)((char*)curr_header + sizeof(Header) + curr_header->size + sizeof(Footer));
        MAGIC_VALIDATE(next_header);
    }

    if(prev_footer && prev_footer->free == 1){
        // Updating size bin for prev
        Header* prev_header = (Header*)((char*)prev_footer - prev_footer->size - sizeof(Header));
        MAGIC_VALIDATE(prev_header);

        Header* bin_prev = prev_header->free_prev;
        Header* bin_next = prev_header->free_next;

        bin_prev->free_next = bin_next;
        bin_next->free_prev = bin_prev;

        // Removing from heap
        prev_header->size = prev_footer->size + sizeof(Footer) + sizeof(Header) + curr_header->size;

        Footer* curr_footer = (Footer*)((char*)curr_header + sizeof(Header) + curr_header->size);
        MAGIC_VALIDATE(curr_footer);
        curr_footer->size = prev_footer->size + sizeof(Footer) + sizeof(Header) + curr_header->size;

        curr_header = prev_header;

    }

    if(next_header && next_header->free == 1){
        // Updating size bin for next
        Header* bin_prev = next_header->free_prev;
        Header* bin_next = next_header->free_next;

        bin_prev->free_next = bin_next;
        bin_next->free_prev = bin_prev;

        // Removing from heap
        Footer* next_footer = (Footer*)((char*)next_header + sizeof(Header) + next_header->size);
        MAGIC_VALIDATE(next_footer);
        next_footer->size = curr_header->size + sizeof(Footer) + sizeof(Header) + next_header->size;

        curr_header->size = curr_header->size + sizeof(Footer) + sizeof(Header) + next_header->size;
    }

    return (void*) curr_header;
}

void init(){
    heap_start = sbrk(0);
    helper_extend_heap(1024*1024);

    for(int i = 0; i < NUM_BINS; i++){
        size_bins[i].head.size = -1;
        size_bins[i].head.free = -1;
        size_bins[i].tail.size = -1;
        size_bins[i].tail.free = -1;

        size_bins[i].head.free_prev = NULL;
        size_bins[i].head.free_next = &(size_bins[i].tail);

        size_bins[i].tail.free_next = NULL;
        size_bins[i].tail.free_prev = &(size_bins[i].head);
    }
}

void show_heap(){
    char* trav = heap_start;
    
    printf("-----*-----\n");
    while(trav < heap_start + ptr){
        Header* header = (Header*)trav;
        MAGIC_VALIDATE(header);

        Footer* footer = (Footer*)(trav + sizeof(Header) + header->size);
        MAGIC_VALIDATE(footer);

        if(header->free == 1){
            printf("* ");
        }

        printf("[H]__%d__[F]\n", header->size);

        trav += sizeof(Header) + header->size + sizeof(Footer);
    }
}

void validate_heap(){
    char* trav = heap_start;

    while(trav < heap_start + ptr){
        Header* head = (Header*) trav;
        Footer* foot = (Footer*)(trav + sizeof(Header) + head->size);

        if(head->size != foot->size || head->free != foot->free){
            printf("Mismatch: H: %d, %d & F: %d, %d\n", head->free, head->size, foot->free, foot->size);
        }

        trav += sizeof(Header) + head->size + sizeof(Footer);
    }
}

void* custom_malloc(int req_size){
    req_size = (req_size + 7) & ~7;
    int idx = helper_get_range(req_size);

    Header* found = NULL;
    while(!found && idx < NUM_BINS){
        Header* it = size_bins[idx].head.free_next;

        while(!found && it != &(size_bins[idx].tail)){
            if(it->size < req_size){
                it = it->free_next;
            } else {
                found = it;
                break;
            }
        }

        if(!found){
            idx++;
        }
    }

    if(found){
        MAGIC_VALIDATE(found);

        Header* pre_found = found->free_prev;
        Header* post_found = found->free_next;

        pre_found->free_next = post_found;
        post_found->free_prev = pre_found;

        found->free = 0;
        found->free_next = NULL;
        found->free_prev = NULL;

        Footer* found_old_footer = (Footer*) ((char*)found + sizeof(Header) + found->size);
        found_old_footer->free = 0;

        if(found->size >= req_size + sizeof(Header) + sizeof(Footer) + 8){
            int remain_size = found->size - req_size - sizeof(Header) - sizeof(Footer);
            int remain_idx = helper_get_range(remain_size);

            // Insert remaining space into heap
            found->size = req_size;

            Footer* found_footer = (Footer*)((char*)found + sizeof(Header) + req_size);
            found_footer->free = 0;
            found_footer->size = req_size;
            found_footer->magic = MAGIC;

            Header* remain_header = (Header*)((char*)found_footer + sizeof(Footer));
            remain_header->free = 1;
            remain_header->size = remain_size;
            remain_header->magic = MAGIC;

            Footer* remain_footer = (Footer*)((char*)remain_header + sizeof(Header) + remain_size);
            remain_footer->free = 1;
            remain_footer->size = remain_size;
            remain_footer->magic = MAGIC;

            // Update size bins to reflect remaining free space
            Header* header_bin = &(size_bins[remain_idx].head);
            Header* header_bin_N = header_bin->free_next;

            header_bin->free_next = remain_header;

            remain_header->free_prev = header_bin;
            remain_header->free_next = header_bin_N;

            header_bin_N->free_prev = remain_header;
        }

        return (void*)((char*)found + sizeof(Header));
    }

    while(heap_start + ptr + sizeof(Header) + req_size + sizeof(Footer) >= heap_end){
        helper_extend_heap(1024*1024);
    }

    Header* header = (Header*)(heap_start + ptr);
    header->size = req_size;
    header->free = 0;
    header->magic = MAGIC;
    ptr += sizeof(Header);

    char* p = heap_start + ptr;
    ptr += req_size;

    Footer* footer = (Footer*)(heap_start + ptr);
    footer->size = req_size;
    footer->free = 0;
    footer->magic = MAGIC;
    ptr += sizeof(Footer);

    return (void*)p;
}

void* custom_realloc(int req_size, void* p){
    req_size = (req_size + 7) & ~7;

    if(req_size == 0){
        custom_free(p);
        return NULL;
    }

    // Size already big enough
    Header* initial_header = (Header*)((char*)p - sizeof(Header));
    MAGIC_VALIDATE(initial_header);
    if(initial_header->size >= req_size){
        return p;
    }

    // Requires next block
    Header* next_header = (Header*)((char*)p + initial_header->size + sizeof(Footer));
    if((char*)next_header < heap_start + ptr && next_header->free == 1 && initial_header->size + sizeof(Footer) + sizeof(Header) + next_header->size >= req_size){
        // Clearing from free bins
        MAGIC_VALIDATE(next_header);

        Header* next_header_P = next_header->free_prev;
        Header* next_header_N = next_header->free_next;

        next_header_P->free_next = next_header_N;
        next_header_N->free_prev = next_header_P;

        next_header->free_next = NULL;
        next_header->free_prev = NULL;

        // Heap resizing
        Footer* next_footer = (Footer*)((char*)next_header + sizeof(Header) + next_header->size);
        MAGIC_VALIDATE(next_footer);

        int merged_size = initial_header->size + sizeof(Footer) + sizeof(Header) + next_header->size;
        initial_header->size = merged_size;
        next_footer->size = merged_size;

        initial_header->free = 0;
        next_footer->free = 0;

        // Splitting up remaining chunk if big enough left to be marked free
        if(initial_header->size >= sizeof(Header) + sizeof(Footer) + req_size + 8){
            // Splitting the heap
            int remain_size = initial_header->size - sizeof(Footer) - sizeof(Header) - req_size;
            int remain_index = helper_get_range(remain_size);

            initial_header->size = req_size;
            initial_header->free = 0;

            Footer* initial_footer = (Footer*)((char*)initial_header + sizeof(Header) + req_size);
            initial_footer->size = req_size;
            initial_footer->free = 0;
            initial_footer->magic = MAGIC;

            Header* remain_header = (Header*)((char*)initial_footer + sizeof(Footer));
            remain_header->free = 1;
            remain_header->size = remain_size;
            remain_header->magic = MAGIC;

            Footer* remain_footer = (Footer*)((char*)remain_header + sizeof(Header) + remain_size);
            remain_footer->free = 1;
            remain_footer->size = remain_size;
            remain_footer->magic = MAGIC;

            // Adding remainder to free bin
            Header* bin_index_head = &(size_bins[remain_index].head);
            Header* bin_index_head_N = bin_index_head->free_next;

            bin_index_head->free_next = remain_header;

            remain_header->free_prev = bin_index_head;
            remain_header->free_next = bin_index_head_N;

            bin_index_head_N->free_prev = remain_header;
        }

        return (void*)((char*)initial_header + sizeof(Header));
    }

    //If next block is not big enough change location to big enough block and free current allocation
    void* new_location = custom_malloc(req_size);
    memcpy(new_location, p, initial_header->size);

    custom_free(p);
    
    return new_location;
}

void custom_free(void* p){
    Header* header = (Header*)((char*)p - sizeof(Header));
    MAGIC_VALIDATE(header);
    if(header->free == 1){
        printf("Heap curroption, double free \n");
        exit(EXIT_FAILURE);
    }

    header->free = 1;

    Footer* foot_trav = (Footer*)((char*)p + header->size);
    MAGIC_VALIDATE(foot_trav);
    foot_trav->free = 1;

    void* header_ptr = helper_coalasce(p);

    Header* new_header = (Header*) header_ptr;
    int header_idx = helper_get_range(new_header->size);

    Header* bin_head = &(size_bins[header_idx].head);
    Header* next_head = bin_head->free_next;

    bin_head->free_next = new_header;

    new_header->free_prev = bin_head;
    new_header->free_next = next_head;

    next_head->free_prev = new_header;
}