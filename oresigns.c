#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

constexpr int_fast16_t INVALID_HEIGHT = INT_FAST16_MIN;

struct OreInfo {
    const char *name;
    int_fast16_t lowerPoor;
    int_fast16_t lowerOkay;
    int_fast16_t lowerGood;
    int_fast16_t ideal;
    int_fast16_t upperGood;
    int_fast16_t upperOkay;
    int_fast16_t upperPoor;
    int_fast16_t lowerPoor2;
    int_fast16_t lowerOkay2;
    int_fast16_t upperOkay2;
    int_fast16_t upperPoor2;
};

const struct OreInfo OreInfo[] = {
    {"Coal", -3, 19, 30, 45, 59, 66, 187, INVALID_HEIGHT, INVALID_HEIGHT, INVALID_HEIGHT, INVALID_HEIGHT},
    {"Copper", -19, 8, 20, 44, 50, 58, 110, INVALID_HEIGHT, INVALID_HEIGHT, INVALID_HEIGHT, INVALID_HEIGHT},
    {"Iron", -63, -11, 8, 15, 26, 36, 72, 78, INVALID_HEIGHT, INVALID_HEIGHT, 179},
    {"Lapis", -63, -17, 8, 9, 18, 28, 64, INVALID_HEIGHT, INVALID_HEIGHT, INVALID_HEIGHT, INVALID_HEIGHT},
    {"Gold", -63, -54, -17, -16, -5, -49,  29, 19, -43, 6, 32},
    {"Diamond", -63, -45, -54, -53, -46, -9, 15, INVALID_HEIGHT, INVALID_HEIGHT, INVALID_HEIGHT, INVALID_HEIGHT},
    {"Redstone", -63, -47, -54, -53, -48, -41, 15, INVALID_HEIGHT, INVALID_HEIGHT, INVALID_HEIGHT, INVALID_HEIGHT}
};


int main(int argc, char *argv[static argc + 1]) {
    if (argc != 2) {
        printf("%s height\n", argv[0]);
        return 1;
    }

    char *end;
    long height = strtol(argv[1], &end, 10);
    if (end == argv[1]) {
        printf("Expected height, received %s\n", argv[1]);
        return 1;
    }

    size_t oreCount = sizeof OreInfo / sizeof *OreInfo;

    puts("Left Signs:\n");
    printf("Height: %ld\n", height);
    for (size_t i = 0; i < oreCount; ++i) {
        printf("%s:\n", OreInfo[i].name);
    }
    putchar('\n');

    puts("Right Signs:\n");

    time_t timestamp;
    if (time(&timestamp) == (time_t)(-1)) {
        return -1;
    }
    struct tm date;
    if (localtime_r(&timestamp, &date) == nullptr) {
        return -1;
    }
    printf("          %d%02d%02d|\n", 1900 + date.tm_year, date.tm_mon + 1, date.tm_mday);

    for (size_t i = 0; i < oreCount; ++i) {
        struct OreInfo info = OreInfo[i];

        if (height == info.ideal) {
            printf("Ideal");
        } else if (height >= info.lowerGood && height <= info.upperGood) {
            printf("Good");
        } else if ((height >= info.lowerOkay && height <= info.upperOkay) ||
                   (info.lowerOkay2 != INVALID_HEIGHT && height >= info.lowerOkay2 && height <= info.upperOkay2)) {
            printf("Okay");
        } else if ((height >= info.lowerPoor && height <= info.upperPoor) ||
                   (info.lowerPoor2 != INVALID_HEIGHT && height >= info.lowerPoor2 && height <= info.upperPoor2)) {
            printf("Poor");
        } else {
            printf("None");
        }
        printf(", best: %i", info.ideal);
        
        int_fast16_t padding = 0;
        if (info.ideal >= -9) {
            padding += 2;
            if (info.ideal <= 9) {
                padding += 1;
            }
            if (height == info.ideal) {
                padding -= 1;
            }
        }
        printf("%*s|\n", padding, "");
    }

    return 0;
}
