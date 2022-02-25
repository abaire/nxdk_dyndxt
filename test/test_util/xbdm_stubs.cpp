#include "xbdm_stubs.h"

#include <stdlib.h>
#include "xbdm.h"

// Allocate a new block of memory with the given tag.
PVOID_API DmAllocatePoolWithTag(DWORD size, DWORD tag) {
  return malloc(size);
}

// Free the given block, which was previously allocated via
// DmAllocatePoolWithTag.
VOID_API DmFreePool(void *block) {
  free(block);
}
