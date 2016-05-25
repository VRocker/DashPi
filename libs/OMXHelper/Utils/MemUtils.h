#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* _aligned_malloc(size_t s, size_t alignTo);
void _aligned_free(void* p);

#ifdef __cplusplus
}
#endif
