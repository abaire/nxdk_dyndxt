#include "command_processor_util.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "xbdm.h"
#include "xbdm_err.h"

// 'dxcp' - ddxt command processor
static const uint32_t kTag = 0x64786370;

static const char *FirstNonSpace(const char *input);
static const char *KeyEnd(const char *input);
static const char *ValueEnd(const char *input, bool *has_escaped_values);

void CP_API CPDelete(CommandParameters *cp) {
  for (int i = 0; i < cp->entries; ++i) {
    if (cp->keys && cp->keys[i]) {
      DmFreePool(cp->keys[i]);
    }
    if (cp->values && cp->values[i]) {
      DmFreePool(cp->values[i]);
    }
  }

  cp->entries = 0;
  if (cp->keys) {
    DmFreePool(cp->keys);
    cp->keys = NULL;
  }

  if (cp->values) {
    DmFreePool(cp->values);
    cp->values = NULL;
  }
}

static bool CommandParametersReserve(int32_t max_entries,
                                     CommandParameters *cp) {
  const size_t new_size = sizeof(char *) * max_entries;
  char **new_keys = DmAllocatePoolWithTag(new_size, kTag);
  if (!new_keys) {
    return false;
  }
  if (cp->keys) {
    memcpy(new_keys, cp->keys, sizeof(new_keys[0]) * cp->entries);
    DmFreePool(cp->keys);
  }
  cp->keys = new_keys;

  char **new_values = DmAllocatePoolWithTag(new_size, kTag);
  if (!new_values) {
    return false;
  }
  if (cp->values) {
    memcpy(new_values, cp->values, sizeof(new_values[0]) * cp->entries);
    DmFreePool(cp->values);
  }
  cp->values = new_values;

  for (int32_t i = cp->entries; i < max_entries; ++i) {
    cp->keys[i] = NULL;
    cp->values[i] = NULL;
  }

  return true;
}

static int32_t CommandParametersAppend(
    int32_t *max_entries, const char *key_start, const char *key_end,
    const char *value_start, const char *value_end, bool value_needs_unescaping,
    CommandParameters *cp) {
  if (cp->entries == *max_entries) {
    *max_entries *= 2;
    if (!CommandParametersReserve(*max_entries, cp)) {
      return PCP_ERR_OUT_OF_MEMORY;
    }
  }

  int32_t index = cp->entries;
  size_t key_size = 1 + key_end - key_start;
  cp->keys[index] = (char *)DmAllocatePoolWithTag(key_size, kTag);
  if (!cp->keys[index]) {
    return PCP_ERR_OUT_OF_MEMORY;
  }
  ++cp->entries;
  memcpy(cp->keys[index], key_start, key_size - 1);
  cp->keys[index][key_size - 1] = 0;

  if (value_start && value_start != value_end) {
    size_t value_size = 1 + value_end - value_start;
    cp->values[cp->entries - 1] =
        (char *)DmAllocatePoolWithTag(value_size, kTag);
    if (!cp->values[index]) {
      return PCP_ERR_OUT_OF_MEMORY;
    }
    if (!value_needs_unescaping) {
      memcpy(cp->values[index], value_start, value_size - 1);
      cp->values[index][value_size - 1] = 0;
    } else {
      char *dest = cp->values[index];
      while (value_start != value_end) {
        if (*value_start == '\\') {
          if (++value_start == value_end) {
            return PCP_ERR_INVALID_INPUT_UNTERMINATED_ESCAPE;
          }
        }
        *dest++ = *value_start++;
      }
      *dest = 0;
    }
  }

  return 0;
}

int32_t CP_API CPParseCommandParameters(const char *params,
                                        CommandParameters *result) {
  if (!result) {
    return PCP_ERR_INVALID_INPUT;
  }

  memset(result, 0, sizeof(CommandParameters));

  if (!params) {
    return PCP_ERR_INVALID_INPUT;
  }

  int32_t max_entries = DEFAULT_COMMAND_PARAMETER_RESERVE_SIZE;
  if (!CommandParametersReserve(max_entries, result)) {
    CPDelete(result);
    return PCP_ERR_OUT_OF_MEMORY;
  }

  const char *key_start = FirstNonSpace(params);
  if (!*key_start) {
    CPDelete(result);
    return 0;
  }

  while (*key_start) {
    const char *key_end = KeyEnd(key_start);
    if (key_end == key_start) {
      CPDelete(result);
      return PCP_ERR_INVALID_KEY;
    }

    const char *entry_end = key_end;
    const char *value_start = NULL;
    const char *value_end = NULL;
    bool value_needs_unescaping = false;
    if (*key_end == '=') {
      value_start = key_end + 1;
      value_end = ValueEnd(value_start, &value_needs_unescaping);
      entry_end = value_end;

      // Ignore the enclosing quotation marks.
      if (*value_start == '"') {
        ++value_start;
        if (*value_end != '"') {
          CPDelete(result);
          return PCP_ERR_INVALID_INPUT_UNTERMINATED_QUOTED_KEY;
        }
        // The next entry must start after the terminating quotation mark.
        ++entry_end;
      }
    }

    int set_result =
        CommandParametersAppend(&max_entries, key_start, key_end, value_start,
                                value_end, value_needs_unescaping, result);
    if (set_result) {
      CPDelete(result);
      return set_result;
    }

    key_start = FirstNonSpace(entry_end);
  }

  return result->entries;
}

