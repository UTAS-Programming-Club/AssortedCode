#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <zstd.h>

#define base 85

struct  __attribute__((__packed__)) SaveData {
  uint16_t a;
  uint8_t b;
  uint8_t c;
  uint8_t d;
  uint16_t e;
  uint8_t f;
  int8_t g;
  uint32_t h;
  int32_t i;
  int8_t j;
  uint64_t k;
};

#include <stdlib.h>
#include <string.h>

static inline size_t min(size_t a, size_t b) {
  return a < b ? a : b;
}

static inline char GetChar(uint_fast8_t val) {
  if (val > base) {
    return ' ';
  }

  return '!' + val;
}

static void *CompressData(const void *data, size_t dataSize, size_t *maxCompressedDataSize, size_t *compressedDataSize) {
  if (!data || !maxCompressedDataSize || !compressedDataSize) {
    return NULL;
  }

  *maxCompressedDataSize = ZSTD_compressBound(dataSize);
  if (ZSTD_isError(*maxCompressedDataSize)) {
    puts("Failed to determine max compressed size.");
    return NULL;
  }

  void *compressedData = malloc(*maxCompressedDataSize);
  if (!compressedData) {
    puts("Failed to allocate room for compressed data.");
    return NULL;
  }

  *compressedDataSize = ZSTD_compress(compressedData, *maxCompressedDataSize, data, dataSize, 3);
  if (ZSTD_isError(*compressedDataSize)) {
    puts("Failed to compress data.");
    free(compressedData);
    return NULL;
  }

  return compressedData;
}

static char *EncodeData(const void *data, size_t dataSize, size_t *passwordSize) {
  // TODO: This overestimates if dataSize is not a multiple of 4
  *passwordSize = 5 * ceil(dataSize / 4.) + 1;
  printf("Size of password string: %zu\n", *passwordSize);

  char *password = malloc(*passwordSize);
  char *passwordPos = password;
  if (!password) {
    puts("Failed to allocate room for the password.");
    return NULL;
  }

  uint_fast32_t powers[] = {
    pow(base, 0),
    pow(base, 1),
    pow(base, 2),
    pow(base, 3),
    pow(base, 4)
  };

  for (size_t i = 0; i < dataSize; i += 4) {
    uint_fast32_t value = 0;
    size_t rem = dataSize - i % dataSize;
    size_t quot = min(rem, 4);
    // printf("\n%zu-%zu: remainder is %zu, encoding %zu bytes\n", i, i + 3, rem, quot);
    memcpy(&value, data + i, quot);

    uint_fast8_t digit4 = value / powers[4];
    uint_fast32_t value4 = value - digit4 * powers[4];
    if (digit4 >= base) {
      printf("base %i is too small.\n", base);
      free(password);
      return NULL;
    }

    uint_fast8_t digit3 = value4 / powers[3];
    uint_fast32_t value3 = value4 - digit3 * powers[3];
    if (digit3 >= base) {
      printf("base %i is too small.\n", base);
      free(password);
      return NULL;
    }

    uint_fast8_t digit2 = value3 / powers[2];
    uint_fast32_t value2 = value3 - digit2 * powers[2];
    if (digit2 >= base) {
      printf("base %i is too small.\n", base);
      free(password);
      return NULL;
    }

    uint_fast8_t digit1 = value2 / powers[1];
    uint_fast32_t value1 = value2 - digit1 * powers[1];
    if (digit1 >= base) {
      printf("base %i is too small.\n", base);
      free(password);
      return NULL;
    }

    uint_fast8_t digit0 = value1 / powers[0];
    uint_fast32_t value0 = value1 - digit0 * powers[0];
    if (0 != value0) {
      puts("value is too big.");
      free(password);
      return NULL;
    }

    *(passwordPos++) = GetChar(digit0);
    *(passwordPos++) = GetChar(digit1);
    *(passwordPos++) = GetChar(digit2);
    *(passwordPos++) = GetChar(digit3);
    *(passwordPos++) = GetChar(digit4);

    // printf(
    //   "%zu-%zu: %" PRIuFAST32 " = "
    //   "%" PRIuFAST8 " * %" PRIuFAST8 "^%i + "
    //   "%" PRIuFAST8 " * %" PRIuFAST8 "^%i + "
    //   "%" PRIuFAST8 " * %" PRIuFAST8 "^%i + "
    //   "%" PRIuFAST8 " * %" PRIuFAST8 "^%i + "
    //   "%" PRIuFAST8 " * %" PRIuFAST8 "^%i\n",
    //   i, i + 3, value,
    //   digit4, base, 4,
    //   digit3, base, 3,
    //   digit2, base, 2,
    //   digit1, base, 1,
    //   digit0, base, 0
    // );

    // printf(
    //   "%zu-%zu: "
    //    "%*s\n", 
    //   i, i + 3, 
    //   5, passwordPos - 5
    // );
  }
  *passwordPos = '\0';

  return password;
}

int main(void) {
  struct SaveData data = {
    .a = 12345,
    .b = 210,
    .c = 32,
    .d = 126,
    .e = UINT16_MAX,
    .f = 3,
    .g = -42,
    .h = UINT32_MAX,
    .i = INT32_MAX / 3,
    .j = 9,
    .k = INT64_MAX
  };

  void *pData = &data;
  size_t dataSize = sizeof data;

  // For testing
  dataSize *= 2;
  pData = malloc(dataSize);
  memcpy(pData, &data, sizeof data);
  memcpy(pData + sizeof data, &data, sizeof data);

  size_t maxCompressedDataSize, compressedDataSize;
  void *compressedData = CompressData(pData, dataSize, &maxCompressedDataSize, &compressedDataSize);
  if (!compressedData) {
    return 1;
  }

  printf("Size of struct SaveData: %zu\n", dataSize);
  printf("Max compressed size of struct SaveData: %zu\n", maxCompressedDataSize);
  printf("Actual compressed size of struct SaveData: %zu\n", compressedDataSize);

  if (compressedDataSize < dataSize) {
    pData = compressedData;
    dataSize = compressedDataSize;
    puts("Using compressed data.");
  } else {
    puts("Using uncompressed data.");
  }
  putchar('\n');

  size_t passwordSize;
  char *password = EncodeData(pData, dataSize, &passwordSize);
  free(compressedData);
  if (!password) {
    return 1;
  }

  // TODO: Add decompression
  printf("Password: %s\n\n", password);
  free(password);

  return 0;
}
