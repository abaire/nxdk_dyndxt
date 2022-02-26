#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "command_processor_util.h"
#include "dll_loader.h"
#include "module_registry.h"
#include "response_util.h"
#include "util.h"
#include "xbdm.h"

// Command that will be handled by this processor.
static const char kHandlerName[] = "ddxt";
static const uint32_t kTag = 0x64647874;  // 'ddxt'

typedef struct SendMethodAddressesContext {
  ModuleRegistryCursor cursor;
} SendMethodAddressesContext;

typedef HRESULT_API (*DxtMainProc)(void);
typedef struct ReceiveImageDataContext {
  DxtMainProc dxt_main;
  void *image_base;
  uint32_t raw_image_size;
  void *receive_pointer;
  uint32_t num_tls_callbacks;
  uint32_t *tls_callbacks;
  bool relocation_needed;
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

// Trivial request to indicate that this DLL is running. Enumerates the module
// export registry to aid debugging.
static HRESULT HandleHello(const char *command, char *response,
                           DWORD response_len, struct CommandContext *ctx);

// Registers a method exported by some module. E.g.,
// "xboxkrnl.exe @ 1 (_AvGetSavedDataAddress@0) = 0x8003FE0E"
static HRESULT HandleRegisterModuleExport(const char *command, char *response,
                                          DWORD response_len,
                                          struct CommandContext *ctx);

// Loads a DLL image, relocates it, and invokes its entrypoint.
static HRESULT HandleDynamicLoad(const char *command, char *response,
                                 DWORD response_len,
                                 struct CommandContext *ctx);

// Allocates a block of memory. Intended for use with the "install" command.
// The all-in-one "load" command should be preferred for most cases.
static HRESULT HandleReserve(const char *command, char *response,
                             DWORD response_len, struct CommandContext *ctx);

// Loads a pre-relocated DLL image and invokes its entrypoint.
// The all-in-one "load" command should be preferred for most cases.
static HRESULT HandleInstall(const char *command, char *response,
                             DWORD response_len, struct CommandContext *ctx);

static HRESULT_API SendMethodAddresses(struct CommandContext *ctx,
                                       char *response, DWORD response_len);
static HRESULT_API ReceiveImageData(struct CommandContext *ctx, char *response,
                                    DWORD response_len);

static HRESULT ReceiveImageDataComplete(ReceiveImageDataContext *ctx,
                                        char *response, DWORD response_len);
static bool RegisterExport(const char *name, uint32_t ordinal,
                           uint32_t address);

HRESULT_API DxtMain(void) {
  // Register methods exported by this DLL for use in DLLs to be loaded later.
  RegisterExport("CPDelete@4", 2, (uint32_t)CPDelete);
  RegisterExport("ParseCommandParameters@8", 3,
                 (uint32_t)ParseCommandParameters);
  RegisterExport("CPPrintError@12", 4, (uint32_t)CPPrintError);
  RegisterExport("CPHasKey@8", 5, (uint32_t)CPHasKey);
  RegisterExport("CPGetString@12", 6, (uint32_t)CPGetString);
  RegisterExport("CPGetUInt32@12", 7, (uint32_t)CPGetUInt32);
  RegisterExport("CPGetInt32@12", 8, (uint32_t)CPGetInt32);
  RegisterExport("MRRegisterMethod@8", 9, (uint32_t)MRRegisterMethod);
  RegisterExport("MRGetMethodByOrdinal@12", 10, (uint32_t)MRGetMethodByOrdinal);
  RegisterExport("MRGetMethodByName@12", 11, (uint32_t)MRGetMethodByName);

  return DmRegisterCommandProcessor(kHandlerName, ProcessCommand);
}

static HRESULT_API ProcessCommand(const char *command, char *response,
                                  DWORD response_len,
                                  struct CommandContext *ctx) {
  const char *subcommand = command + sizeof(kHandlerName);

  if (!strncmp(subcommand, "hello", 5)) {
    return HandleHello(command + 5, response, response_len, ctx);
  }

  if (!strncmp(subcommand, "load", 4)) {
    return HandleDynamicLoad(command + 4, response, response_len, ctx);
  }

  if (!strncmp(subcommand, "reserve", 7)) {
    return HandleReserve(command + 7, response, response_len, ctx);
  }

  if (!strncmp(subcommand, "install", 7)) {
    return HandleInstall(command + 7, response, response_len, ctx);
  }

  if (!strncmp(subcommand, "export", 6)) {
    return HandleRegisterModuleExport(command + 6, response, response_len, ctx);
  }

  return SetXBDMErrorWithSuffix(XBOX_E_UNKNOWN_COMMAND, "Unknown command ",
                                command, response, response_len);
}

static HRESULT_API SendMethodAddresses(struct CommandContext *ctx,
                                       char *response, DWORD response_len) {
  SendMethodAddressesContext *rctx = ctx->user_data;

  const char *module_name;
  const ModuleExport *entry;

  if (!MREnumerateRegistry(&module_name, &entry, &rctx->cursor)) {
    return XBOX_S_NO_MORE_DATA;
  }

  static const char no_name[] = "";
  const char *export_name = entry->method_name ? entry->method_name : no_name;
  sprintf(ctx->buffer, "%s @ %d (%s) = 0x%08X", module_name, entry->ordinal,
          export_name, entry->address);
  return XBOX_S_OK;
}

static HRESULT HandleHello(const char *command, char *response,
                           DWORD response_len, struct CommandContext *ctx) {
  SendMethodAddressesContext *response_context =
      &context_store.send_method_addresses_context;
  MREnumerateRegistryBegin(&response_context->cursor);

  ctx->user_data = response_context;
  ctx->handler = SendMethodAddresses;

  *response = 0;
  strncat(response, "Registered exports", response_len);
  return XBOX_S_MULTILINE;
}

static HRESULT HandleDynamicLoad(const char *command, char *response,
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

  void *allocation = DmAllocatePoolWithTag(size, kTag);
  if (!allocation) {
    return SetXBDMError(XBOX_E_ACCESS_DENIED, "Allocation failed", response,
                        response_len);
  }

  ReceiveImageDataContext *process_context =
      &context_store.receive_image_data_context;
  process_context->dxt_main = NULL;
  process_context->image_base = (void *)allocation;
  process_context->raw_image_size = size;
  process_context->receive_pointer = process_context->image_base;
  process_context->num_tls_callbacks = 0;
  process_context->tls_callbacks = NULL;
  process_context->relocation_needed = true;

  // TODO: Investigate whether the buffer can be used directly.
  // It's unclear if there is any guarantee that the buffer will be entirely
  // filled before calling the handler, or if it's safe to update the buffer
  // pointer in the handler (e.g., simply advancing by data_size).
  // If the buffer is not set here, a default buffer within xbdm is used.
  //  ctx->buffer = (void *)base;
  //  ctx->buffer_size = length;
  ctx->user_data = process_context;
  ctx->bytes_remaining = size;
  ctx->handler = ReceiveImageData;

  return XBOX_S_SEND_BINARY;
}

static HRESULT HandleReserve(const char *command, char *response,
                             DWORD response_len, struct CommandContext *ctx) {
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

  void *allocation = DmAllocatePoolWithTag(size, kTag);
  if (!allocation) {
    return SetXBDMError(XBOX_E_ACCESS_DENIED, "Allocation failed", response,
                        response_len);
  }

  sprintf(response, "addr=0x%X", (uint32_t)allocation);
  return XBOX_S_OK;
}

static void *DLL_LOADER_API AllocateImage(size_t size) {
  return DmAllocatePoolWithTag(size, kTag);
}

static HRESULT ReceiveImageDataComplete(ReceiveImageDataContext *receive_ctx,
                                        char *response, DWORD response_len) {
  if (!receive_ctx->relocation_needed) {
    // TODO: Call any TLS callbacks.
    receive_ctx->dxt_main();

    sprintf(response, "image_base=0x%X entrypoint=0x%X",
            (uint32_t)receive_ctx->image_base, (uint32_t)receive_ctx->dxt_main);
    return XBOX_S_OK;
  }

  DLLContext ctx;
  memset(&ctx, 0, sizeof(ctx));

  ctx.input.raw_data = receive_ctx->image_base;
  ctx.input.raw_data_size = receive_ctx->raw_image_size;
  ctx.input.alloc = AllocateImage;
  ctx.input.free = DmFreePool;
  ctx.input.resolve_import_by_ordinal = MRGetMethodByOrdinal;
  ctx.input.resolve_import_by_name = MRGetMethodByName;

  if (!DLLLoad(&ctx)) {
    sprintf(response, "DLLLoad failed %d::%d", ctx.output.context,
            ctx.output.status);
    DLLFreeContext(&ctx, false);
    return XBOX_E_FAIL;
  }

  if (!DLLInvokeTLSCallbacks(&ctx)) {
    sprintf(response, "Failed to invoke TLS callbacks %d::%d",
            ctx.output.context, ctx.output.status);
    DLLFreeContext(&ctx, false);
    return XBOX_E_FAIL;
  }

  DxtMainProc entrypoint = (DxtMainProc)ctx.output.entrypoint;
  sprintf(response, "image_base=0x%X entrypoint=0x%X",
          (uint32_t)receive_ctx->image_base, (uint32_t)entrypoint);

  entrypoint();
  DLLFreeContext(&ctx, true);

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

  if (ctx->bytes_remaining) {
    return XBOX_S_OK;
  }

  return ReceiveImageDataComplete(process_context, response, response_len);
}

static HRESULT HandleInstall(const char *command, char *response,
                             DWORD response_len, struct CommandContext *ctx) {
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
  process_context->raw_image_size = length;
  process_context->receive_pointer = process_context->image_base;
  process_context->num_tls_callbacks = 0;
  process_context->tls_callbacks = NULL;
  process_context->relocation_needed = false;

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

static HRESULT HandleRegisterModuleExport(const char *command, char *response,
                                          DWORD response_len,
                                          struct CommandContext *ctx) {
  CommandParameters cp;
  int32_t result = ParseCommandParameters(command, &cp);
  if (result < 0) {
    return CPPrintError(result, response, response_len);
  }

  ModuleExport entry;

  const char *module_name;
  bool name_found = CPGetString("module", &module_name, &cp);
  const char *export_name;
  bool export_name_found = CPGetString("name", &export_name, &cp);
  bool ordinal_found = CPGetUInt32("ordinal", &entry.ordinal, &cp);
  bool address_found = CPGetUInt32("addr", &entry.address, &cp);

  if (!ordinal_found) {
    CPDelete(&cp);
    return SetXBDMError(XBOX_E_FAIL, "Missing required 'ordinal' param",
                        response, response_len);
  }
  if (!address_found) {
    CPDelete(&cp);
    return SetXBDMError(XBOX_E_FAIL, "Missing required 'address' param",
                        response, response_len);
  }

  if (!name_found) {
    CPDelete(&cp);
    return SetXBDMError(XBOX_E_FAIL, "Missing required 'module' param",
                        response, response_len);
  }

  char *module_name_saved = PoolStrdup(module_name, kTag);
  if (!module_name_saved) {
    CPDelete(&cp);
    return SetXBDMError(XBOX_E_ACCESS_DENIED, "Out of memory", response,
                        response_len);
  }

  entry.method_name = NULL;
  if (export_name_found) {
    entry.method_name = PoolStrdup(export_name, kTag);
    if (!entry.method_name) {
      DmFreePool(module_name_saved);
      CPDelete(&cp);
      return SetXBDMError(XBOX_E_ACCESS_DENIED, "Out of memory", response,
                          response_len);
    }
  }

  CPDelete(&cp);

  if (!MRRegisterMethod(module_name_saved, &entry)) {
    if (entry.method_name) {
      DmFreePool(entry.method_name);
    }

    DmFreePool(module_name_saved);
    return SetXBDMError(XBOX_E_FAIL, "Registration failed", response,
                        response_len);
  }

  // Note: The module registry now owns entry.method_name.

  DmFreePool(module_name_saved);
  return XBOX_S_OK;
}

static bool RegisterExport(const char *name, uint32_t ordinal,
                           uint32_t address) {
  // Keep in sync with name used in dynamic_dxt_loader.dll.def
  static const char kDynamicDXTLoaderDLLName[] = "dynamic_dxt_loader.dll";

  ModuleExport entry;
  entry.method_name = PoolStrdup(name, kTag);
  if (!entry.method_name) {
    return false;
  }
  entry.ordinal = ordinal;
  entry.address = address;
  return MRRegisterMethod(kDynamicDXTLoaderDLLName, &entry);
}
