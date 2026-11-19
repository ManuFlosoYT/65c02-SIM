#include "Libs/BIOS.h"

enum MatrixConfig {
    MAT_DIM = 4,
};

static const uint8_t MAT_A[MAT_DIM][MAT_DIM] = {
    {1, 2, 0, 1},
    {0, 1, 0, 1},
    {1, 0, 1, 0},
    {0, 1, 0, 0},
};

static const uint8_t MAT_B[MAT_DIM][MAT_DIM] = {
    {0, 1, 0, 1},
    {1, 0, 1, 0},
    {0, 1, 1, 1},
    {1, 0, 0, 0},
};

static const uint8_t MAT_C[MAT_DIM][MAT_DIM] = {
    {1, 0, 1, 0},
    {0, 1, 0, 0},
    {1, 0, 0, 1},
    {0, 1, 1, 0},
};

int main(void) {
    INIT_BUFFER();

    uint16_t mat_temp[MAT_DIM][MAT_DIM];
    uint16_t mat_final[MAT_DIM][MAT_DIM];
    uint16_t sum_total = 0;

    for (uint16_t i = 0; i < MAT_DIM; i++) {
        for (uint16_t j = 0; j < MAT_DIM; j++) {
            uint16_t sum_temp = 0;
            for (uint16_t k = 0; k < MAT_DIM; k++) {
                sum_temp += (uint16_t)MAT_A[i][k] * (uint16_t)MAT_B[k][j];
            }
            mat_temp[i][j] = sum_temp;
        }
    }

    for (uint16_t i = 0; i < MAT_DIM; i++) {
        for (uint16_t j = 0; j < MAT_DIM; j++) {
            uint16_t sum_temp = 0;
            for (uint16_t k = 0; k < MAT_DIM; k++) {
                sum_temp += mat_temp[i][k] * (uint16_t)MAT_C[k][j];
            }
            mat_final[i][j] = sum_temp;
        }
    }

    print_str("Resulting Matrix:\n");
    for (uint16_t i = 0; i < MAT_DIM; i++) {
        bios_putchar('[');
        for (uint16_t j = 0; j < MAT_DIM; j++) {
            print_num(mat_final[i][j]);
            if (j < 3) {
                print_str(", ");
            }
        }
        print_str("]\n");
    }
    print_str("\n");

    for (uint16_t i = 0; i < MAT_DIM; i++) {
        for (uint16_t j = 0; j < MAT_DIM; j++) {
            sum_total += mat_final[i][j];
        }
    }

    print_str("Total Sum: ");
    print_num(sum_total);
    print_str("\n");

    return 0;
}