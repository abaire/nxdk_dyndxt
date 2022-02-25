#define BOOST_TEST_MODULE DXTLibraryTests
#include <boost/test/unit_test.hpp>
#include <string>

#include "dll_loader.h"
#include "golden_dll.h"

static bool ResolveImportByOrdinalAlwaysFail(const char *, uint32_t,
                                             uint32_t *);
static bool ResolveImportByNameAlwaysFail(const char *, const char *,
                                          uint32_t *);
static bool ResolveImportByOrdinalAlwaysSucceed(const char *, uint32_t,
                                                uint32_t *);
static bool ResolveImportByNameAlwaysSucceed(const char *, const char *,
                                             uint32_t *);

BOOST_AUTO_TEST_SUITE(dll_loader_suite)

BOOST_AUTO_TEST_CASE(empty_raw_data_test) {
  DLLContext ctx;

  memset(&ctx, 0, sizeof(ctx));
  BOOST_TEST(!DLLLoad(&ctx));
  BOOST_TEST(ctx.output.context == DLLL_DOS_HEADER);
  BOOST_TEST(ctx.output.status == DLLL_FILE_TOO_SMALL);

  DLLFreeContext(&ctx, false);
}

BOOST_AUTO_TEST_CASE(valid_dll_test) {
  DLLContext ctx;

  memset(&ctx, 0, sizeof(ctx));

  ctx.input.raw_data = kDynDXTLoader;
  ctx.input.raw_data_size = sizeof(kDynDXTLoader);
  ctx.input.alloc = malloc;
  ctx.input.free = free;
  ctx.input.resolve_import_by_ordinal = ResolveImportByOrdinalAlwaysSucceed;
  ctx.input.resolve_import_by_name = ResolveImportByNameAlwaysSucceed;

  BOOST_TEST(DLLLoad(&ctx));

  DLLFreeContext(&ctx, false);
}

BOOST_AUTO_TEST_SUITE_END()

static bool ResolveImportByOrdinalAlwaysFail(const char *, uint32_t,
                                             uint32_t *) {
  return false;
}

static bool ResolveImportByNameAlwaysFail(const char *, const char *,
                                          uint32_t *) {
  return false;
}

static bool ResolveImportByOrdinalAlwaysSucceed(const char *, uint32_t,
                                                uint32_t *result) {
  *result = 0xF00DABCD;
  return true;
}

static bool ResolveImportByNameAlwaysSucceed(const char *, const char *,
                                             uint32_t *result) {
  *result = 0xABCDF00D;
  return true;
}
