#include "link_loaded_modules.h"

#include "module_registry.h"
#include "util.h"
#include "winapi/winnt.h"
#include "xbdm.h"

// static const uint32_t kTag = 0x64786C6C;  // 'dxll'

static HRESULT RegisterModuleExports(const char *module_name,
                                     void *module_base) {
  const uint8_t *image = (const uint8_t *)module_base;
  const uint8_t *read_ptr = image;

  IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *)read_ptr;
  if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
    DbgPrint("Bad DOS header for %s at 0x%x\n", module_name, module_base);
    return XBOX_E_TYPE_INVALID;
  }

  read_ptr += dos_header->e_lfanew;

  const IMAGE_NT_HEADERS32 *nt_header = (const IMAGE_NT_HEADERS32 *)read_ptr;
  if (nt_header->Signature != IMAGE_NT_SIGNATURE) {
    DbgPrint("Bad NT header for %s at 0x%x\n", module_name, module_base);
    return XBOX_E_TYPE_INVALID;
  }

  const IMAGE_DATA_DIRECTORY *directory_info =
      (const IMAGE_DATA_DIRECTORY *)nt_header->OptionalHeader.DataDirectory +
      IMAGE_DIRECTORY_ENTRY_EXPORT;
  if (!directory_info->Size) {
    return XBOX_S_OK;
  }

  const IMAGE_EXPORT_DIRECTORY *directory =
      (const IMAGE_EXPORT_DIRECTORY *)(image + directory_info->VirtualAddress);

  uint32_t *function_array =
      (uint32_t *)(image + directory->AddressOfFunctions);
  ModuleExport export = {0};
  for (uint32_t i = 0; i < directory->NumberOfFunctions; ++i) {
    export.ordinal = i + directory->Base;

    // TODO: Handle forwarded exports.
    export.address = function_array[export.ordinal - directory->Base] +
                     (intptr_t)module_base;

    if (!MRRegisterMethod(module_name, &export)) {
      DbgPrint("Failed to register %s @ %d (%s)\n", module_name, export.ordinal,
               export.method_name ? export.method_name : "");
      return XBOX_E_FAIL;
    }
  }

  // TODO: Consider adding support for names as well.
  // The most kernel and xbdm images do not have any names.

  return XBOX_S_OK;
}

HRESULT LinkLoadedModules(void) {
  PDM_WALK_MODULES token = NULL;
  DMN_MODLOAD module_info;

  HRESULT ret = XBOX_S_OK;
  while (DmWalkLoadedModules(&token, &module_info) == XBOX_S_OK &&
         ret == XBOX_S_OK) {
    ret = RegisterModuleExports(module_info.name, module_info.base);
  }
  DmCloseLoadedModules(token);

  return ret;
}
