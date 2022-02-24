#ifndef DYDXT_XBDM_H
#define DYDXT_XBDM_H

#include <windows.h>

#include "xbdm_err.h"

#define HRESULT_API HRESULT __attribute__((stdcall))
#define VOID_API void __attribute__((stdcall))
#define PVOID_API void *__attribute__((stdcall))

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
extern PVOID_API DmAllocatePoolWithTag(DWORD size, DWORD tag);

// Free the given block, which was previously allocated via
// DmAllocatePoolWithTag.
extern VOID_API DmFreePool(void *block);

#endif  // DYDXT_XBDM_H
