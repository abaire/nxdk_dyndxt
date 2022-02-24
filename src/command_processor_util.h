#ifndef DYDXT_COMMAND_PROCESSOR_UTIL_H
#define DYDXT_COMMAND_PROCESSOR_UTIL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef DLLEXPORT
#define DECLSPEC __declspec(dllexport)
#else
#ifdef __NO_DLL_LINKAGE
#define DECLSPEC
#else
#define DECLSPEC __declspec(dllimport)
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

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

void DECLSPEC CPDelete(CommandParameters *cp);

// Parses an XBDM parameter string, populating the given result struct.
// Returns the number of successfully parsed keys or a PCP_ERR_ define on
// invalid input.
int32_t DECLSPEC ParseCommandParameters(const char *params,
                                        CommandParameters *result);

uint32_t DECLSPEC CPPrintError(int32_t parse_return_code, char *buffer,
                               uint32_t buffer_len);

bool DECLSPEC CPHasKey(const char *key, CommandParameters *cp);
bool DECLSPEC CPGetString(const char *key, const char **result,
                          CommandParameters *cp);
bool DECLSPEC CPGetUInt32(const char *key, uint32_t *result,
                          CommandParameters *cp);
bool DECLSPEC CPGetInt32(const char *key, int32_t *result,
                         CommandParameters *cp);

#endif  // DYDXT_COMMAND_PROCESSOR_UTIL_H

#ifdef __cplusplus
};  // exern "C"
#endif