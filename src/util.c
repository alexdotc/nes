#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "util.h"

void* xmalloc(size_t size){
	void* r;
    if ((r = malloc(size)) == NULL){
        fprintf(stderr, "fatal: malloc failed to allocate %lu bytes\n", size);
        exit(EXIT_FAILURE);
    }
    return r;
}
