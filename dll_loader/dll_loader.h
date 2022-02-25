#ifndef DYNDXT_LOADER_DLL_LOADER_H
#define DYNDXT_LOADER_DLL_LOADER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <winapi/winnt.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DLLLoaderContext {
  DLLL_NOT_PARSED = 0,
  DLLL_DOS_HEADER,
  DLLL_NT_HEADER,
  DLLL_NT_HEADER_SECTION_TABLE,
  DLLL_LOAD_IMAGE,
  DLLL_LOAD_SECTION,
  DLLL_RESOLVE_IMPORTS,
  DLLL_RELOCATE,
  DLLL_INVOKE_TLS_CALLBACKS,
} DLLLoaderContext;

typedef enum DLLLoaderStatus {
  DLLL_OK = 0,

  // A general error occurred.
  DLLL_ERROR,

  // An attempt was made to read beyond the size of the raw data, likely
  // indicating that the data is invalid.
  DLLL_FILE_TOO_SMALL,

  // A signature in the raw data did not match the expected value, likely
  // indicating that the data is invalid.
  DLLL_INVALID_SIGNATURE,

  // The Machine target is not i386.
  DLLL_WRONG_MACHINE_TYPE,

  // The DLL was not built with a dynamic base address.
  DLLL_FIXED_BASE,

  // Memory allocation failed.
  DLLL_OUT_OF_MEMORY,

  // One of the imports uses DLL forwarding.
  DLLL_DLL_FORWARDING_NOT_SUPPORTED,

  // An import could not be resolved to a function address.
  DLLL_UNRESOLVED_IMPORT,

  // No relocation data is available in the DLL.
  DLLL_NO_RELOCATION_DATA,

  // A relocation entry used an unimplemented type.
  DLLL_UNSUPPORTED_RELOCATION_TYPE,
} DLLLoaderStatus;

// The caller is responsible for setting up and cleaning up these values.
typedef struct DLLLoaderInput {
  // Pointer to the raw DLL file data.
  const void *raw_data;
  // Size of the raw data in bytes.
  uint32_t raw_data_size;

  // Pointer to a method used to allocate memory.
  void *(*alloc)(size_t size);

  // Pointer to a method used to free memory.
  void (*free)(void *ptr);

  // Pointer to a method used to look up the address of a function by ordinal.
  // `image` - the name of the image (e.g., "xbdm.dll")
  // `ordinal` - the export ordinal number
  // `result` - [OUT] set to the address of the requested function
  //  Returns true if the lookup was successful, false if not.
  bool (*resolve_import_by_ordinal)(const char *image, uint32_t ordinal,
                                    uint32_t *result);

  // Pointer to a method used to look up the address of a function by name.
  // `image` - the name of the image (e.g., "xbdm.dll")
  // `name` - the name of the export (e.g., "DmFreePool@4")
  // `result` - [OUT] set to the address of the requested function
  //  Returns true if the lookup was successful, false if not.
  bool (*resolve_import_by_name)(const char *image, const char *name,
                                 uint32_t *result);
} DLLLoaderInput;

typedef struct DLLLoaderOutput {
  IMAGE_NT_HEADERS32 header;
  IMAGE_SECTION_HEADER *section_headers;

  // The loaded DLL image.
  void *image;

  // The rebased entrypoint of the DLL.
  void *entrypoint;

  DLLLoaderContext context;
  DLLLoaderStatus status;
} DLLLoaderOutput;

typedef struct DLLContext {
  DLLLoaderInput input;
  DLLLoaderOutput output;
} DLLContext;

bool DLLLoad(DLLContext *ctx);

bool DLLInvokeTLSCallbacks(DLLContext *ctx);

// Frees resources owned by the given DLLContext instance, optionally leaving
// the loaded DLL image intact (it is up to the caller to preserve and free the
// image if `keep_image` is true).
void DLLFreeContext(DLLContext *ctx, bool keep_image);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // DYNDXT_LOADER_DLL_LOADER_H
