# Custom_Alloc

A custom heap allocator written in C, implementing `malloc`, `free`, and `realloc` from scratch using `sbrk`. Designed as a drop-in replacement for any C program.

---

## How It Works

The allocator uses a **segregated free list** architecture with boundary tags.

### Memory Layout

Every allocation is wrapped in a header and footer:

```
[Header | size | free | magic | prev* | next*] [user data] [Footer | size | free | magic]
```

This doubly linked list lets the allocator find adjacent blocks in O(1) time for coalescing, and validate heap integrity via a magic number (`0xCAFEBABE`).

### Size Bins

Free blocks are organised into 41 segregated bins:

| Bin Range | Granularity |
|---|---|
| 0 – 31 | 8-byte steps (8 to 256 bytes) |
| 32 – 39 | Power-of-two steps (512 to 65536 bytes) |
| 40 | Everything larger |

On `malloc`, the allocator finds the correct bin for the requested size and searches upward until a large enough free block is found. On `free`, the block is coalesced with its immediate neighbours and inserted at the head of the appropriate bin.

### Key Features

- **Immediate coalescing** — adjacent free blocks are merged on every `free`, preventing fragmentation buildup
- **Block splitting** — oversized free blocks are split, with the remainder returned to its bin
- **Magic number validation** — every header and footer carries `0xCAFEBABE`; mismatches abort with an error, catching corruption and double-frees early
- **Heap extension** — the heap grows automatically in 1MB increments via `sbrk` when no suitable free block exists


---

## Benchmark Results

Tested against glibc on a mixed workload of 1,000,000 random `malloc`/`free` operations with sizes from 1–2048 bytes.

```
Custom allocator:  ~0.058s
glibc allocator:   ~0.040s
```

Summary: 1.5x glibc. At lower iteration counts (≤1000 ops) the custom allocator matches or beats glibc, since glibc's tcache has higher setup overhead relative to work done.

---

## API

```c
void  init();                            // Must be called once before any allocation
void* custom_malloc(int size);           // Allocate size bytes
void* custom_realloc(int size, void* p); // Resize an existing allocation
void  custom_free(void* p);             // Free an allocation

void  show_heap();                       // Print a visual map of the heap
void  validate_heap();                   // Check all header/footer pairs match
```

> **Note:** `custom_realloc` takes `(new_size, pointer)` — the argument order is reversed compared to standard `realloc`.

---

## Building

```bash
# Compile the test driver
gcc -O2 -o main main.c alloc.c

# Compile and run the benchmark
gcc -O2 -o bench bench.c alloc.c
./bench

# Debug build — enables magic number validation on every operation
gcc -O2 -DDEBUG -o main_debug main.c alloc.c
```

---

## Using With Your Own Program

**1. Include the header**

```c
#include "alloc.h"
```

**2. Call `init()` once at startup**

```c
int main() {
    init();
    // your code
}
```

**3. Replace standard calls**

| Standard | Custom |
|---|---|
| `malloc(size)` | `custom_malloc(size)` |
| `free(ptr)` | `custom_free(ptr)` |
| `realloc(ptr, size)` | `custom_realloc(size, ptr)` |

**4. Compile together**

```bash
gcc -O2 -o your_program your_program.c alloc.c
```

### Example

```c
#include "alloc.h"
#include <string.h>
#include <stdio.h>

int main() {
    init();

    char* buf = custom_malloc(64);
    strcpy(buf, "hello");

    buf = custom_realloc(256, buf);
    printf("%s\n", buf);   // hello

    custom_free(buf);
    return 0;
}
```

---

## File Structure

```
.
├── alloc.c      # Allocator implementation
├── alloc.h      # Public API
├── bench.c      # Benchmark against glibc
└── README.md
```