#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "command_processor_util.h"
#include "response_util.h"
#include "xbdm.h"

// Command that will be handled by this processor.
static const char kHandlerName[] = "ddxt";
#define TAG 'txdd'

typedef struct SendMethodAddressesContext {
  uint32_t next_method_index;
} SendMethodAddressesContext;

typedef HRESULT_API (*DxtMainProc)(void);
typedef struct ReceiveImageDataContext {
  DxtMainProc dxt_main;
  void *image_base;
  void *receive_pointer;
  uint32_t num_tls_callbacks;
  uint32_t *tls_callbacks;
} ReceiveImageDataContext;

// Reserve memory space for context objects used by multiline and binary receive
// responses.
static union {
  SendMethodAddressesContext send_method_addresses_context;
  ReceiveImageDataContext receive_image_data_context;
} context_store;

static HRESULT_API ProcessCommand(const char *command, char *response,
                                  DWORD response_len,
                                  struct CommandContext *ctx);
static HRESULT_API HandleHello(const char *command, char *response,
                               DWORD response_len, struct CommandContext *ctx);
static HRESULT_API HandleReserve(const char *command, char *response,
                                 DWORD response_len,
                                 struct CommandContext *ctx);
static HRESULT_API HandleInstall(const char *command, char *response,
                                 DWORD response_len,
                                 struct CommandContext *ctx);

static HRESULT_API SendMethodAddresses(struct CommandContext *ctx,
                                       char *response, DWORD response_len);

HRESULT DxtMain(void) {
  return DmRegisterCommandProcessor(kHandlerName, ProcessCommand);
}

static HRESULT_API ProcessCommand(const char *command, char *response,
                                  DWORD response_len,
                                  struct CommandContext *ctx) {
  const char *subcommand = command + sizeof(kHandlerName);

  if (!strncmp(subcommand, "hello", 5)) {
    return HandleHello(command + 5, response, response_len, ctx);
  }

  if (!strncmp(subcommand, "reserve", 7)) {
    return HandleReserve(command + 7, response, response_len, ctx);
  }

  if (!strncmp(subcommand, "install", 7)) {
    return HandleInstall(command + 7, response, response_len, ctx);
  }

  return SetXBDMErrorWithSuffix(XBOX_E_UNKNOWN_COMMAND, "Unknown command ",
                                command, response, response_len);
}

static HRESULT_API HandleHello(const char *command, char *response,
                               DWORD response_len, struct CommandContext *ctx) {
  SendMethodAddressesContext *response_context =
      &context_store.send_method_addresses_context;
  response_context->next_method_index = 0;

  ctx->user_data = response_context;
  ctx->handler = SendMethodAddresses;

  *response = 0;
  strncat(response, "DDXT methods", response_len);
  return XBOX_S_MULTILINE;
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

static HRESULT_API ReceiveImageData(struct CommandContext *ctx, char *response,
                                    DWORD response_len) {
  ReceiveImageDataContext *process_context = ctx->user_data;

  if (!ctx->data_size) {
    // Unclear if this can ever happen. Presumably it'd indicate some sort of
    // error and the image_base block should be freed.
    return XBOX_E_UNEXPECTED;
  }

  memcpy(process_context->receive_pointer, ctx->buffer, ctx->data_size);
  process_context->receive_pointer += ctx->data_size;

  ctx->bytes_remaining -= ctx->data_size;

  if (!ctx->bytes_remaining) {
    // TODO: Call any TLS callbacks.
    process_context->dxt_main();

    sprintf(response, "image_base=0x%X entrypoint=0x%X",
            (uint32_t)process_context->image_base,
            (uint32_t)process_context->dxt_main);
  }

  return XBOX_S_OK;
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
  bool length_found = CPGetUInt32("length", &length, &cp);
  uint32_t dxt_main;
  bool main_found = CPGetUInt32("entrypoint", &dxt_main, &cp);
  CPDelete(&cp);

  if (!base_found) {
    return SetXBDMError(XBOX_E_FAIL, "Missing required 'base' param", response,
                        response_len);
  }
  if (!length_found) {
    return SetXBDMError(XBOX_E_FAIL, "Missing required 'length' param",
                        response, response_len);
  }
  if (!main_found) {
    return SetXBDMError(XBOX_E_FAIL, "Missing required 'entrypoint' param",
                        response, response_len);
  }
  if (!base) {
    return SetXBDMError(XBOX_E_FAIL, "Invalid 'base' param", response,
                        response_len);
  }
  if (!length) {
    return SetXBDMError(XBOX_E_FAIL, "Invalid 'length' param", response,
                        response_len);
  }
  if (dxt_main < base) {
    return SetXBDMError(XBOX_E_FAIL, "Invalid 'entrypoint' param, < base",
                        response, response_len);
  }
  if (dxt_main >= base + length) {
    return SetXBDMError(XBOX_E_FAIL, "Invalid 'entrypoint' param, > image",
                        response, response_len);
  }

  ReceiveImageDataContext *process_context =
      &context_store.receive_image_data_context;
  process_context->dxt_main = (DxtMainProc)dxt_main;
  process_context->image_base = (void *)base;
  process_context->receive_pointer = process_context->image_base;
  process_context->num_tls_callbacks = 0;
  process_context->tls_callbacks = NULL;

  // TODO: Investigate whether the buffer can be used directly.
  // It's unclear if there is any guarantee that the buffer will be entirely
  // filled before calling the handler, or if it's safe to update the buffer
  // pointer in the handler (e.g., simply advancing by data_size).
  // If the buffer is not set here, a default buffer within xbdm is used.
  //  ctx->buffer = (void *)base;
  //  ctx->buffer_size = length;
  ctx->user_data = process_context;
  ctx->bytes_remaining = length;
  ctx->handler = ReceiveImageData;

  return XBOX_S_SEND_BINARY;
}

typedef struct MethodExport {
  const char *name;
  uint32_t address;
} MethodExport;

static const MethodExport kMethodExports[] = {
    {"CPDelete", (uint32_t)CPDelete},
    {"ParseCommandParameters", (uint32_t)ParseCommandParameters},
    {"CPPrintError", (uint32_t)CPPrintError},
    {"CPHasKey", (uint32_t)CPHasKey},
    {"CPGetString", (uint32_t)CPGetString},
    {"CPGetUInt32", (uint32_t)CPGetUInt32},
    {"CPGetInt32", (uint32_t)CPGetInt32},
};
#define NUM_METHOD_EXPORTS (sizeof(kMethodExports) / sizeof(kMethodExports[0]))

static HRESULT_API SendMethodAddresses(struct CommandContext *ctx,
                                       char *response, DWORD response_len) {
  SendMethodAddressesContext *rctx = ctx->user_data;
  if (rctx->next_method_index >= NUM_METHOD_EXPORTS) {
    return XBOX_S_NO_MORE_DATA;
  }

  const MethodExport *export = kMethodExports + rctx->next_method_index++;
  if (ctx->buffer_size < strlen(export->name) + 16) {
    return XBOX_E_ACCESS_DENIED;
  }

  sprintf(ctx->buffer, "%s=0x%08X", export->name, export->address);
  return XBOX_S_OK;
}
