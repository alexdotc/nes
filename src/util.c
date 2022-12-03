#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "util.h"

void* xalloc(size_t num, size_t size, void* (*allocator)(size_t,size_t)){
	void* r = (*allocator)(num,size);
    if (r == NULL)
        err_exit("fatal: Failed to allocate %lu bytes\n", size);
    return r;
}

void* twoarg_malloc(size_t num, size_t size){
    return malloc(num * size);
}

void err_exit(const char* format, ...){
    fprintf(stderr, "fatal: ");
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}
