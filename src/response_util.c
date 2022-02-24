#include "response_util.h"

#include <string.h>

uint32_t SetXBDMError(uint32_t error_code, const char *message, char *response,
                      uint32_t response_len) {
  *response = 0;
  strncat(response, message, response_len);
  return error_code;
}

uint32_t SetXBDMErrorWithSuffix(uint32_t error_code, const char *message,
                                const char *suffix, char *response,
                                uint32_t response_len) {
  uint32_t message_len = strlen(message);
  *response = 0;
  strncat(response, message, response_len);
  if (message_len < response_len) {
    strncat(response + message_len, suffix, response_len - message_len);
  }
  return error_code;
}
