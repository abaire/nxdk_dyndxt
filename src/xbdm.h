#ifndef XBDM_GDB_BRIDGE_XBDM_H
#define XBDM_GDB_BRIDGE_XBDM_H

#include <windows.h>

#define FACILITY_XBOX 0x2db
#define XBOX_S_OK (FACILITY_XBOX << 16)
#define XBOX_S_MULTILINE ((FACILITY_XBOX << 16) + 2)
#define XBOX_S_BINARY ((FACILITY_XBOX << 16) + 3)
#define XBOX_S_SEND_BINARY ((FACILITY_XBOX << 16) + 4)
#define XBOX_S_DEDICATED ((FACILITY_XBOX << 16) + 5)

#define HRESULT_API HRESULT __attribute__((stdcall))

// Helper macros to hide the attribute verbosity
#define X_API_PTR(ret, n) ret(__attribute__((stdcall)) * (n))

#define HRESULT_API_PTR(n) X_API_PTR(HRESULT, (n))
#define PVOID_API_PTR(n) X_API_PTR(void *, (n))
#define VOID_API_PTR(n) X_API_PTR(void, (n))

struct CommandContext;

// Main processor procedure.
typedef HRESULT_API_PTR(ProcessorProc)(const char *command, char *response,
                                       DWORD response_len,
                                       struct CommandContext *ctx);

// Function handler.
typedef HRESULT_API_PTR(CommandHandlerFunc)(struct CommandContext *ctx,
                                            char *response, DWORD response_len);

typedef struct CommandContext {
  CommandHandlerFunc handler;
  DWORD data_size;
  void *buffer;
  DWORD buffer_size;
  void *user_data;
  DWORD bytes_remaining;
} CommandContext;

// Register a new processor for commands with the given prefix.
extern HRESULT_API DmRegisterCommandProcessor(const char *prefix,
                                              ProcessorProc proc);

// Allocate a new block of memory with the given tag.
typedef PVOID_API_PTR(DmAllocatePoolWithTag)(DWORD size, DWORD tag);

// Free the given block, which was previously allocated via
// DmAllocatePoolWithTag.
typedef VOID_API_PTR(DmFreePool)(void *block);

#endif  // XBDM_GDB_BRIDGE_XBDM_H
