#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>

void* xalloc(size_t, size_t, void* (*)(size_t, size_t));
void* twoarg_malloc(size_t, size_t);

#endif
