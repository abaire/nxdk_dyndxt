#define BOOST_TEST_MODULE DXTLibraryTests
#include <boost/test/unit_test.hpp>

#include "command_processor_util.h"

#define TEST_KEY(cp, index, value)                        \
  do {                                                    \
    BOOST_TEST((cp).keys);                                \
    if ((cp).keys) {                                      \
      BOOST_TEST((cp).keys[(index)] != nullptr);          \
      if ((cp).keys[(index)]) {                           \
        BOOST_TEST(!strcmp((cp).keys[(index)], (value))); \
      }                                                   \
    }                                                     \
  } while (0)

#define TEST_VALUE_NULL(cp, index)                 \
  do {                                             \
    BOOST_TEST((cp).values);                       \
    if ((cp).values) {                             \
      BOOST_TEST((cp).values[(index)] == nullptr); \
    }                                              \
  } while (0)

#define TEST_VALUE(cp, index, value)                        \
  do {                                                      \
    BOOST_TEST((cp).values);                                \
    if ((cp).values) {                                      \
      BOOST_TEST((cp).values[(index)] != nullptr);          \
      if ((cp).values[(index)]) {                           \
        BOOST_TEST(!strcmp((cp).values[(index)], (value))); \
      }                                                     \
    }                                                       \
  } while (0)

#define SAFE_TEST_STRING(value, expected)       \
  do {                                          \
    BOOST_TEST(value);                          \
    if (value) {                                \
      BOOST_TEST(!strcmp((value), (expected))); \
    }                                           \
  } while (0)

BOOST_AUTO_TEST_SUITE(command_processor_suite)

// Missing inputs are erroneous.
BOOST_AUTO_TEST_CASE(invalid_inputs_test) {
  CommandParameters cp;

  BOOST_TEST(CPParseCommandParameters(nullptr, &cp) == PCP_ERR_INVALID_INPUT);
  BOOST_TEST(cp.keys == nullptr);
  BOOST_TEST(cp.values == nullptr);
  BOOST_TEST(CPParseCommandParameters("", nullptr) == PCP_ERR_INVALID_INPUT);
}

// Values without a key are erroneous.
BOOST_AUTO_TEST_CASE(invalid_key_test) {
  CommandParameters cp;

  BOOST_TEST(CPParseCommandParameters("=keyless_value", &cp) ==
             PCP_ERR_INVALID_KEY);
  BOOST_TEST(cp.keys == nullptr);
  BOOST_TEST(cp.values == nullptr);
}

// Empty parameter strings should result in an empty struct.
BOOST_AUTO_TEST_CASE(empty_input_test) {
  CommandParameters cp;

  BOOST_TEST(CPParseCommandParameters("", &cp) == 0);
  BOOST_TEST(cp.keys == nullptr);
  BOOST_TEST(cp.values == nullptr);
}

// Whitespace-only parameter strings should result in an empty struct.
BOOST_AUTO_TEST_CASE(whitespace_only_input_test) {
  CommandParameters cp;

  BOOST_TEST(CPParseCommandParameters("     ", &cp) == 0);
  BOOST_TEST(cp.keys == nullptr);
  BOOST_TEST(cp.values == nullptr);
}

// Any amount of leading whitespace should be ignored.
BOOST_AUTO_TEST_CASE(leading_whitespace_ignored_test) {
  CommandParameters cp;

  BOOST_TEST(CPParseCommandParameters("          test", &cp) == 1);
  BOOST_TEST(cp.keys);
  TEST_KEY(cp, 0, "test");
  TEST_VALUE_NULL(cp, 0);

  CPDelete(&cp);
}

// Valueless keys with trailing whitespace should ignore the trailing
// whitespace.
BOOST_AUTO_TEST_CASE(valueless_key_trailing_whitespace_ignored_test) {
  CommandParameters cp;

  BOOST_TEST(CPParseCommandParameters("test       ", &cp) == 1);
  TEST_KEY(cp, 0, "test");
  TEST_VALUE_NULL(cp, 0);

  BOOST_TEST(CPHasKey("test", &cp));

  CPDelete(&cp);
}

