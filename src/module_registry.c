#include "module_registry.h"

#include <stddef.h>
#include <string.h>

#include "util.h"
#include "xbdm.h"

// ddxt module registry
#define TAG 'rmxd'

struct ExportNode;
typedef struct ExportNode {
  struct ExportNode *next;
  ModuleExport entry;
} ExportNode;

// Captures information about methods exported from a particular module.
struct ModuleExportTable;
typedef struct ModuleExportTable {
  struct ModuleExportTable *next;
  char *module_name;
  ExportNode *exports;
} ModuleExportTable;

static ModuleExportTable *export_table = NULL;

static bool AppendExport(ModuleExportTable *table,
                         const ModuleExport *module_export);
static bool AppendExportTable(const char *module_name,
                              const ModuleExport *module_export);

bool MRRegisterMethod(const char *module_name,
                      const ModuleExport *module_export) {
  ModuleExportTable *e = export_table;
  while (e) {
    if (!strcmp(e->module_name, module_name)) {
      return AppendExport(e, module_export);
    }
    e = e->next;
  }

  return AppendExportTable(module_name, module_export);
}

bool MRGetMethodByOrdinal(const char *module_name, uint32_t ordinal,
                          uint32_t *result) {
  *result = 0;

  ModuleExportTable *table = export_table;
  while (table && strcmp(table->module_name, module_name)) {
    table = table->next;
  }
  if (!table) {
    return false;
  }

  ExportNode *node = table->exports;
  while (node && node->entry.ordinal != ordinal) {
    node = node->next;
  }
  if (!node) {
    return false;
  }

  *result = node->entry.address;
  return true;
}

bool MRGetMethodByName(const char *module_name, const char *name,
                       uint32_t *result) {
  *result = 0;

  ModuleExportTable *table = export_table;
  while (table && strcmp(table->module_name, module_name)) {
    table = table->next;
  }
  if (!table) {
    return false;
  }

  ExportNode *node = table->exports;
  while (node && strcmp(node->entry.method_name, name)) {
    node = node->next;
  }
  if (!node) {
    return false;
  }

  *result = node->entry.address;
  return true;
}

uint32_t MRGetNumRegisteredModules() {
  uint32_t ret = 0;
  ModuleExportTable *table = export_table;
  while (table) {
    ++ret;
    table = table->next;
  }
  return ret;
}

uint32_t MRGetTotalNumExports() {
  uint32_t ret = 0;

  ModuleExportTable *table = export_table;
  while (table) {
    ExportNode *node = table->exports;
    while (node) {
      ++ret;
      node = node->next;
    }
    table = table->next;
  }

  return ret;
}

void MREnumerateRegistryBegin(ModuleRegistryCursor *cursor) {
  cursor->module_ = export_table;
  cursor->export_ = export_table->exports;
}

bool MREnumerateRegistry(const char **module_name,
                         const ModuleExport **module_export,
                         ModuleRegistryCursor *cursor) {
  if (!cursor->module_ || !cursor->export_) {
    return false;
  }

  ModuleExportTable *table = (ModuleExportTable *)cursor->module_;
  *module_name = table->module_name;
  ExportNode *node = (ExportNode *)cursor->export_;
  *module_export = &node->entry;

  node = node->next;
  if (!node) {
    table = table->next;
    if (table) {
      node = table->exports;
    }
  }

  cursor->module_ = table;
  cursor->export_ = node;

  return true;
}

void MRResetRegistry(void) {
  ModuleExportTable *table = export_table;
  while (table) {
    ExportNode *node = table->exports;
    while (node) {
      ExportNode *node_delete = node;
      node = node->next;
      DmFreePool(node_delete->entry.method_name);
      DmFreePool(node_delete);
    }
    ModuleExportTable *table_delete = table;
    table = table->next;

    DmFreePool(table_delete->module_name);
    DmFreePool(table_delete);
  }
  export_table = NULL;
}

static bool SetExportEntry(ExportNode **n, const ModuleExport *module_export) {
  *n = (ExportNode *)DmAllocatePoolWithTag(sizeof(**n), TAG);
  if (!n) {
    return false;
  }
  (*n)->next = NULL;
  memcpy(&(*n)->entry, module_export, sizeof((*n)->entry));
  return true;
}

static bool AppendExport(ModuleExportTable *table,
                         const ModuleExport *module_export) {
  ExportNode *n = table->exports;
  if (!n) {
    return SetExportEntry(&table->exports, module_export);
  }

  // Replace any existing entry.
  ExportNode *last = n;
  while (n) {
    if (n->entry.ordinal == module_export->ordinal) {
      if (n->entry.method_name) {
        DmFreePool(n->entry.method_name);
      }
      memcpy(&n->entry, module_export, sizeof(n->entry));
      return true;
    }
    last = n;
    n = n->next;
  }

  return SetExportEntry(&last->next, module_export);
}

static bool SetExportTableEntry(ModuleExportTable **dest,
                                const char *module_name) {
  *dest = (ModuleExportTable *)DmAllocatePoolWithTag(sizeof(**dest), TAG);
  if (!*dest) {
    return false;
  }
  (*dest)->next = NULL;
  (*dest)->module_name = PoolStrdup(module_name, TAG);
  if (!(*dest)->module_name) {
    DmFreePool(*dest);
    *dest = NULL;
    return false;
  }
  (*dest)->exports = NULL;

  return true;
}

static bool AppendExportTable(const char *module_name,
                              const ModuleExport *module_export) {
  ModuleExportTable *e = export_table;
  if (!e) {
    if (!SetExportTableEntry(&export_table, module_name)) {
      return false;
    }
    return AppendExport(export_table, module_export);
  }

  while (e->next) {
    e = e->next;
  }

  if (!SetExportTableEntry(&e->next, module_name)) {
    return false;
  }
  return AppendExport(e->next, module_export);
}
