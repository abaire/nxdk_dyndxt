#ifndef DYNDXT_LOADER_UTIL_H
#define DYNDXT_LOADER_UTIL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Basic strdup implementation using DmAllocatePool memory.
char *PoolStrdup(const char *source, uint32_t tag);

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // DYNDXT_LOADER_UTIL_H