// Keys can be given without any associated value.
BOOST_AUTO_TEST_CASE(single_valueless_key_test) {
  CommandParameters cp;

  BOOST_TEST(CPParseCommandParameters("test", &cp) == 1);
  BOOST_TEST(cp.entries == 1);
  TEST_KEY(cp, 0, "test");
  TEST_VALUE_NULL(cp, 0);

  BOOST_TEST(CPHasKey("test", &cp));

  CPDelete(&cp);
}

// Keys can have a unquoted value.
BOOST_AUTO_TEST_CASE(single_key_value_test) {
  CommandParameters cp;

  BOOST_TEST(CPParseCommandParameters("test=value", &cp) == 1);
  BOOST_TEST(cp.entries == 1);
  TEST_KEY(cp, 0, "test");
  TEST_VALUE(cp, 0, "value");

  CPDelete(&cp);
}

// Multiple key value pairs are allowed.
BOOST_AUTO_TEST_CASE(multiple_key_value_test) {
  CommandParameters cp;

  BOOST_TEST(CPParseCommandParameters("test=value test2=value2", &cp) == 2);
  BOOST_TEST(cp.entries == 2);
  TEST_KEY(cp, 0, "test");
  TEST_VALUE(cp, 0, "value");
  TEST_KEY(cp, 1, "test2");
  TEST_VALUE(cp, 1, "value2");

  CPDelete(&cp);
}

// Valueless keys and key-value pairs can be intermixed.
BOOST_AUTO_TEST_CASE(valueless_and_key_value_test) {
  CommandParameters cp;

  BOOST_TEST(CPParseCommandParameters("test test2=value2 test3", &cp) == 3);
  BOOST_TEST(cp.entries == 3);
  TEST_KEY(cp, 0, "test");
  TEST_VALUE_NULL(cp, 0);
  TEST_KEY(cp, 1, "test2");
  TEST_VALUE(cp, 1, "value2");
  TEST_KEY(cp, 2, "test3");
  TEST_VALUE_NULL(cp, 2);

  CPDelete(&cp);
}

// Keys can have a quoted value.
BOOST_AUTO_TEST_CASE(single_key_quoted_value_test) {
  CommandParameters cp;

  BOOST_TEST(
      CPParseCommandParameters("test=\"quoted value with spaces\"", &cp) == 1);
  BOOST_TEST(cp.entries == 1);
  TEST_KEY(cp, 0, "test");
  TEST_VALUE(cp, 0, "quoted value with spaces");

  CPDelete(&cp);
}

// Unterminated quotes are erroneous.
BOOST_AUTO_TEST_CASE(single_key_unterminated_quoted_value_test) {
  CommandParameters cp;

  BOOST_TEST(CPParseCommandParameters("test=\"quoted value ", &cp) ==
             PCP_ERR_INVALID_INPUT_UNTERMINATED_QUOTED_KEY);
  BOOST_TEST(cp.keys == nullptr);
  BOOST_TEST(cp.values == nullptr);

  CPDelete(&cp);
}

// Quoted values can have escaped quotes.
BOOST_AUTO_TEST_CASE(quoted_value_escaped_quote_test) {
  CommandParameters cp;

  BOOST_TEST(CPParseCommandParameters("test=\"quoted value with \\\" quote\"",
                                      &cp) == 1);
  BOOST_TEST(cp.entries == 1);
  TEST_KEY(cp, 0, "test");
  TEST_VALUE(cp, 0, "quoted value with \" quote");

  CPDelete(&cp);
}

// Quoted values can have escaped quotes.
BOOST_AUTO_TEST_CASE(complex_test) {
  CommandParameters cp;

  BOOST_TEST(
      CPParseCommandParameters(
          "test=\"escaped \\\" quote\" test2=value2 test3 test4=\"value4\"",
          &cp) == 4);
  BOOST_TEST(cp.entries == 4);
  TEST_KEY(cp, 0, "test");
  TEST_VALUE(cp, 0, "escaped \" quote");
  TEST_KEY(cp, 1, "test2");
  TEST_VALUE(cp, 1, "value2");
  TEST_KEY(cp, 2, "test3");
  TEST_VALUE_NULL(cp, 2);
  TEST_KEY(cp, 3, "test4");
  TEST_VALUE(cp, 3, "value4");

  CPDelete(&cp);
}

