#ifndef XBDM_GDB_BRIDGE_XBDM_H
#define XBDM_GDB_BRIDGE_XBDM_H

#include <windows.h>

#define FACILITY_XBOX 0x2db
#define XBOX_S_OK (FACILITY_XBOX << 16)
#define XBOX_S_MULTILINE ((FACILITY_XBOX << 16) + 2)
#define XBOX_S_BINARY ((FACILITY_XBOX << 16) + 3)
#define XBOX_S_SEND_BINARY ((FACILITY_XBOX << 16) + 4)
#define XBOX_S_DEDICATED ((FACILITY_XBOX << 16) + 5)

#define XBOX_E_UNEXPECTED (0x80000000 | (FACILITY_XBOX << 16))
#define XBOX_E_MAX_CONNECTIONS_EXCEEDED (0x80000000 | (FACILITY_XBOX << 16) | 1)
#define XBOX_E_FILE_NOT_FOUND (0x80000000 | (FACILITY_XBOX << 16) | 2)
#define XBOX_E_NO_SUCH_MODULE (0x80000000 | (FACILITY_XBOX << 16) | 3)
#define XBOX_E_MEMORY_NOT_MAPPED (0x80000000 | (FACILITY_XBOX << 16) | 4)
#define XBOX_E_NO_SUCH_THREAD (0x80000000 | (FACILITY_XBOX << 16) | 5)
#define XBOX_E_UNKNOWN_COMMAND (0x80000000 | (FACILITY_XBOX << 16) | 7)
#define XBOX_E_NOT_STOPPED (0x80000000 | (FACILITY_XBOX << 16) | 8)
#define XBOX_E_FILE_MUST_BE_COPIED (0x80000000 | (FACILITY_XBOX << 16) | 9)
#define XBOX_E_EXISTS (0x80000000 | (FACILITY_XBOX << 16) | 10)
#define XBOX_E_DIRECTORY_NOT_EMPTY (0x80000000 | (FACILITY_XBOX << 16) | 11)
#define XBOX_E_FILENAME_INVALID (0x80000000 | (FACILITY_XBOX << 16) | 12)
#define XBOX_E_CREATE_FILE_FAILED (0x80000000 | (FACILITY_XBOX << 16) | 13)
#define XBOX_E_ACCESS_DENIED (0x80000000 | (FACILITY_XBOX << 16) | 14)
#define XBOX_E_TYPE_INVALID (0x80000000 | (FACILITY_XBOX << 16) | 17)
#define XBOX_E_DATA_NOT_AVAILABLE (0x80000000 | (FACILITY_XBOX << 16) | 18)
#define XBOX_E_DEDICATED_CONNECTION_REQUIRED (0x80000000 | (FACILITY_XBOX << 16) | 22)

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
