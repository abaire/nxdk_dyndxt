#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "command_processor_util.h"
#include "response_util.h"
#include "xbdm.h"

// Command that will be handled by this processor.
static const char kHandlerName[] = "ddxt";
#define TAG 'txdd'

typedef struct DXTContext {
  uint32_t dxt_main;
  uint32_t image_base;
  uint32_t num_tls_callbacks;
  uint32_t *tls_callbacks;
} DXTContext;

static HRESULT_API ProcessCommand(const char *command, char *response,
                                  DWORD response_len,
                                  struct CommandContext *ctx);
static HRESULT_API HandleReserve(const char *command, char *response,
                                 DWORD response_len,
                                 struct CommandContext *ctx);
static HRESULT_API HandleInstall(const char *command, char *response,
                                 DWORD response_len,
                                 struct CommandContext *ctx);

HRESULT __declspec(dllexport) DxtMain(void) {
  return DmRegisterCommandProcessor(kHandlerName, ProcessCommand);
}

static HRESULT_API ProcessCommand(const char *command, char *response,
                                  DWORD response_len,
                                  struct CommandContext *ctx) {
  const char *subcommand = command + sizeof(kHandlerName);

  if (!strncmp(subcommand, "hello", 5)) {
    strncpy(response, "Hi!", response_len);
    return XBOX_S_OK;
  }

  if (!strncmp(subcommand, "reserve", 7)) {
    return HandleReserve(command, response, response_len, ctx);
  }

  if (!strncmp(subcommand, "install", 7)) {
    return HandleInstall(command, response, response_len, ctx);
  }

  return SetXBDMErrorWithSuffix(XBOX_E_UNKNOWN_COMMAND, "Unknown command ",
                                command, response, response_len);
}

static HRESULT_API HandleReserve(const char *command, char *response,
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
    return SetXBDMError(XBOX_E_FAIL, "Missing required 'size' param", response,
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

static HRESULT_API ReceiveImageData(struct CommandContext *ctx,
                                    char *response, DWORD response_len) {
  return XBOX_E_UNEXPECTED;
}


static HRESULT_API HandleInstall(const char *command, char *response,
                                 DWORD response_len,
                                 struct CommandContext *ctx) {
  CommandParameters cp;
  int32_t result = ParseCommandParameters(command, &cp);
  if (result < 0) {
    return CPPrintError(result, response, response_len);
  }

  uint32_t base;
  bool base_found = CPGetUInt32("base", &base, &cp);
  uint32_t length;
  bool length_found = CPGetUInt32("length", &base, &cp);
  uint32_t dxt_main;
  bool main_found = CPGetUInt32("entrypoint", &base, &cp);
  CPDelete(&cp);

  if (!base_found) {
    return SetXBDMError(XBOX_E_FAIL, "Missing required 'base' param", response,
                        response_len);
  }
  if (!length_found) {
    return SetXBDMError(XBOX_E_FAIL, "Missing required 'length' param", response,
                        response_len);
  }
  if (!main_found) {
    return SetXBDMError(XBOX_E_FAIL, "Missing required 'entrypoint' param", response,
                        response_len);
  }
  if (!base) {
    return SetXBDMError(XBOX_E_FAIL, "Invalid 'base' param", response, response_len);
  }
  if (!length) {
    return SetXBDMError(XBOX_E_FAIL, "Invalid 'length' param", response, response_len);
  }
  if (!dxt_main || dxt_main < base) {
    return SetXBDMError(XBOX_E_FAIL, "Invalid 'entrypoint' param", response, response_len);
  }

  DXTContext *dxt_context = (DXTContext*)malloc(sizeof(DXTContext));
  dxt_context->dxt_main = dxt_main;
  dxt_context->image_base = base;
  dxt_context->num_tls_callbacks = 0;
  dxt_context->tls_callbacks = NULL;

  ctx->buffer = (void*)base;
  ctx->buffer_size = length;
  ctx->user_data = (void*)dxt_context;
  ctx->data_size = 0;
  ctx->bytes_remaining = length;
  ctx->handler = ReceiveImageData;

  return XBOX_S_SEND_BINARY;
}
