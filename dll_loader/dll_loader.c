#include "dll_loader.h"

#include <string.h>

#define SET_ERROR_CONTEXT(context_ptr, value) \
  (context_ptr)->output.context = (value)

#define SET_ERROR_STATUS(context_ptr, value) \
  (context_ptr)->output.status = (value)

#define SETUP_READ_PTR(context_ptr)                     \
  const void *read_ptr = (context_ptr)->input.raw_data; \
  const void *eof = read_ptr + (context_ptr)->input.raw_data_size

#define ADVANCE_READ_PTR(bytes)                 \
  do {                                          \
    read_ptr += (bytes);                        \
    if (read_ptr > eof) {                       \
      ctx->output.status = DLLL_FILE_TOO_SMALL; \
      return false;                             \
    }                                           \
  } while (0)

// TODO: Fully support process and thread TLS invocations.
// See: https://docs.microsoft.com/en-us/windows/win32/dlls/dllmain
#define DLL_PROCESS_DETACH 0
#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH 2
#define DLL_THREAD_DETACH 3

#ifdef _WIN32
typedef void (*TLSCallback)(LPVOID h, DWORD dwReason, PVOID pv)
    __attribute__((__stdcall__));
#endif

typedef struct IMAGE_BASE_RELOCATION {
  DWORD VirtualAddress;
  DWORD SizeOfBlock;
} IMAGE_BASE_RELOCATION;

typedef struct IMAGE_IMPORT_BY_NAME {
  USHORT Hint;
  UCHAR Name[1];
} IMAGE_IMPORT_BY_NAME;

// From https://docs.microsoft.com/en-us/windows/win32/debug/pe-format
#define IMAGE_REL_BASED_ABSOLUTE 0
#define IMAGE_REL_BASED_HIGHLOW 3

#define IMAGE_SNAP_BY_ORDINAL(ordinal) (((ordinal)&0x80000000) != 0)

static bool DLLParseHeader(DLLContext *ctx);
static bool DLLLoadImage(DLLContext *ctx);
static bool DLLProcessSections(DLLContext *ctx);
static bool DLLResolveImports(DLLContext *ctx);
static bool DLLRelocate(DLLContext *ctx);

bool DLLLoad(DLLContext *ctx) {
  SET_ERROR_CONTEXT(ctx, DLLL_NOT_PARSED);
  memset(&ctx->output, 0, sizeof(ctx->output));

  if (!DLLParseHeader(ctx)) {
    DLLFreeContext(ctx, false);
    return false;
  }

  if (!DLLLoadImage(ctx)) {
    DLLFreeContext(ctx, false);
    return false;
  }

  if (!DLLProcessSections(ctx)) {
    DLLFreeContext(ctx, false);
    return false;
  }

  if (!DLLResolveImports(ctx)) {
    DLLFreeContext(ctx, false);
    return false;
  }

  if (!DLLRelocate(ctx)) {
    DLLFreeContext(ctx, false);
    return false;
  }

  ctx->output.entrypoint =
      ctx->output.header.OptionalHeader.ImageBase +
      ctx->output.header.OptionalHeader.AddressOfEntryPoint;

  return true;
}

bool DLLInvokeTLSCallbacks(DLLContext *ctx) {
  SET_ERROR_CONTEXT(ctx, DLLL_INVOKE_TLS_CALLBACKS);

  const IMAGE_DATA_DIRECTORY *directory =
      ctx->output.header.OptionalHeader.DataDirectory +
      IMAGE_DIRECTORY_ENTRY_TLS;
  if (!directory->Size) {
    return true;
  }

#ifdef _WIN32
  // TODO: Verify this behavior and rebase if necessary.
  const IMAGE_TLS_DIRECTORY_32 *init =
      (const IMAGE_TLS_DIRECTORY_32 *)(ctx->output.image +
                                       directory->VirtualAddress);

  TLSCallback callback = (TLSCallback)init->AddressOfCallBacks;
  while (callback && *callback) {
    callback(ctx->output.image, DLL_PROCESS_ATTACH, NULL);
  }
#endif

  return true;
}

void DLLFreeContext(DLLContext *ctx, bool keep_image) {
  DLLLoaderInput *i = &ctx->input;
  DLLLoaderOutput *o = &ctx->output;
  if (o->section_headers) {
    i->free(o->section_headers);
    o->section_headers = NULL;
  }

  if (!keep_image && o->image) {
    i->free(o->image);
    o->image = NULL;
  }
}

