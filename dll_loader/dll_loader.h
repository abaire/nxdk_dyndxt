#ifndef DYNDXT_LOADER_DLL_LOADER_H
#define DYNDXT_LOADER_DLL_LOADER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "winapi/winnt.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define DLL_LOADER_API __attribute__((stdcall))
#else
#define DLL_LOADER_API
#endif  // #ifdef _WIN32

// Maximum size of an error message string.
// WARNING: This must always be at least sizeof("TOOSMALL") to allow the loader
// to report that the buffer is insufficient to hold the real error message.
#define DLLL_MAX_ERROR_LEN 256

typedef uint32_t hwaddress_t;

typedef enum DLLLoaderContext {
  DLLL_NOT_PARSED = 0,
  DLLL_DOS_HEADER = 1,
  DLLL_NT_HEADER = 2,
  DLLL_NT_HEADER_SECTION_TABLE = 3,
  DLLL_LOAD_IMAGE = 4,
  DLLL_LOAD_SECTION = 5,
  DLLL_RESOLVE_IMPORTS = 6,
  DLLL_RELOCATE = 7,
  DLLL_INVOKE_TLS_CALLBACKS = 8,
} DLLLoaderContext;

typedef enum DLLLoaderStatus {
  DLLL_OK = 0,

  // A general error occurred.
  DLLL_ERROR = 1,

  // An attempt was made to read beyond the size of the raw data, likely
  // indicating that the data is invalid.
  DLLL_FILE_TOO_SMALL = 2,

  // A signature in the raw data did not match the expected value, likely
  // indicating that the data is invalid.
  DLLL_INVALID_SIGNATURE = 3,

  // The Machine target is not i386.
  DLLL_WRONG_MACHINE_TYPE = 4,

  // The DLL was not built with a dynamic base address.
  DLLL_FIXED_BASE = 5,

  // Memory allocation failed.
  DLLL_OUT_OF_MEMORY = 6,

  // One of the imports uses DLL forwarding.
  DLLL_DLL_FORWARDING_NOT_SUPPORTED = 7,

  // An import could not be resolved to a function address.
  DLLL_UNRESOLVED_IMPORT = 8,

  // No relocation data is available in the DLL.
  DLLL_NO_RELOCATION_DATA = 9,

  // A relocation entry used an unimplemented type.
  DLLL_UNSUPPORTED_RELOCATION_TYPE = 10,
} DLLLoaderStatus;

// The caller is responsible for setting up and cleaning up these values.
typedef struct DLLLoaderInput {
  // Pointer to the raw DLL file data.
  const void *raw_data;
  // Size of the raw data in bytes.
  uint32_t raw_data_size;

  // Pointer to a method used to allocate memory.
  void *(DLL_LOADER_API *alloc)(size_t size);

  // Pointer to a method used to free memory.
  void(DLL_LOADER_API *free)(void *ptr);

  // Pointer to a method used to look up the address of a function by ordinal.
  // `image` - the name of the image (e.g., "xbdm.dll")
  // `ordinal` - the export ordinal number
  // `result` - [OUT] set to the address of the requested function
  //  Returns true if the lookup was successful, false if not.
  bool(DLL_LOADER_API *resolve_import_by_ordinal)(const char *image,
                                                  uint32_t ordinal,
                                                  uint32_t *result);

  // Pointer to a method used to look up the address of a function by name.
  // `image` - the name of the image (e.g., "xbdm.dll")
  // `name` - the name of the export (e.g., "DmFreePool@4")
  // `result` - [OUT] set to the address of the requested function
  //  Returns true if the lookup was successful, false if not.
  bool(DLL_LOADER_API *resolve_import_by_name)(const char *image,
                                               const char *name,
                                               uint32_t *result);
} DLLLoaderInput;

typedef struct DLLLoaderOutput {
  IMAGE_NT_HEADERS32 header;
  IMAGE_SECTION_HEADER *section_headers;

  // The loaded DLL image.
  uint8_t *image;

  // The rebased entrypoint of the DLL.
  hwaddress_t entrypoint;

  DLLLoaderContext context;
  DLLLoaderStatus status;
  char error_message[DLLL_MAX_ERROR_LEN];
} DLLLoaderOutput;

typedef struct DLLContext {
  DLLLoaderInput input;
  DLLLoaderOutput output;
} DLLContext;

// Load the DLL into newly allocated memory, relocating to work properly
// in-place.
bool DLLLoad(DLLContext *ctx);

// Load the DLL into newly allocated memory and resolve imports but do not
// relocate it.
bool DLLParse(DLLContext *ctx);

// Update the image to be loaded at the given address.
bool DLLRelocate(DLLContext *ctx, hwaddress_t base_address);

bool DLLInvokeTLSCallbacks(DLLContext *ctx);

// Frees resources owned by the given DLLContext instance.
// If `keep_image` is true, the loaded DLL image is kept intact. It is up to the
// caller to preserve and free the image if it has been allocated).
void DLLFreeContext(DLLContext *ctx, bool keep_image);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // DYNDXT_LOADER_DLL_LOADER_H
