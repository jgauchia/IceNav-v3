#pragma once
#include <stdlib.h>
#define MALLOC_CAP_SPIRAM 0
static inline void *heap_caps_malloc(size_t n, int) { return malloc(n); }
