#define BOOST_TEST_MODULE DXTLibraryTests
#include <boost/test/unit_test.hpp>
#include <string>

#include "module_registry.h"

static bool RegisterExport(const char *name, const char *alias,
                           uint32_t ordinal, uint32_t address);
static bool RegisterExport(const char *module, const char *name,
                           const char *alias, uint32_t ordinal,
                           uint32_t address);

BOOST_AUTO_TEST_SUITE(dll_loader_suite)

BOOST_AUTO_TEST_CASE(populate_registry_test) {
  MRResetRegistry();
  BOOST_TEST(MRGetNumRegisteredModules() == 0);
  BOOST_TEST(MRGetTotalNumExports() == 0);

  RegisterExport("CPDelete@4", "CPDelete", 2, 0x00123400);

  BOOST_TEST(MRGetNumRegisteredModules() == 1);
  BOOST_TEST(MRGetTotalNumExports() == 1);

  RegisterExport("AnotherExport@0", "AnotherExport", 3, 0x00432100);
  BOOST_TEST(MRGetNumRegisteredModules() == 1);
  BOOST_TEST(MRGetTotalNumExports() == 2);
}

BOOST_AUTO_TEST_CASE(exports_with_same_ordinal_overwrite_test) {
  MRResetRegistry();
  BOOST_TEST(MRGetNumRegisteredModules() == 0);
  BOOST_TEST(MRGetTotalNumExports() == 0);

  const uint32_t expected = 0xF00D;
  RegisterExport("M1", "E1@0", "E1", 1, 0x00123400);
  RegisterExport("M1", nullptr, nullptr, 1, expected);

  BOOST_TEST(MRGetNumRegisteredModules() == 1);
  BOOST_TEST(MRGetTotalNumExports() == 1);

  uint32_t result;
  BOOST_TEST(MRGetMethodByOrdinal("M1", 1, &result));
  BOOST_TEST(result == expected);
}

BOOST_AUTO_TEST_CASE(enumerate_registry_test) {
  MRResetRegistry();
  BOOST_TEST(MRGetNumRegisteredModules() == 0);
  BOOST_TEST(MRGetTotalNumExports() == 0);

  RegisterExport("M1", "E1@0", "E1", 1, 0x00123400);
  RegisterExport("M1", "E2@1234", "E2", 2, 0x00432100);

  RegisterExport("M2", "E1@4", "E1", 1, 0x1);
  RegisterExport("M2", "E2@1234", "E2", 2, 0x2);

  BOOST_TEST(MRGetNumRegisteredModules() == 2);
  BOOST_TEST(MRGetTotalNumExports() == 4);

  ModuleRegistryCursor cursor;
  MREnumerateRegistryBegin(&cursor);

  int remaining = 4;
  const char *module;
  const ModuleExport *module_export;

  for (int remaining = 4; remaining > 0; --remaining) {
    BOOST_TEST(MREnumerateRegistry(&module, &module_export, &cursor));
  }

  BOOST_TEST(!MREnumerateRegistry(&module, &module_export, &cursor));
}

BOOST_AUTO_TEST_CASE(resolve_by_name_and_alias_test) {
  MRResetRegistry();
  BOOST_TEST(MRGetNumRegisteredModules() == 0);
  BOOST_TEST(MRGetTotalNumExports() == 0);

  RegisterExport("M1", "E1@0", "E1", 1, 0x00123400);
  RegisterExport("M1", "E2@1234", "E2", 2, 0x00432100);

  RegisterExport("M2", "E1@4", "E1", 1, 0x01);
  RegisterExport("M2", "E2@1234", "E2", 2, 0x02);

  BOOST_TEST(MRGetNumRegisteredModules() == 2);
  BOOST_TEST(MRGetTotalNumExports() == 4);

  uint32_t result;
  BOOST_TEST(MRGetMethodByName("M2", "E1@4", &result));
  BOOST_TEST(result == 0x01);

  BOOST_TEST(MRGetMethodByName("M2", "E2", &result));
  BOOST_TEST(result == 0x02);

  BOOST_TEST(MRGetMethodByName("M1", "E2@1234", &result));
  BOOST_TEST(result == 0x00432100);
}

BOOST_AUTO_TEST_SUITE_END()

static bool RegisterExport(const char *name, const char *alias,
                           uint32_t ordinal, uint32_t address) {
  return RegisterExport("test.dll", name, alias, ordinal, address);
}

static bool RegisterExport(const char *module, const char *name,
                           const char *alias, uint32_t ordinal,
                           uint32_t address) {
  ModuleExport entry;
  if (!name) {
    entry.method_name = NULL;
  } else {
    entry.method_name = strdup(name);
    if (!entry.method_name) {
      return false;
    }
  }
  if (!alias) {
    entry.alias = NULL;
  } else {
    entry.alias = strdup(alias);
    if (!entry.alias) {
      return false;
    }
  }
  entry.ordinal = ordinal;
  entry.address = address;
  return MRRegisterMethod(module, &entry);
}
