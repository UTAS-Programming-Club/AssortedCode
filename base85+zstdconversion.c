#include <assert.h>   // static_assert
#include <inttypes.h> // PRIu8, PRIu16, PRIu32
#include <math.h>     // pow
#include <stdbool.h>  // bool, false, true
#include <stdint.h>   // uint8_t, uint16_t, uint32_t, uint_fast8_t, UINT_FAST8_MAX
#include <string.h>   // memcpy, strlen
#include <stdio.h>    // puts, printf
#include <stdlib.h>   // free, malloc, NULL, size_t
#include <zstd.h>     // ZSTD_compress, ZSTD_compressBound, ZSTD_CONTENTSIZE_ERROR, ZSTD_CONTENTSIZE_UNKNOWN, ZSTD_decompress, ZSTD_findFrameCompressedSize, ZSTD_getFrameContentSize, ZSTD_isError

#define BASE 85
#define STARTING_CHAR '!'
#define STARTING_CHAR_STR "!"

#define DOUBLE_DATA 0
#define ALLOW_COMPRESSION 1
#define FORCE_COMPRESSION 0


static_assert(sizeof(unsigned char) == sizeof(uint8_t), "Need 8 bit bytes.");

struct __attribute__((packed, scalar_storage_order("little-endian"))) SaveData {
  uint16_t a;
  uint8_t b;
  uint8_t c;
  uint8_t d;
  uint8_t e;
  uint8_t f[7];
  uint32_t g;
};

static inline size_t min(size_t a, size_t b) {
  return a < b ? a : b;
}

// Assumes ascii 
static inline char GetChar(uint_fast8_t val) {
  if (BASE < val) {
    return ' ';
  }

  return STARTING_CHAR + val;
}

// Assumes ascii
static inline uint_fast8_t GetVal(char c) {
  if (STARTING_CHAR > c || BASE + STARTING_CHAR <= c) {
    return UINT_FAST8_MAX;
  }

  return c - '!';
}

static inline void PrintSaveData(const struct SaveData * data, const char *name) {
  printf("%s = { %" PRIu16 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 ", { "
         "%" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8 ", %" PRIu8
         " }, %" PRIu32 " }\n",
         name, data->a, data->b, data->c, data->d, data->e,
         data->f[0], data->f[1], data->f[2], data->f[3], data->f[4], data->f[5], data->f[6],
         data->g);
}


