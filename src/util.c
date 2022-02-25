#include "util.h"

#include <string.h>

#include "xbdm.h"

char *PoolStrdup(const char *source, uint32_t tag) {
  uint32_t size = strlen(source) + 1;
  char *ret = DmAllocatePoolWithTag(size, tag);
  if (!ret) {
    return ret;
  }

  memcpy(ret, source, size);
  return ret;
}