// More than the default size number of keys can be parsed.
BOOST_AUTO_TEST_CASE(many_keys_test) {
  CommandParameters cp;

#if (DEFAULT_COMMAND_PARAMETER_RESERVE_SIZE > 7)
#error "Test must be updated to check allocation beyond initial reserve"
#endif

  BOOST_TEST(
      CPParseCommandParameters("test=\"escaped \\\" quote\" test2=value2 "
                               "test3 test4=\"value4\" test5 test6 test7",
                               &cp) == 7);
  BOOST_TEST(cp.entries == 7);
  TEST_KEY(cp, 0, "test");
  TEST_VALUE(cp, 0, "escaped \" quote");
  TEST_KEY(cp, 1, "test2");
  TEST_VALUE(cp, 1, "value2");
  TEST_KEY(cp, 2, "test3");
  TEST_VALUE_NULL(cp, 2);
  TEST_KEY(cp, 3, "test4");
  TEST_VALUE(cp, 3, "value4");
  TEST_KEY(cp, 4, "test5");
  TEST_VALUE_NULL(cp, 4);
  TEST_KEY(cp, 5, "test6");
  TEST_VALUE_NULL(cp, 5);
  TEST_KEY(cp, 6, "test7");
  TEST_VALUE_NULL(cp, 6);

  CPDelete(&cp);
}

// CPHasKey tests
BOOST_AUTO_TEST_CASE(has_key_test) {
  CommandParameters cp;

  CPParseCommandParameters(
      "test=\"escaped \\\" quote\" test2=value2 test3 test4=\"value4\" test5 "
      "test6 test7",
      &cp);
  BOOST_TEST(cp.entries == 7);

  BOOST_TEST(!CPHasKey(nullptr, nullptr));
  BOOST_TEST(!CPHasKey("", nullptr));
  BOOST_TEST(!CPHasKey("", &cp));
  BOOST_TEST(!CPHasKey("KeyDoesNotExist", &cp));
  BOOST_TEST(CPHasKey("test7", &cp));
  BOOST_TEST(CPHasKey("test6", &cp));
  BOOST_TEST(CPHasKey("test5", &cp));
  BOOST_TEST(CPHasKey("test4", &cp));
  BOOST_TEST(CPHasKey("test3", &cp));
  BOOST_TEST(CPHasKey("test2", &cp));
  BOOST_TEST(CPHasKey("test", &cp));

  CPDelete(&cp);
}

// CPGetString tests
BOOST_AUTO_TEST_CASE(get_string_test) {
  CommandParameters cp;

  CPParseCommandParameters(
      "test=\"escaped \\\" quote\" test2=value2 test3 test4=\"value4\" ", &cp);
  BOOST_TEST(cp.entries == 4);

  BOOST_TEST(!CPGetString(nullptr, nullptr, nullptr));

  const char *result = (const char *)1;
  BOOST_TEST(!CPGetString(nullptr, nullptr, &cp));

  BOOST_TEST(!CPGetString("", &result, &cp));
  BOOST_TEST(result == nullptr);
  BOOST_TEST(!CPGetString("KeyDoesNotExist", &result, &cp));
  BOOST_TEST(result == nullptr);
  BOOST_TEST(!CPGetString("test3", &result, &cp));
  BOOST_TEST(result == nullptr);

  BOOST_TEST(CPGetString("test", &result, &cp));
  SAFE_TEST_STRING(result, "escaped \" quote");

  BOOST_TEST(CPGetString("test2", &result, &cp));
  SAFE_TEST_STRING(result, "value2");

  BOOST_TEST(CPGetString("test4", &result, &cp));
  SAFE_TEST_STRING(result, "value4");

  CPDelete(&cp);
}