uint32_t CP_API CPPrintError(int32_t parse_return_code, char *buffer,
                             uint32_t buffer_len) {
  switch (parse_return_code) {
    default:
      strncpy(buffer, "UNKNOWN ERROR", buffer_len);
      return XBOX_E_UNEXPECTED;

    case PCP_ERR_INVALID_INPUT:
      strncpy(buffer, "Invalid input", buffer_len);
      return XBOX_E_FAIL;

    case PCP_ERR_OUT_OF_MEMORY:
      strncpy(buffer, "Out of memory", buffer_len);
      return XBOX_E_CREATE_FILE_FAILED;

    case PCP_ERR_INVALID_KEY:
      strncpy(buffer, "Malformed key", buffer_len);
      return XBOX_E_FAIL;

    case PCP_ERR_INVALID_INPUT_UNTERMINATED_QUOTED_KEY:
      strncpy(buffer, "Malformed key (unterminated quote)", buffer_len);
      return XBOX_E_FAIL;

    case PCP_ERR_INVALID_INPUT_UNTERMINATED_ESCAPE:
      strncpy(buffer, "Malformed key (unterminated escape char)", buffer_len);
      return XBOX_E_FAIL;
  }
}

bool CP_API CPHasKey(const char *key, CommandParameters *cp) {
  if (!cp || !key) {
    return false;
  }
  for (int32_t i = 0; i < cp->entries; ++i) {
    if (!strcmp(cp->keys[i], key)) {
      return true;
    }
  }
  return false;
}

bool CP_API CPGetString(const char *key, const char **result,
                        CommandParameters *cp) {
  if (!result) {
    return false;
  }
  *result = NULL;

  if (!cp || !key) {
    return false;
  }

  for (int32_t i = 0; i < cp->entries; ++i) {
    if (!strcmp(cp->keys[i], key)) {
      *result = cp->values[i];
      return *result != NULL;
    }
  }
  return false;
}

bool CP_API CPGetUInt32(const char *key, uint32_t *result,
                        CommandParameters *cp) {
  if (!result) {
    return false;
  }

  const char *string_value;
  if (!CPGetString(key, &string_value, cp)) {
    *result = 0;
    return false;
  }

  const char *end;
  *result = strtoul(string_value, (char **)&end, 0);
  if (end == string_value) {
    return false;
  }

  // The full value must be a number to consider it valid.
  if (*end != 0) {
    *result = 0;
    return false;
  }

  return true;
}

bool CP_API CPGetInt32(const char *key, int32_t *result,
                       CommandParameters *cp) {
  if (!result) {
    return false;
  }

  const char *string_value;
  if (!CPGetString(key, &string_value, cp)) {
    *result = 0;
    return false;
  }

  const char *end;
  *result = strtol(string_value, (char **)&end, 0);
  if (end == string_value) {
    return false;
  }

  // The full value must be a number to consider it valid.
  if (*end != 0) {
    *result = 0;
    return false;
  }

  return true;
}

static const char *FirstNonSpace(const char *input) {
  while (*input && *input == ' ') {
    ++input;
  }
  return input;
}

static const char *KeyEnd(const char *input) {
  while (*input && *input != ' ' && *input != '=') {
    ++input;
  }
  return input;
}

static const char *ValueEnd(const char *input, bool *has_escaped_values) {
  *has_escaped_values = false;
  if (*input && *input == '"') {
    ++input;
    while (*input && *input != '"') {
      // Ignore any escaped values but flag that unescaping is necessary.
      if (*input == '\\') {
        *has_escaped_values = true;
        ++input;
      }
      ++input;
    }
    return input;
  }
  while (*input && *input != ' ') {
    ++input;
  }
  return input;
}
