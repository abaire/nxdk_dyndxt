#include <windows.h>

#include "xbdm.h"

//// Reserve space for function pointers, to be populated by the L2 loader.
// void *kImportTable[BOOTSTRAP_L2_IMPORT_TABLE_SIZE]
//     __attribute__((section(".import_table_data"))) = {0};

// Put the entrypoint function in a predictable location.
extern HRESULT entrypoint(void) __attribute__((section(".entry_point")));

// Command that will be handled by this processor.
static const char kHandlerName[] = "bl2";

static HRESULT_API process_command(const char *command, char *response,
                                   DWORD response_len,
                                   struct CommandContext *ctx);

HRESULT entrypoint(void) {
  return DmRegisterCommandProcessor(kHandlerName, process_command);
}

static HRESULT_API process_command(const char *command, char *response,
                                   DWORD response_len,
                                   struct CommandContext *ctx) {
  return XBOX_S_OK;
}
