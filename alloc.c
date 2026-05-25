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
        size_bins[i].head.free_next = &(size_bins[i].tail);

        size_bins[i].tail.free_next = NULL;
        size_bins[i].tail.free_prev = &(size_bins[i].head);
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

// Effectively constant time
int helper_get_range(int req_size){
    if(req_size > largest_block){
        return 9;
    }

    int index = 0;
    int upper = 16;

    while(req_size >= upper){
        index++;
        upper <<= 1;
    }

    return index;
}

// Coalazcing with immediate next and prev allocated regions
// is enough for all freed regions to become continuous
void* helper_coalasce(void* p){
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
        // Updating size bin for prev
        Header* prev_header = (Header*)((char*)prev_footer - prev_footer->size - sizeof(Header));
        Header* bin_prev = prev_header->free_prev;
        Header* bin_next = prev_header->free_next;

        bin_prev->free_next = bin_next;
        bin_next->free_prev = bin_prev;

        // Removing from heap
        prev_header->size = prev_footer->size + sizeof(Footer) + sizeof(Header) + curr_header->size;

        Footer* curr_footer = (Footer*)((char*)curr_header + sizeof(Header) + curr_header->size);
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
        next_footer->size = curr_header->size + sizeof(Footer) + sizeof(Header) + next_header->size;

        curr_header->size = curr_header->size + sizeof(Footer) + sizeof(Header) + next_header->size;
    }

    return (void*) curr_header;
}

void* custom_malloc(int req_size){
    req_size = (req_size + 7) & ~7;
    int idx = helper_get_range(req_size);

    Header* found = NULL;
    while(!found && idx < num_bins){
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
        Header* pre_found = found->free_prev;
        Header* post_found = found->free_next;

        pre_found->free_next = post_found;
        post_found->free_prev = pre_found;

        found->free = 0;
        found->free_next = NULL;
        found->free_prev = NULL;

        if(found->size >= req_size + sizeof(Header) + sizeof(Footer) + 8){
            int remain_size = found->size - req_size - sizeof(Header) - sizeof(Footer);
            int remain_idx = helper_get_range(remain_size);

            // Insert remaining space into heap
            found->size = req_size;

            Footer* found_footer = (Footer*)((char*)found + sizeof(Header) + req_size);
            found_footer->free = 0;
            found_footer->size = req_size;

            Header* remain_header = (Header*)((char*)found_footer + sizeof(Footer));
            remain_header->free = 1;
            remain_header->size = remain_size;

            Footer* remain_footer = (Footer*)((char*)remain_header + sizeof(Header) + remain_size);
            remain_footer->free = 1;
            remain_footer->size = remain_size;

            // Update size bins to reflect remaining free space
            Header* header_bin = &(size_bins[remain_idx].head);
            Header* header_bin_N = header_bin->free_next;

            header_bin->free_next = remain_header;

            remain_header->free_prev = header_bin;
            remain_header->free_next = header_bin_N;

            header_bin_N->free_prev = remain_header;
        }

        return(void*) ((char*)found + sizeof(Header));
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