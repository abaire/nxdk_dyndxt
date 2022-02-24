#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "command_processor_util.h"
#include "response_util.h"
#include "xbdm.h"

// Command that will be handled by this processor.
static const char kHandlerName[] = "ddxt";
#define TAG 'txdd'

static HRESULT_API process_command(const char *command, char *response,
                                   DWORD response_len,
                                   struct CommandContext *ctx);
static HRESULT_API handle_reserve(const char *command, char *response,
                                  DWORD response_len,
                                  struct CommandContext *ctx);
static HRESULT_API handle_install(const char *command, char *response,
                                  DWORD response_len,
                                  struct CommandContext *ctx);

HRESULT __declspec(dllexport) DxtMain(void) {
  return DmRegisterCommandProcessor(kHandlerName, process_command);
}

static HRESULT_API process_command(const char *command, char *response,
                                   DWORD response_len,
                                   struct CommandContext *ctx) {
  const char *subcommand = command + sizeof(kHandlerName);

  if (!strncmp(subcommand, "hello", 5)) {
    strncpy(response, "Hi!", response_len);
    return XBOX_S_OK;
  }

  if (!strncmp(subcommand, "reserve", 7)) {
    return handle_reserve(command, response, response_len, ctx);
  }

  if (!strncmp(subcommand, "install", 7)) {
    return handle_install(command, response, response_len, ctx);
  }

  return SetXBDMErrorWithSuffix(XBOX_E_UNKNOWN_COMMAND, "Unknown command ",
                                command, response, response_len);
}

static HRESULT_API handle_reserve(const char *command, char *response,
                                  DWORD response_len,
                                  struct CommandContext *ctx) {
  CommandParameters cp;

  int32_t result = ParseCommandParameters(command, &cp);
  if (result < 0) {
    return CPPrintError(result, response, response_len);
  }

  uint32_t size;
  bool size_found = CPGetUInt32("size", &size, &cp);
  CPDelete(&cp);

  if (!size_found) {
    return SetXBDMError(XBOX_E_FAIL, "Missing required size param", response,
                        response_len);
  }

  void *allocation = DmAllocatePoolWithTag(size, TAG);
  if (!allocation) {
    return SetXBDMError(XBOX_E_ACCESS_DENIED, "Allocation failed", response,
                        response_len);
  }

  sprintf(response, "addr=0x%X", (uint32_t)allocation);
  return XBOX_S_OK;
}

static HRESULT_API handle_install(const char *command, char *response,
                                  DWORD response_len,
                                  struct CommandContext *ctx) {
  strncpy(response, "OK?!?", response_len);
  return XBOX_S_OK;
}
