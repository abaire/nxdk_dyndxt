#ifndef DYNDXT_LOADER_RESPONSE_UTIL_H
#define DYNDXT_LOADER_RESPONSE_UTIL_H

#include <stdint.h>

uint32_t SetXBDMError(uint32_t error_code, const char *message, char *response,
                      uint32_t response_len);
uint32_t SetXBDMErrorWithSuffix(uint32_t error_code, const char *message,
                                const char *suffix, char *response,
                                uint32_t response_len);

#endif  // DYNDXT_LOADER_RESPONSE_UTIL_H