static bool DLLParseHeader(DLLContext *ctx) {
  SETUP_READ_PTR(ctx);

  SET_ERROR_CONTEXT(ctx, DLLL_DOS_HEADER);
  IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *)read_ptr;
  ADVANCE_READ_PTR(sizeof(IMAGE_DOS_HEADER));

  if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
    SET_ERROR_STATUS(ctx, DLLL_INVALID_SIGNATURE);
    return false;
  }

  if (dos_header->e_lfanew > sizeof(*dos_header)) {
    ADVANCE_READ_PTR(dos_header->e_lfanew - sizeof(*dos_header));
  }

  SET_ERROR_CONTEXT(ctx, DLLL_NT_HEADER);
  const IMAGE_NT_HEADERS32 *nt_header = (const IMAGE_NT_HEADERS32 *)read_ptr;
  ADVANCE_READ_PTR(sizeof(IMAGE_NT_HEADERS32));

  memcpy(&ctx->output.header, nt_header, sizeof(ctx->output.header));

  if (ctx->output.header.Signature != IMAGE_NT_SIGNATURE) {
    SET_ERROR_STATUS(ctx, DLLL_INVALID_SIGNATURE);
    return false;
  }

  if (ctx->output.header.FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
    SET_ERROR_STATUS(ctx, DLLL_WRONG_MACHINE_TYPE);
    return false;
  }

  if (!(ctx->output.header.OptionalHeader.DllCharacteristics &
        IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE)) {
    SET_ERROR_STATUS(ctx, DLLL_FIXED_BASE);
    return false;
  }

  if (ctx->output.header.FileHeader.SizeOfOptionalHeader >
      sizeof(ctx->output.header.OptionalHeader)) {
    ADVANCE_READ_PTR(ctx->output.header.FileHeader.SizeOfOptionalHeader -
                     sizeof(ctx->output.header.OptionalHeader));
  }

  SET_ERROR_CONTEXT(ctx, DLLL_NT_HEADER_SECTION_TABLE);
  uint32_t num_sections = ctx->output.header.FileHeader.NumberOfSections;
  size_t section_header_size =
      num_sections * sizeof(*ctx->output.section_headers);
  ctx->output.section_headers = ctx->input.alloc(section_header_size);
  if (!ctx->output.section_headers) {
    SET_ERROR_STATUS(ctx, DLLL_OUT_OF_MEMORY);
    return false;
  }

  const IMAGE_SECTION_HEADER *section_header_base =
      (const IMAGE_SECTION_HEADER *)read_ptr;
  ADVANCE_READ_PTR(section_header_size);
  memcpy(ctx->output.section_headers, section_header_base, section_header_size);

  return true;
}

static bool DLLLoadImage(DLLContext *ctx) {
  SET_ERROR_CONTEXT(ctx, DLLL_LOAD_IMAGE);
  ctx->output.image =
      ctx->input.alloc(ctx->output.header.OptionalHeader.SizeOfImage);
  if (!ctx->output.image) {
    SET_ERROR_STATUS(ctx, DLLL_OUT_OF_MEMORY);
    return false;
  }

  if (ctx->output.header.OptionalHeader.SizeOfHeaders >
      ctx->output.header.OptionalHeader.SizeOfImage) {
    SET_ERROR_STATUS(ctx, DLLL_ERROR);
    return false;
  }

  memcpy(ctx->output.image, ctx->input.raw_data,
         ctx->output.header.OptionalHeader.SizeOfHeaders);
  return true;
}

static bool ProcessSection(const IMAGE_SECTION_HEADER *header,
                           DLLContext *ctx) {
  SETUP_READ_PTR(ctx);
  SET_ERROR_CONTEXT(ctx, DLLL_LOAD_SECTION);

  if (!header->SizeOfRawData) {
    // The section likely defines uninitialized data and can be skipped. The
    // image already contains reserved space.
    return true;
  }

  char name_buf[16] = {0};
  memcpy(name_buf, header->Name, sizeof(header->Name));

  ADVANCE_READ_PTR(header->PointerToRawData);
  const void *section_start = read_ptr;
  ADVANCE_READ_PTR(header->SizeOfRawData);

  memcpy(ctx->output.image + header->VirtualAddress, section_start,
         header->SizeOfRawData);

  return true;
}

static bool DLLProcessSections(DLLContext *ctx) {
  for (uint32_t i = 0; i < ctx->output.header.FileHeader.NumberOfSections;
       ++i) {
    if (!ProcessSection(&ctx->output.section_headers[i], ctx)) {
      return false;
    }
  }
  return true;
}

