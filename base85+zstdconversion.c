#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zstd.h>

#define base 85

struct  __attribute__((packed, scalar_storage_order("little-endian"))) SaveData {
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

static inline size_t min(size_t a, size_t b) {
  return a < b ? a : b;
}

static inline char GetChar(uint_fast8_t val) {
  if (val > base) {
    return ' ';
  }

  return '!' + val;
}

static inline uint_fast8_t GetVal(char c) {
  if ('!' > c || base + '!' <= c) {
    return UINT_FAST8_MAX;
  }

  return c - '!';
}

static inline void PrintSaveData(const struct SaveData * data) {
  printf(
    "data = { %" PRIu16 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu16 ", %"
      PRIu8 ", %" PRIi8 ", %" PRIu32 ", %" PRIi32 ", %" PRIi8 ", %" PRIu64 " }\n\n",
    data->a, data->b, data->c, data->d, data->e, data->f, data->g, data->h, data->i,
    data->j, data->k
  );
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

static char *CompressAndEncodeData(const void *data, size_t dataSize) {
  size_t maxCompressedDataSize, compressedDataSize;
  void *compressedData = CompressData(data, dataSize, &maxCompressedDataSize, &compressedDataSize);
  if (!compressedData) {
    return NULL;
  }

  // TODO: Avoid "passing argument 1 of ‘PrintSaveData’ from incompatible scalar storage order" warning when compiling for big endian systems
  // Also make sure this code works at all there
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wscalar-storage-order"
  PrintSaveData(data);
#pragma GCC diagnostic pop 


  printf("Size of struct SaveData: %zu\n", dataSize);
  printf("Max compressed size of struct SaveData: %zu\n", maxCompressedDataSize);
  printf("Actual compressed size of struct SaveData: %zu\n", compressedDataSize);

  if (compressedDataSize < dataSize) {
    data = compressedData;
    dataSize = compressedDataSize;
    puts("Using compressed data.");
  } else {
    puts("Using uncompressed data.");
  }
  putchar('\n');

  size_t passwordSize;
  char *password = EncodeData(data, dataSize, &passwordSize);
  free(compressedData);
  if (!password) {
    return NULL;
  }

  return password;
}


static void *DecodeData(const char* password, size_t *dataSize) {
  if (!password || !dataSize) {
    return NULL;
  }

  size_t requiredPasswordSize = 5 * ceil(sizeof(struct SaveData) / 4.) + 1;
  size_t passwordSize = strlen(password);

  uint_fast32_t powers[] = {
    pow(base, 0),
    pow(base, 1),
    pow(base, 2),
    pow(base, 3),
    pow(base, 4)
  };

  // Can't compare to requiredPasswordSize yet as that is for uncompressed data
  if (passwordSize % 5 != 0) {
    puts("Password length is invalid");
    return NULL;
  }

  *dataSize = 4 * passwordSize / 5;
  uint32_t *decodedData = malloc(*dataSize);
  if (!decodedData) {
    puts("Failed to allocate memory for decoded data");
    return NULL;
  }

  for (size_t i = 0; i < passwordSize; i += 5) {
    printf("\n%zu-%zu: %.*s\n", i, i + 4, 5, password + i);

    uint_fast8_t digit0 = GetVal(password[i]); 
    uint_fast8_t digit1 = GetVal(password[i + 1]);
    uint_fast8_t digit2 = GetVal(password[i + 2]);
    uint_fast8_t digit3 = GetVal(password[i + 3]);
    uint_fast8_t digit4 = GetVal(password[i + 4]);
    if (UINT_FAST8_MAX == digit0 || UINT_FAST8_MAX == digit1 || UINT_FAST8_MAX == digit2
        || UINT_FAST8_MAX == digit3 || UINT_FAST8_MAX == digit4) {
      puts("Password char not in range");
      return NULL;
    }

    decodedData[i / 5] = digit4 * powers[4] + digit3 * powers[3] + digit2 * powers[2] + digit1 * powers[1] + digit0 * powers[0];

    printf(
      "%zu-%zu: "
      "%" PRIuFAST8 " * %" PRIuFAST8 "^%i + "
      "%" PRIuFAST8 " * %" PRIuFAST8 "^%i + "
      "%" PRIuFAST8 " * %" PRIuFAST8 "^%i + "
      "%" PRIuFAST8 " * %" PRIuFAST8 "^%i + "
      "%" PRIuFAST8 " * %" PRIuFAST8 "^%i = "
      "%" PRIu32 "\n",
      i, i + 4,
      digit4, base, 4,
      digit3, base, 3,
      digit2, base, 2,
      digit1, base, 1,
      digit0, base, 0,
      decodedData[i / 5]
    );
  }

  return decodedData;
}

static void *DecompressData(const void *data, size_t dataSize) {
  if (!data) {
    return NULL;
  }

  unsigned long long decompressedDataSize = ZSTD_getFrameContentSize(data);
  if (ZSTD_CONTENTSIZE_UNKNOWN == dataSize || ZSTD_CONTENTSIZE_ERROR == dataSize) {
    puts("Unable to identify compressed data, not compressed?");
    return NULL;
  }

  void *decompressedData = malloc(decompressedDataSize);
  if (!decompressedData) {
    puts("Unable to allocate room for decompressed data");
    return NULL;
  }

  size_t res = ZSTD_decompress(decompressedData, decompressedDataSize, data, dataSize);
  if (ZSTD_isError(res)) {
    free(decompressedData);
    return NULL;
  }
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

  void *pData = (void *)&data;
  size_t dataSize = sizeof data;

  // For testing
  dataSize *= 2;
  pData = malloc(dataSize);
  memcpy(pData, &data, sizeof data);
  memcpy(pData + sizeof data, &data, sizeof data);

  char *password = CompressAndEncodeData(pData, dataSize);
  if (!password) {
    return 1;
  }
  printf("\nPassword: %s\n\n", password);

  size_t decodedPasswordSize;
  void *decodedPassword = DecodeData(password, &decodedPasswordSize);
  free(password);
  if (!decodedPassword) {
    puts("Failed to decode password.");
    return 1;
  }


  return 0;
}