static const void *CompressData(const void *data, size_t dataSize, size_t *maxCompressedDataSize, size_t *compressedDataSize) {
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

static const char *EncodeData(const void *data, size_t dataSize, size_t *passwordSize) {
  if (!data || !passwordSize) {
    return NULL;
  }
  
  // Note that this overestimates if dataSize is not a multiple of 4
  // This is corrected by reversing the order of each group of chars
  // so that the password ends with ! which can then be stripped out
  // to get a password that is shorter by 0 to 4 chars.
  // Integer formula that gives the same result as 5*ceil(size/4.) for integer sizes
  *passwordSize = 5 * ((dataSize + 3) / 4);
  // printf("Expected size of password string: %zu\n", *passwordSize);

  char *password = malloc(*passwordSize + 1);
  if (!password) {
    puts("Failed to allocate room for the password.");
    return NULL;
  }
  char *passwordPos = password;

  uint_fast32_t powers[] = {
    pow(BASE, 0),
    pow(BASE, 1),
    pow(BASE, 2),
    pow(BASE, 3),
    pow(BASE, 4)
  };

  for (size_t i = 0; i < dataSize; i += 4) {
    uint_fast32_t value = 0;
    size_t rem = dataSize - i % dataSize;
    size_t quot = min(rem, 4);
    // printf("\n%zu-%zu: remainder is %zu, encoding %zu bytes\n", i, i + 3, rem, quot);
    memcpy(&value, data + i, quot);

    uint_fast8_t digit4 = value / powers[4];
    uint_fast32_t value4 = value - digit4 * powers[4];
    if (digit4 >= BASE) {
      printf("base %i is too small.\n", BASE);
      free(password);
      return NULL;
    }

    uint_fast8_t digit3 = value4 / powers[3];
    uint_fast32_t value3 = value4 - digit3 * powers[3];
    if (digit3 >= BASE) {
      printf("base %i is too small.\n", BASE);
      free(password);
      return NULL;
    }

    uint_fast8_t digit2 = value3 / powers[2];
    uint_fast32_t value2 = value3 - digit2 * powers[2];
    if (digit2 >= BASE) {
      printf("base %i is too small.\n", BASE);
      free(password);
      return NULL;
    }

    uint_fast8_t digit1 = value2 / powers[1];
    uint_fast32_t value1 = value2 - digit1 * powers[1];
    if (digit1 >= BASE) {
      printf("base %i is too small.\n", BASE);
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
    //   digit4, BASE, 4,
    //   digit3, BASE, 3,
    //   digit2, BASE, 2,
    //   digit1, BASE, 1,
    //   digit0, BASE, 0
    // );

    // printf(
    //   "%zu-%zu: "
    //    "%*s\n", 
    //   i, i + 3, 
    //   5, passwordPos - 5
    // );
  }

  for (int i = 0; i < 4 && passwordPos[-1] == STARTING_CHAR; ++i) {
    --passwordPos;
    --*passwordSize;
  }
  *passwordPos = '\0';

  // printf("Actual size of password string: %zu\n", *passwordSize);

  return password;
}

static const char *CompressAndEncodeData(const void *data, size_t dataSize) {
  if (!data) {
    return NULL;
  }

  size_t encodedDataSize;
  const char *encodedData = EncodeData(data, dataSize, &encodedDataSize);
  if (!encodedData) {
    return NULL;
  }

  size_t maxCompressedDataSize, compressedDataSize;
  const void *compressedData = CompressData(data, dataSize, &maxCompressedDataSize, &compressedDataSize);
  if (!compressedData) {
    free((void *)encodedData);
    return NULL;
  }

  size_t encodedCompressedDataSize;
  const char *encodedCompressedData = EncodeData(compressedData, compressedDataSize, &encodedCompressedDataSize);
  free((void *)compressedData);
  if (!encodedCompressedData) {
    free((void *)encodedData);
    return NULL;
  }

  printf("\nSize of struct SaveData: %zu\n", dataSize);
  printf("Encoded size of struct SaveData: %zu\n", encodedDataSize);
  printf("Max compressed size of struct SaveData: %zu\n", maxCompressedDataSize);
  printf("Actual compressed size of struct SaveData: %zu\n", compressedDataSize);
  printf("Encoded and compressed size of struct SaveData: %zu\n", encodedCompressedDataSize);

  const unsigned char *password;
  if (ALLOW_COMPRESSION && (FORCE_COMPRESSION || encodedCompressedDataSize < encodedDataSize)) {
    password = encodedCompressedData;
    free((void *)encodedData);
    puts("Using compressed data.");
  } else {
    password = encodedData;
    free((void *)encodedCompressedData);
    puts("Using uncompressed data.");
  }

  return password;
}


static const unsigned char *DecodeData(const char* password, size_t *dataSize) {
  if (!password || !dataSize) {
    return NULL;
  }

  size_t passwordSize = strlen(password);
  // Integer formula that gives the same result as 4*ceil(size/5.) for integer sizes
  *dataSize = 4 * ((passwordSize + 4) / 5);

  uint32_t *decodedData = malloc(*dataSize);
  if (!decodedData) {
    puts("Failed to allocate memory for decoded data");
    return NULL;
  }

  uint_fast32_t powers[] = {
    pow(BASE, 0),
    pow(BASE, 1),
    pow(BASE, 2),
    pow(BASE, 3),
    pow(BASE, 4)
  };

  for (size_t i = 0; i < passwordSize; i += 5) {
    char data[5] = STARTING_CHAR_STR STARTING_CHAR_STR STARTING_CHAR_STR STARTING_CHAR_STR STARTING_CHAR_STR;
    uint8_t *pData = (uint8_t *)&data;
    size_t rem = passwordSize - i % passwordSize;
    size_t quot = min(rem, 5);
    // printf("\n%zu-%zu: remainder is %zu, decoding %zu bytes\n", i, i + 4, rem, quot);
    memcpy(pData, password + i, quot);

    // printf("\n%zu-%zu: %.*s\n", i, i + 4, 5, password + i);

    uint_fast8_t digit0 = GetVal(pData[0]);
    uint_fast8_t digit1 = GetVal(pData[1]);
    uint_fast8_t digit2 = GetVal(pData[2]);
    uint_fast8_t digit3 = GetVal(pData[3]);
    uint_fast8_t digit4 = GetVal(pData[4]);
    if (UINT_FAST8_MAX == digit0 || UINT_FAST8_MAX == digit1 || UINT_FAST8_MAX == digit2
        || UINT_FAST8_MAX == digit3 || UINT_FAST8_MAX == digit4) {
      puts("Password char not in range");
      return NULL;
    }

    decodedData[i / 5] = digit4 * powers[4] + digit3 * powers[3] + digit2 * powers[2] + digit1 * powers[1] + digit0 * powers[0];

    // printf(
    //   "%zu-%zu: "
    //   "%" PRIuFAST8 " * %" PRIuFAST8 "^%i + "
    //   "%" PRIuFAST8 " * %" PRIuFAST8 "^%i + "
    //   "%" PRIuFAST8 " * %" PRIuFAST8 "^%i + "
    //   "%" PRIuFAST8 " * %" PRIuFAST8 "^%i + "
    //   "%" PRIuFAST8 " * %" PRIuFAST8 "^%i = "
    //   "%" PRIu32 "\n",
    //   i, i + 4,
    //   digit4, BASE, 4,
    //   digit3, BASE, 3,
    //   digit2, BASE, 2,
    //   digit1, BASE, 1,
    //   digit0, BASE, 0,
    //   decodedData[i / 5]
    // );
  }

  return (unsigned char *)decodedData;
}

static bool DecompressData(const void *data, size_t dataSize, void **decompressedData, size_t *compressedDataSize, unsigned long long *decompressedDataSize) {
  if (!data || !decompressedData) {
    return false;
  }

  *decompressedDataSize = ZSTD_getFrameContentSize(data, dataSize);
  if (ZSTD_CONTENTSIZE_UNKNOWN == *decompressedDataSize) {
    puts("Unable to identify compressed data.");
    return false;
  }

  if (ZSTD_CONTENTSIZE_ERROR == *decompressedDataSize) {
    puts("Unable to identify compressed data, not compressed?");
    *decompressedData = NULL;
    return true;
  }

  *compressedDataSize = ZSTD_findFrameCompressedSize(data, dataSize);
  if (ZSTD_isError(*compressedDataSize)) {
    return false;
  }

  *decompressedData = malloc(*decompressedDataSize);
  if (!*decompressedData) {
    puts("Unable to allocate room for decompressed data.");
    return false;
  }

  size_t res = ZSTD_decompress(*decompressedData, *decompressedDataSize, data, *compressedDataSize);
  if (ZSTD_isError(res)) {
    puts("Upable to decompress password.");
    free(*decompressedData);
    return false;
  }

  return true;
}

static const unsigned char *DecodeAndDecompressData(const char *password) {
  if (!password) {
    return NULL;
  }

  size_t decodedPasswordSize;
  const unsigned char *decodedPassword = DecodeData(password, &decodedPasswordSize);
  if (!decodedPassword) {
    puts("Failed to decode password.");
    return NULL;
  }

  size_t actualCompressedDataSize;
  unsigned long long decompressedDataSize;
  const void *decompressedData;
  bool decompressionResult = DecompressData(decodedPassword, decodedPasswordSize, (void **)&decompressedData, &actualCompressedDataSize, &decompressedDataSize);
  if (!decompressionResult) {
    puts("Failed to decompress password.");
    return NULL;
  }

  printf("Size of password: %zu\n", strlen(password));
  printf("Decoded size of struct SaveData: %zu\n", decodedPasswordSize);

  if (!decompressedData) {
    puts("Assuming uncompressed.");
    return decodedPassword;
  }

  printf("Actual compressed size of struct SaveData: %zu\n", actualCompressedDataSize);
  printf("Decompressed size of struct SaveData: %llu\n\n", decompressedDataSize);

  free((void *)decodedPassword);
  return decompressedData;
}


int main(void) {
  struct SaveData data = {
    .a = 0,
    .b = 1,
    .c = 2,
    .d = 80,
    .e = 100,
    .g = 1,
  };
  data.f[0] = 1;

  void *pData = (void *)&data;
  size_t dataSize = sizeof data;

#if DOUBLE_DATA
  dataSize *= 2;
  pData = malloc(dataSize);
  memcpy(pData, &data, sizeof data);
  memcpy(pData + sizeof data, &data, sizeof data);

  PrintSaveData(pData, "Data 1");
  PrintSaveData(pData + sizeof data, "Data 2");
#else
  pData = malloc(dataSize);
  memcpy(pData, &data, sizeof data);

  PrintSaveData(pData, "Data");
#endif
  putchar('\n');

  const char *password = CompressAndEncodeData(pData, dataSize);
  free(pData);
  if (!password) {
    return 1;
  }
  printf("\nPassword: %s\n\n", password);

  const struct SaveData *decodedData = (const struct SaveData *)DecodeAndDecompressData(password);
  
  free((void *)password);
  if (!decodedData) {
    return 1;
  }

  putchar('\n');
#if DOUBLE_DATA
  PrintSaveData(decodedData, "Decoded data 1");
  PrintSaveData(decodedData + 1, "Decoded data 2");
#else
  PrintSaveData(decodedData, "Decoded data");
#endif
  free((void *)decodedData);

  return 0;
}