static bool DLLResolveImports(DLLContext *ctx) {
  SET_ERROR_CONTEXT(ctx, DLLL_RESOLVE_IMPORTS);

  const IMAGE_DATA_DIRECTORY *directory =
      ctx->output.header.OptionalHeader.DataDirectory +
      IMAGE_DIRECTORY_ENTRY_IMPORT;
  if (!directory->Size) {
    return true;
  }

  void *descriptor_start = ctx->output.image + directory->VirtualAddress;
  const IMAGE_IMPORT_DESCRIPTOR *descriptor =
      (const IMAGE_IMPORT_DESCRIPTOR *)descriptor_start;

  // TODO: Prevent reads beyond end of table.
  for (; descriptor->Name; ++descriptor) {
    const char *image_name = ctx->output.image + descriptor->Name;

    if (descriptor->ForwarderChain) {
      SET_ERROR_STATUS(ctx, DLLL_DLL_FORWARDING_NOT_SUPPORTED);
      return false;
    }

    uint32_t *thunk;
    uint32_t *function =
        (uint32_t *)(ctx->output.image + descriptor->FirstThunk);
    if (descriptor->DUMMYUNIONNAME.OriginalFirstThunk) {
      thunk = (uint32_t *)(ctx->output.image +
                           descriptor->DUMMYUNIONNAME.OriginalFirstThunk);
    } else {
      thunk = function;
    }

    for (; *thunk; ++thunk, ++function) {
      if (IMAGE_SNAP_BY_ORDINAL(*thunk)) {
        uint32_t ordinal = *thunk & 0xFFFF;
        if (!ctx->input.resolve_import_by_ordinal(image_name, ordinal,
                                                  function)) {
          SET_ERROR_STATUS(ctx, DLLL_UNRESOLVED_IMPORT);
          return false;
        }
      } else {
        const IMAGE_IMPORT_BY_NAME *name_data =
            (const IMAGE_IMPORT_BY_NAME *)(ctx->output.image + *thunk);
        const char *import_name = (const char *)(name_data->Name);
        if (!ctx->input.resolve_import_by_name(image_name, import_name,
                                               function)) {
          SET_ERROR_STATUS(ctx, DLLL_UNRESOLVED_IMPORT);
          return false;
        }
      }
    }
  }

  return true;
}

static bool DLLRelocate(DLLContext *ctx) {
  SET_ERROR_CONTEXT(ctx, DLLL_RELOCATE);

  intptr_t header_image_base = ctx->output.header.OptionalHeader.ImageBase;
  if ((intptr_t)ctx->output.image == header_image_base) {
    return true;
  }

  int32_t image_delta = 0;
  if ((intptr_t)ctx->output.image > header_image_base) {
    image_delta = (int32_t)((intptr_t)ctx->output.image - header_image_base);
  } else {
    image_delta = -1 * (header_image_base - (intptr_t)ctx->output.image);
  }

  const IMAGE_DATA_DIRECTORY *directory =
      ctx->output.header.OptionalHeader.DataDirectory +
      IMAGE_DIRECTORY_ENTRY_BASERELOC;
  if (!directory->Size) {
    // TODO: Verify that this means relocation data is missing.
    // Does this fail erroneously for images that need no relocations?
    SET_ERROR_STATUS(ctx, DLLL_NO_RELOCATION_DATA);
    return false;
  }

  void *relocation_start = ctx->output.image + directory->VirtualAddress;
  IMAGE_BASE_RELOCATION *block = (IMAGE_BASE_RELOCATION *)(relocation_start);

  // TODO: Prevent reads beyond end of table.
  while (block->VirtualAddress > 0) {
    void *dest = ctx->output.image + block->VirtualAddress;
    uint16_t *entry = (uint16_t *)(relocation_start + sizeof(relocation_start));
    void *relocation_end = relocation_start + block->SizeOfBlock;

    while ((void *)entry < relocation_end) {
      uint32_t type = *entry >> 12;
      uint32_t rva_offset = *entry & 0x0FFF;
      ++entry;

      switch (type) {
        case IMAGE_REL_BASED_ABSOLUTE:
          // Skip padding entries.
          continue;

        case IMAGE_REL_BASED_HIGHLOW: {
          uint32_t *target = (uint32_t *)(dest + rva_offset);
          *target += image_delta;
          break;
        }

        default:
          SET_ERROR_STATUS(ctx, DLLL_UNSUPPORTED_RELOCATION_TYPE);
          return false;
      }
    }

    relocation_start = relocation_end;
    block = (IMAGE_BASE_RELOCATION *)(relocation_start);
  }

  ctx->output.header.OptionalHeader.ImageBase = (intptr_t)ctx->output.image;
  return true;
}
