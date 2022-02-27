#ifndef DYNDXT_LOADER_MODULE_REGISTRY_H
#define DYNDXT_LOADER_MODULE_REGISTRY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define MR_API __attribute__((stdcall))
#else
#define MR_API
#endif  // #ifdef _WIN32

// Captures information about a method exported from some module.
typedef struct ModuleExport {
  uint32_t ordinal;
  char *method_name;
  char *alias;  // Optional alias for method_name.
  uint32_t address;
} ModuleExport;

// Opaque token used to enumerate registry content.
typedef struct ModuleRegistryCursor {
  void *module_;
  void *export_;
} ModuleRegistryCursor;

// Registers the given export for the given module.
// NOTE: The module info registry takes ownership of the strings within the
// export, which must be DmPoolAllocate-allocated or scoped beyond the lifetime
// of the registry.
bool MR_API MRRegisterMethod(const char *module_name,
                             const ModuleExport *module_export);

// Returns the previously registered address for the given module + ordinal pair
// (e.g., "xbdm.dll", 30  should return the address of the
// DmRegisterCommandProcessor method).
bool MR_API MRGetMethodByOrdinal(const char *module_name, uint32_t ordinal,
                                 uint32_t *result);

// Returns the previously registered address for the given module + export name
// pair (e.g., "xbdm.dll", DmRegisterCommandProcessor@8  should return the
// address of the DmRegisterCommandProcessor method).
bool MR_API MRGetMethodByName(const char *module_name, const char *name,
                              uint32_t *result);

// WARNING: These methods are intended to be called without any concurrent
// modification to the registry. Concurrent mutation may lead to incorrect data
// or crashes.
uint32_t MR_API MRGetNumRegisteredModules(void);
uint32_t MR_API MRGetTotalNumExports(void);
void MR_API MREnumerateRegistryBegin(ModuleRegistryCursor *cursor);
bool MR_API MREnumerateRegistry(const char **module_name,
                                const ModuleExport **module_export,
                                ModuleRegistryCursor *cursor);

void MR_API MRResetRegistry(void);

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // DYNDXT_LOADER_MODULE_REGISTRY_H
