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
#define HANDLE_API HANDLE __attribute__((stdcall))
#else
#define HRESULT_API HRESULT
#define VOID_API void
#define PVOID_API void *
#define X_API_PTR(ret, n) ret(*(n))
#define HANDLE_API HANDLE
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

//! Contains contextual information used when a debug command processor needs to
//! do something more than immediately reply with a simple, short response.
typedef struct CommandContext {
  //! Function to be invoked to actually send or receive data. This function
  //! will be called repeatedly until `bytes_remaining` is set to 0.
  ContinuationProc handler;
  //! When receiving data, this will be set to the number of bytes in `buffer`
  //! that contain valid received data. When sending data, this must be set by
  //! the debug processor to the number of valid bytes in `buffer` that should
  //! be sent.
  DWORD data_size;
  //! Buffer used to hold send/receive data. XBDM will create a small buffer by
  //! default, this can be reassigned to an arbitrary buffer allocated by the
  //! debug processor, provided it is cleaned up.
  void *buffer;
  //! The size of `buffer` in bytes.
  DWORD buffer_size;
  //! Arbitrary data defined by the command processor.
  void *user_data;
  //! Used when sending a chunked response, indicates the number of bytes
  //! remaining to be sent. `handler` will be called repeatedly until this is
  //! set to 0, indicating completion of the send.
  DWORD bytes_remaining;
} __attribute__((packed)) CommandContext;

// Register a new processor for commands with the given prefix.
extern HRESULT_API DmRegisterCommandProcessor(const char *prefix,
                                              ProcessorProc proc);

//! A function that may be invoked by DmRegisterCommandProcessorEx to create a
//! dedicated handler thread.
typedef HANDLE_API (*CreateThreadFunc)(LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                       SIZE_T dwStackSize,
                                       LPTHREAD_START_ROUTINE lpStartAddress,
                                       LPVOID lpParameter,
                                       DWORD dwCreationFlags,
                                       LPDWORD lpThreadId);

// Register a new processor for commands with the given prefix, running in a
// dedicated thread created with the given `create_thread_func`.
extern HRESULT_API DmRegisterCommandProcessorEx(
    const char *prefix, ProcessorProc proc,
    CreateThreadFunc create_thread_func);

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
