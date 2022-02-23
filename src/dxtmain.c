#include <string.h>
#include <windows.h>

#include "xbdm.h"

//// Reserve space for function pointers, to be populated by the L2 loader.
// void *kImportTable[BOOTSTRAP_L2_IMPORT_TABLE_SIZE]
//     __attribute__((section(".import_table_data"))) = {0};

// Put the entrypoint function in a predictable location.
extern HRESULT DxtMain(void) __attribute__((section(".dxtmain")));

// Command that will be handled by this processor.
static const char kHandlerName[] = "bl2";

static HRESULT_API process_command(const char *command, char *response,
                                   DWORD response_len,
                                   struct CommandContext *ctx);
static HRESULT_API handle_install(const char *command, char *response,
                                  DWORD response_len,
                                  struct CommandContext *ctx);

HRESULT DxtMain(void) {
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
  if (!strncmp(subcommand, "install", 7)) {
    return handle_install(command, response, response_len, ctx);
  }

  return XBOX_S_OK;
}

static HRESULT_API handle_install(const char *command, char *response,
                                  DWORD response_len,
                                  struct CommandContext *ctx) {
  strncpy(response, "OK?!?", response_len);
  return XBOX_S_OK;
}