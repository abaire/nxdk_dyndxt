#ifndef DYNDXT_LOADER_MODULE_REGISTRY_H
#define DYNDXT_LOADER_MODULE_REGISTRY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Captures information about a method exported from some module.
typedef struct ModuleExport {
  uint32_t ordinal;
  char *method_name;
  uint32_t address;
} ModuleExport;

// Opaque token used to enumerate registry content.
typedef struct ModuleRegistryCursor {
  void *module_;
  void *export_;
} ModuleRegistryCursor;

// Registers the given export for the given module.
// NOTE: The module info registry takes ownership of the string within the
// export, which must be DmPoolAllocate-allocated or scoped beyond the lifetime
// of the registry.
bool MRRegisterMethod(const char *module_name,
                      const ModuleExport *module_export);

// Returns the previously registered address for the given module + ordinal pair
// (e.g., "xbdm.dll", 30  should return the address of the
// DmRegisterCommandProcessor method).
bool MRGetMethodByOrdinal(const char *module_name, uint32_t ordinal,
                          uint32_t *result);

// Returns the previously registered address for the given module + export name
// pair (e.g., "xbdm.dll", DmRegisterCommandProcessor@8  should return the
// address of the DmRegisterCommandProcessor method).
bool MRGetMethodByName(const char *module_name, const char *name,
                       uint32_t *result);

// WARNING: These methods are intended to be called without any concurrent
// modification to the registry. Concurrent mutation may lead to incorrect data
// or crashes.
uint32_t MRGetNumRegisteredModules();
uint32_t MRGetTotalNumExports();
void MREnumerateRegistryBegin(ModuleRegistryCursor *cursor);
bool MREnumerateRegistry(const char **module_name,
                         const ModuleExport **module_export,
                         ModuleRegistryCursor *cursor);

void MRResetRegistry(void);

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // DYNDXT_LOADER_MODULE_REGISTRY_H
