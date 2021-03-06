#ifndef DYDXT_XBDM_H
#define DYDXT_XBDM_H

#include <windows.h>

#include "xbdm_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Helper macros to hide the attribute verbosity
#ifdef _WIN32
#define HRESULT_API HRESULT __attribute__((stdcall))
#define VOID_API void __attribute__((stdcall))
#define PVOID_API void *__attribute__((stdcall))
#define X_API_PTR(ret, n) ret(__attribute__((stdcall)) * (n))
#else
#define HRESULT_API HRESULT
#define VOID_API void
#define PVOID_API void *
#define X_API_PTR(ret, n) ret(*(n))
#endif  // #ifdef _WIN32

#define HRESULT_API_PTR(n) X_API_PTR(HRESULT, (n))
#define PVOID_API_PTR(n) X_API_PTR(void *, (n))
#define VOID_API_PTR(n) X_API_PTR(void, (n))

struct CommandContext;

// Main processor procedure.
typedef HRESULT_API_PTR(ProcessorProc)(const char *command, char *response,
                                       DWORD response_len,
                                       struct CommandContext *ctx);

// Function handler.
typedef HRESULT_API_PTR(ContinuationProc)(struct CommandContext *ctx,
                                          char *response, DWORD response_len);

typedef struct CommandContext {
  ContinuationProc handler;
  DWORD data_size;
  void *buffer;
  DWORD buffer_size;
  void *user_data;
  DWORD bytes_remaining;
} __attribute__((packed)) CommandContext;

// Register a new processor for commands with the given prefix.
extern HRESULT_API DmRegisterCommandProcessor(const char *prefix,
                                              ProcessorProc proc);

// Allocate a new block of memory with the given tag.
extern PVOID_API DmAllocatePoolWithTag(DWORD size, DWORD tag);

// Free the given block, which was previously allocated via
// DmAllocatePoolWithTag.
extern VOID_API DmFreePool(void *block);

// Send a message to the notification channel.
extern HRESULT_API DmSendNotificationString(const char *message);

typedef PVOID PDM_WALK_MODULES;
typedef struct DMN_MODLOAD {
  char name[260];
  void *base;
  DWORD size;
  DWORD timestamp;
  DWORD checksum;
  DWORD unknown_flags;
} DMN_MODLOAD;

extern HRESULT_API DmWalkLoadedModules(PDM_WALK_MODULES *ppdmwm,
                                       DMN_MODLOAD *pdmml);
extern HRESULT_API DmCloseLoadedModules(PDM_WALK_MODULES pdmwm);

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // DYDXT_XBDM_H
