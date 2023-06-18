#ifndef DYDXT_COMMAND_PROCESSOR_UTIL_H
#define DYDXT_COMMAND_PROCESSOR_UTIL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define CP_API __attribute__((stdcall))
#else
#define CP_API
#endif  // #ifdef _WIN32

#define PCP_ERR_INVALID_INPUT -1
#define PCP_ERR_OUT_OF_MEMORY -2
#define PCP_ERR_INVALID_KEY -10
#define PCP_ERR_INVALID_INPUT_UNTERMINATED_QUOTED_KEY -11
#define PCP_ERR_INVALID_INPUT_UNTERMINATED_ESCAPE -12

#define DEFAULT_COMMAND_PARAMETER_RESERVE_SIZE 4

typedef struct CommandParameters {
  int32_t entries;
  char **keys;
  char **values;
} CommandParameters;

void CP_API CPDelete(CommandParameters *cp);

//! Parses an XBDM parameter string, populating the given result struct.
//! Returns the number of successfully parsed keys or a PCP_ERR_ define on
//! invalid input.
//!
//! Commands are of the form
//!   key1=value1 key2="quoted value2" ...
int32_t CP_API CPParseCommandParameters(const char *params,
                                        CommandParameters *result);

uint32_t CP_API CPPrintError(int32_t parse_return_code, char *buffer,
                             uint32_t buffer_len);

bool CP_API CPHasKey(const char *key, CommandParameters *cp);
bool CP_API CPGetString(const char *key, const char **result,
                        CommandParameters *cp);
bool CP_API CPGetUInt32(const char *key, uint32_t *result,
                        CommandParameters *cp);
bool CP_API CPGetInt32(const char *key, int32_t *result, CommandParameters *cp);

#ifdef __cplusplus
};  // exern "C"
#endif

#endif  // DYDXT_COMMAND_PROCESSOR_UTIL_H
