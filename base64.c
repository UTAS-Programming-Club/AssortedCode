#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char AllowedPasswordChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

char *encode(const char *data) {
    size_t dataSize = strlen(data);
    size_t passwordSize = ceil(8. / 6 * dataSize) + 1;
    char *password = malloc(passwordSize);
    // printf("Data size: %zu, password size: %zu\n", dataSize, passwordSize);
    
    size_t i = 0;
    size_t j = 0;
    if (dataSize - i >= 3) {
        for (; dataSize - i >= 3; i += 3, j += 4) {
            // printf("\n%zu-%zu: 0b%08b, 0b%08b, 0b%08b\n", i, i + 2, data[i], data[i + 1], data[i + 2]);
            uint8_t hex1 = data[i] & 0b111111;
            uint8_t hex2 = (data[i + 1] & 0b1111) << 2 | data[i] >> 6;
            uint8_t hex3 = (data[i + 2] & 0b11) << 4 | data[i + 1] >> 4;
            uint8_t hex4 = data[i + 2] >> 2;

            // printf("%zu-%zu: 0b%06b, 0b%06b, 0b%06b, 0b%06b\n", j, j + 3, hex1, hex2, hex3, hex4);
            password[j] = AllowedPasswordChars[hex1];
            password[j + 1] = AllowedPasswordChars[hex2];
            password[j + 2] = AllowedPasswordChars[hex3];
            password[j + 3] = AllowedPasswordChars[hex4];
        }
    }
    if (dataSize - i == 2) {
        //printf("\n%zu-%zu: 0b%08b, 0b%08b\n", i, i + 1, data[i], data[i + 1]);
        uint8_t hex1 = data[i] & 0b111111;
        uint8_t hex2 = (data[i + 1] & 0b1111) << 2 | data[i] >> 6;
        uint8_t hex3 = data[i + 1] >> 4;
        
        // printf("%zu-%zu: 0b%06b, 0b%06b, 0b%06b\n", j, j + 2, hex1, hex2, hex3);
        password[j] = AllowedPasswordChars[hex1];
        password[j + 1] = AllowedPasswordChars[hex2];
        password[j + 2] = AllowedPasswordChars[hex3];
        i += 2;
        j += 3;
    } else if (dataSize - i == 1) {
        // printf("\n%zu: 0b%08b\n", i, data[i]);
        uint8_t hex1 = data[i] & 0b111111;
        uint8_t hex2 = data[i] >> 6;
        
        // printf("%zu-%zu: 0b%06b, 0b%06b\n", j, hex1, hex2);
        password[j] = AllowedPasswordChars[hex1];
        password[j + 1] = AllowedPasswordChars[hex2];
        ++i;
        j += 2;
    }

    password[j] = '\0';
    ++j;

    // printf("\nRemaining data: %zu\n", dataSize - i);
    // printf("Remaining password: %zu\n", passwordSize - j);

    return password;
}

char *decode(const char *password) {
    size_t passwordSize = strlen(password);
    size_t dataSize = floor(6. / 8 * passwordSize) + 1;
    char *data = malloc(dataSize);
    // printf("Data size: %zu, password size: %zu\n", dataSize, passwordSize);

    size_t i = 0;
    size_t j = 0;
    if (passwordSize - i >= 4) {
        for (; passwordSize - i >= 4; i += 4, j += 3) {
            uint8_t hex1 = strchr(AllowedPasswordChars, password[i]) - AllowedPasswordChars;
            uint8_t hex2 = strchr(AllowedPasswordChars, password[i + 1]) - AllowedPasswordChars;
            uint8_t hex3 = strchr(AllowedPasswordChars, password[i + 2]) - AllowedPasswordChars;
            uint8_t hex4 = strchr(AllowedPasswordChars, password[i + 3]) - AllowedPasswordChars;
            // printf("\n%zu-%zu: 0b%06b, 0b%06b, 0b%06b, 0b%06b\n", i, i + 3, hex1, hex2, hex3, hex4);

            data[j] = hex1 | (hex2 & 0b11) << 6;
            data[j + 1] = (hex2 & 0b111100) >> 2 | (hex3 & 0b1111) << 4;
            data[j + 2] = (hex3 & 0b110000) >> 4 | (hex4) << 2;
            // printf("%zu-%zu: 0b%08b, 0b%08b, 0b%08b\n", j, j + 2, data[j], data[j + 1], data[j + 2]);
        }
    }
    if (passwordSize - i == 3) {
            uint8_t hex1 = strchr(AllowedPasswordChars, password[i]) - AllowedPasswordChars;
            uint8_t hex2 = strchr(AllowedPasswordChars, password[i + 1]) - AllowedPasswordChars;
            uint8_t hex3 = strchr(AllowedPasswordChars, password[i + 2]) - AllowedPasswordChars;
            // printf("\n%zu-%zu: 0b%06b, 0b%06b, 0b%06b\n", i, i + 2, hex1, hex2, hex3);

            data[j] = hex1 | (hex2 & 0b11) << 6;
            data[j + 1] = (hex2 & 0b111100) >> 2 | hex3 << 4;
            // printf("%zu-%zu: 0b%08b, 0b%08b\n", j, j + 1, data[j], data[j + 1]);

            i += 3;
            j += 2;
    } else if (passwordSize - i == 2) {
            uint8_t hex1 = strchr(AllowedPasswordChars, password[i]) - AllowedPasswordChars;
            uint8_t hex2 = strchr(AllowedPasswordChars, password[i + 1]) - AllowedPasswordChars;
            // printf("\n%zu-%zu: 0b%06b, 0b%06b\n", i, i + 1, hex1, hex2);

            data[j] = hex1 | (hex2 & 0b11) << 6;
            // printf("%zu: 0b%08b\n", j, data[j]);

            i += 2;
            ++j;
    }

    data[j] = '\0';
    ++j;

    // printf("\nRemaining password: %zu\n", passwordSize - i);
    // printf("Remaining data: %zu\n", dataSize - j);

    return data;
}

int main(void) {

    char data[] = "This is a test!!!!!";
    //uint8_t data[] = {0b11100101, 0b10000100, 0b01100010, 0b11100110, 0b00101011, 0b10101011, 0b10011111};

    char *password = encode(data);
    printf("\n%s -> %s\n", data, password);

    char *decodedData = decode(password);
    printf("\n%s -> %s", password, decodedData);

    free(password);
    free(decodedData);

    return 0;
}


