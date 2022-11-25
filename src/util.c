#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "util.h"

void* xalloc(size_t num, size_t size, void* (*allocator)(size_t,size_t)){
	void* r = (*allocator)(num,size);
    if (r == NULL){
        fprintf(stderr, "fatal: failed to allocate %lu bytes\n", size);
        exit(EXIT_FAILURE);
    }
    return r;
}

void* twoarg_malloc(size_t num, size_t size){
    return malloc(num * size);
}
