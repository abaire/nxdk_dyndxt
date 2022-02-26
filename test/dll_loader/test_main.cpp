#define BOOST_TEST_MODULE DXTLibraryTests
#include <boost/test/unit_test.hpp>
#include <string>

#include "dll_loader.h"
#include "golden_dll.h"
#include "relocated_B00D7000.h"

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

BOOST_AUTO_TEST_CASE(nop_relocation_test) {
  DLLContext ctx;

  memset(&ctx, 0, sizeof(ctx));

  ctx.input.raw_data = kDynDXTLoader;
  ctx.input.raw_data_size = sizeof(kDynDXTLoader);
  ctx.input.alloc = malloc;
  ctx.input.free = free;
  ctx.input.resolve_import_by_ordinal = ResolveImportByOrdinalAlwaysSucceed;
  ctx.input.resolve_import_by_name = ResolveImportByNameAlwaysSucceed;

  BOOST_TEST(DLLLoad(&ctx));

  hwaddress_t base = (uint32_t)(intptr_t)ctx.output.image;
  hwaddress_t entrypoint = ctx.output.entrypoint;

  DLLRelocate(&ctx, base);

  BOOST_TEST(entrypoint == ctx.output.entrypoint);

  DLLFreeContext(&ctx, false);
}

BOOST_AUTO_TEST_CASE(positive_relocation_test) {
  DLLContext ctx;

  memset(&ctx, 0, sizeof(ctx));

  ctx.input.raw_data = kDynDXTLoader;
  ctx.input.raw_data_size = sizeof(kDynDXTLoader);
  ctx.input.alloc = malloc;
  ctx.input.free = free;
  ctx.input.resolve_import_by_ordinal = ResolveImportByOrdinalAlwaysSucceed;
  ctx.input.resolve_import_by_name = ResolveImportByNameAlwaysSucceed;

  BOOST_TEST(DLLLoad(&ctx));

  hwaddress_t base = (hwaddress_t)(intptr_t)ctx.output.image;
  hwaddress_t entrypoint = ctx.output.entrypoint;

  DLLRelocate(&ctx, base + 0x500);

  BOOST_TEST((entrypoint + 0x500) == (uint32_t)(intptr_t)ctx.output.entrypoint);

  DLLFreeContext(&ctx, false);
}

BOOST_AUTO_TEST_CASE(negative_relocation_test) {
  DLLContext ctx;

  memset(&ctx, 0, sizeof(ctx));

  ctx.input.raw_data = kDynDXTLoader;
  ctx.input.raw_data_size = sizeof(kDynDXTLoader);
  ctx.input.alloc = malloc;
  ctx.input.free = free;
  ctx.input.resolve_import_by_ordinal = ResolveImportByOrdinalAlwaysSucceed;
  ctx.input.resolve_import_by_name = ResolveImportByNameAlwaysSucceed;

  BOOST_TEST(DLLLoad(&ctx));

  hwaddress_t base = (hwaddress_t)(intptr_t)ctx.output.image;
  hwaddress_t entrypoint = ctx.output.entrypoint;

  DLLRelocate(&ctx, base - 0x500);

  BOOST_TEST((entrypoint - 0x500) == (uint32_t)(intptr_t)ctx.output.entrypoint);

  DLLFreeContext(&ctx, false);
}

BOOST_AUTO_TEST_CASE(relocation_test) {
  DLLContext ctx;

  memset(&ctx, 0, sizeof(ctx));

  ctx.input.raw_data = kDynDXTLoader;
  ctx.input.raw_data_size = sizeof(kDynDXTLoader);
  ctx.input.alloc = malloc;
  ctx.input.free = free;
  ctx.input.resolve_import_by_ordinal = ResolveImportByOrdinalAlwaysSucceed;
  ctx.input.resolve_import_by_name = ResolveImportByNameAlwaysSucceed;

  BOOST_TEST(DLLLoad(&ctx));

  uint32_t image_size = ctx.output.header.OptionalHeader.SizeOfImage;

  BOOST_TEST(image_size == sizeof(kRelocatedB00D7000));

  BOOST_TEST(memcmp(ctx.output.image, kRelocatedB00D7000, image_size));

  DLLRelocate(&ctx, 0xB00D7000);

  auto loaded = reinterpret_cast<const uint8_t *>(ctx.output.image);
  const uint8_t *golden = kRelocatedB00D7000;
  for (auto i = 0; i < image_size; ++i, ++loaded, ++golden) {
    BOOST_TEST_CONTEXT("[0x" << std::hex << i << "]") {
      BOOST_TEST(*loaded == *golden);
    }
  }

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

static bool ResolveImportByOrdinalAlwaysSucceed(const char *module,
                                                uint32_t ordinal,
                                                uint32_t *result) {
  // The golden relocated image was prepared with predictable fake addresses
  // that must be matched here.
  *result = (strlen(module) << 16) + ordinal;
  return true;
}

static bool ResolveImportByNameAlwaysSucceed(const char *, const char *,
                                             uint32_t *result) {
  *result = 0xABCDF00D;
  return true;
}