// CPGetInt32 tests
BOOST_AUTO_TEST_CASE(get_int32_test) {
  CommandParameters cp;

  CPParseCommandParameters(
      "test=\"escaped \\\" quote\" test2=2 test3 test4=\"4\" test5=\"5 \" "
      "test6=0xFFFFFFFF test7=-1 test8=+12 test9=0664",
      &cp);
  BOOST_TEST(cp.entries == 9);

  BOOST_TEST(!CPGetInt32(nullptr, nullptr, nullptr));

  int32_t result;
  BOOST_TEST(!CPGetInt32(nullptr, nullptr, &cp));

  BOOST_TEST(!CPGetInt32("", &result, &cp));
  BOOST_TEST(result == 0);
  BOOST_TEST(!CPGetInt32("KeyDoesNotExist", &result, &cp));
  BOOST_TEST(result == 0);
  BOOST_TEST(!CPGetInt32("test", &result, &cp));
  BOOST_TEST(result == 0);
  BOOST_TEST(!CPGetInt32("test3", &result, &cp));
  BOOST_TEST(result == 0);
  BOOST_TEST(!CPGetInt32("test5", &result, &cp));
  BOOST_TEST(result == 0);

  BOOST_TEST(CPGetInt32("test2", &result, &cp));
  BOOST_TEST(result == 2);

  BOOST_TEST(CPGetInt32("test4", &result, &cp));
  BOOST_TEST(result == 4);

  BOOST_TEST(CPGetInt32("test6", &result, &cp));
  BOOST_TEST(result == -1);

  BOOST_TEST(CPGetInt32("test7", &result, &cp));
  BOOST_TEST(result == -1);

  BOOST_TEST(CPGetInt32("test8", &result, &cp));
  BOOST_TEST(result == 12);

  BOOST_TEST(CPGetInt32("test9", &result, &cp));
  BOOST_TEST(result == 0664);

  CPDelete(&cp);
}

// CPGetUInt32 tests
BOOST_AUTO_TEST_CASE(get_uint32_test) {
  CommandParameters cp;

  CPParseCommandParameters(
      "test=\"escaped \\\" quote\" test2=2 test3 test4=\"4\" test5=\"5 \" "
      "test6=0xFFFFFFFF test7=-1 test8=+12 test9=0664",
      &cp);
  BOOST_TEST(cp.entries == 9);

  BOOST_TEST(!CPGetUInt32(nullptr, nullptr, nullptr));

  uint32_t result;
  BOOST_TEST(!CPGetUInt32(nullptr, nullptr, &cp));

  BOOST_TEST(!CPGetUInt32("", &result, &cp));
  BOOST_TEST(result == 0);
  BOOST_TEST(!CPGetUInt32("KeyDoesNotExist", &result, &cp));
  BOOST_TEST(result == 0);
  BOOST_TEST(!CPGetUInt32("test", &result, &cp));
  BOOST_TEST(result == 0);
  BOOST_TEST(!CPGetUInt32("test3", &result, &cp));
  BOOST_TEST(result == 0);
  BOOST_TEST(!CPGetUInt32("test5", &result, &cp));
  BOOST_TEST(result == 0);

  BOOST_TEST(CPGetUInt32("test2", &result, &cp));
  BOOST_TEST(result == 2);

  BOOST_TEST(CPGetUInt32("test4", &result, &cp));
  BOOST_TEST(result == 4);

  BOOST_TEST(CPGetUInt32("test6", &result, &cp));
  BOOST_TEST(result == 0xFFFFFFFF);

  BOOST_TEST(CPGetUInt32("test7", &result, &cp));
  BOOST_TEST(result == 0xFFFFFFFF);

  BOOST_TEST(CPGetUInt32("test8", &result, &cp));
  BOOST_TEST(result == 12);

  BOOST_TEST(CPGetUInt32("test9", &result, &cp));
  BOOST_TEST(result == 0664);

  CPDelete(&cp);
}

BOOST_AUTO_TEST_CASE(realistic_test) {
  CommandParameters cp;
  CPParseCommandParameters(
      "ddxt!install base=0xb00ee000 length=0x5000 entrypoint=0xb00ef000", &cp);
  BOOST_TEST(cp.entries == 4);

  uint32_t result;
  BOOST_TEST(CPGetUInt32("base", &result, &cp));
  BOOST_TEST(result == 0xb00ee000);

  BOOST_TEST(CPGetUInt32("length", &result, &cp));
  BOOST_TEST(result == 0x5000);

  BOOST_TEST(CPGetUInt32("entrypoint", &result, &cp));
  BOOST_TEST(result == 0xb00ef000);

  BOOST_TEST(CPHasKey("ddxt!install", &cp));

  CPDelete(&cp);
}

BOOST_AUTO_TEST_SUITE_END()
