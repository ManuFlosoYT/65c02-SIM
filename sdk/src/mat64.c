#include <stdint.h>

#include "Libs/BIOS.h"

typedef struct {
    uint32_t lo;
    uint32_t hi;
} Uint64;

enum Matrix64Config {
    MAT_DIM = 4,
};

static Uint64 matA[MAT_DIM][MAT_DIM];
static Uint64 matB[MAT_DIM][MAT_DIM];
static Uint64 matC[MAT_DIM][MAT_DIM];
static Uint64 matTemp[MAT_DIM][MAT_DIM];
static Uint64 matFinal[MAT_DIM][MAT_DIM];

void set64_direct(Uint64* dest, uint32_t h, uint32_t l) {
    dest->hi = h;
    dest->lo = l;
}

void copy64(Uint64* dest, const Uint64* src) {
    dest->lo = src->lo;
    dest->hi = src->hi;
}

void add64(const Uint64* a, const Uint64* b, Uint64* res) {
    uint32_t rLo = a->lo + b->lo;
    uint32_t rHi = a->hi + b->hi;
    if (rLo < a->lo) {
        rHi++;
    }
    res->lo = rLo;
    res->hi = rHi;
}

void mul64(const Uint64* a, const Uint64* b, Uint64* res) {
    Uint64 tA;
    Uint64 tB;
    res->lo = 0U;
    res->hi = 0U;
    copy64(&tA, a);
    copy64(&tB, b);

    while (tB.lo != 0U || tB.hi != 0U) {
        if ((tB.lo & 1U) != 0U) {
            add64(res, &tA, res);
        }
        if ((tA.lo & 0x80000000UL) != 0U) {
            tA.hi = (tA.hi << 1U) | 1U;
        } else {
            tA.hi = (tA.hi << 1U);
        }
        tA.lo = (tA.lo << 1U);

        if ((tB.hi & 1U) != 0U) {
            tB.lo = (tB.lo >> 1U) | 0x80000000UL;
        } else {
            tB.lo = (tB.lo >> 1U);
        }
        tB.hi = (tB.hi >> 1U);
    }
}

void print_u64_hex(const Uint64* n) {
    print_hex_byte((uint8_t)(n->hi >> 24U));
    print_hex_byte((uint8_t)(n->hi >> 16U));
    print_hex_byte((uint8_t)(n->hi >> 8U));
    print_hex_byte((uint8_t)n->hi);

    print_hex_byte((uint8_t)(n->lo >> 24U));
    print_hex_byte((uint8_t)(n->lo >> 16U));
    print_hex_byte((uint8_t)(n->lo >> 8U));
    print_hex_byte((uint8_t)n->lo);
}

void init_matrices(void) {
    set64_direct(&matA[0][0], 0x00000000UL, 0x00000005UL);
    set64_direct(&matA[0][1], 0x00000000UL, 0xFFFFFFFFUL);
    set64_direct(&matA[0][2], 0x00000001UL, 0x00000000UL);
    set64_direct(&matA[0][3], 0xAAAAAAAAUL, 0x55555555UL);

    set64_direct(&matA[1][0], 0x00000010UL, 0x00000000UL);
    set64_direct(&matA[1][1], 0x00000100UL, 0x00000001UL);
    set64_direct(&matA[1][2], 0x00001000UL, 0x00000002UL);
    set64_direct(&matA[1][3], 0x00010000UL, 0x00000003UL);

    set64_direct(&matA[2][0], 0x00100000UL, 0x00000004UL);
    set64_direct(&matA[2][1], 0x01000000UL, 0x00000005UL);
    set64_direct(&matA[2][2], 0x10000000UL, 0x00000006UL);
    set64_direct(&matA[2][3], 0x20000000UL, 0x00000007UL);

    set64_direct(&matA[3][0], 0x40000000UL, 0x00000008UL);
    set64_direct(&matA[3][1], 0x80000000UL, 0x00000009UL);
    set64_direct(&matA[3][2], 0xFFFFFFFFUL, 0xFFFFFFF0UL);
    set64_direct(&matA[3][3], 0x12345678UL, 0x9ABCDEF0UL);

    for (uint16_t i = 0; i < MAT_DIM; i++) {
        for (uint16_t j = 0; j < MAT_DIM; j++) {
            if (i == j) {
                set64_direct(&matB[i][j], 0x00000002UL, 0x00000000UL);
            } else {
                set64_direct(&matB[i][j], 0x00000000UL, 0x00000001UL);
            }
        }
    }

    for (uint16_t i = 0; i < MAT_DIM; i++) {
        for (uint16_t j = 0; j < MAT_DIM; j++) {
            set64_direct(&matC[i][j], 0x00000000UL, 0x00000002UL);
        }
    }
}

int main(void) {
    Uint64 term;
    Uint64 acc;

    INIT_BUFFER();

    print_str("Starting Massive Calc 64-bit...\n");
    print_str("NOTE: This may take several seconds...\n");
    init_matrices();

    print_str("Calculating AxB...\n");
    for (uint16_t i = 0; i < MAT_DIM; i++) {
        for (uint16_t j = 0; j < MAT_DIM; j++) {
            set64_direct(&acc, 0U, 0U);
            for (uint16_t k = 0; k < MAT_DIM; k++) {
                mul64(&matA[i][k], &matB[k][j], &term);
                add64(&acc, &term, &acc);
            }
            copy64(&matTemp[i][j], &acc);
        }
        bios_putchar('.');
    }
    print_str("\nAxB Ready.\n");

    print_str("Calculating (AxB)xC...\n");
    for (uint16_t i = 0; i < MAT_DIM; i++) {
        for (uint16_t j = 0; j < MAT_DIM; j++) {
            set64_direct(&acc, 0U, 0U);
            for (uint16_t k = 0; k < MAT_DIM; k++) {
                mul64(&matTemp[i][k], &matC[k][j], &term);
                add64(&acc, &term, &acc);
            }
            copy64(&matFinal[i][j], &acc);
        }
        bios_putchar('.');
    }
    print_str("\nFinished.\n");

    print_str("Resulting Matrix (Hex 64bit):\n");
    for (uint16_t i = 0; i < MAT_DIM; i++) {
        bios_putchar('[');
        for (uint16_t j = 0; j < MAT_DIM; j++) {
            print_u64_hex(&matFinal[i][j]);
            if (j < 3) {
                print_str(", ");
            }
        }
        print_str("]\n");
    }

    return 0;
}