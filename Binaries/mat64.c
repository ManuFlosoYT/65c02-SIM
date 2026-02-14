#include "Include/BIOS.h"

/* 64-bit Structure */
typedef struct {
    unsigned long lo; /* Bits 0-31 */
    unsigned long hi; /* Bits 32-63 */
} Uint64;

/* Global variables */
unsigned char i, j, k;

Uint64 matA[4][4];
Uint64 matB[4][4];
Uint64 matC[4][4];
Uint64 matTemp[4][4];
Uint64 matFinal[4][4];
Uint64 sum_total;

/* --- 64-Bit Arithmetic (Pointers) --- */

/* Assign massive values directly: hi (32bit) and lo (32bit) */
void set64_direct(Uint64* dest, unsigned long h, unsigned long l) {
    dest->hi = h;
    dest->lo = l;
}

void copy64(Uint64* dest, Uint64* src) {
    dest->lo = src->lo;
    dest->hi = src->hi;
}

/* Sum with carry: *res = *a + *b */
void add64(Uint64* a, Uint64* b, Uint64* res) {
    unsigned long rLo, rHi;
    rLo = a->lo + b->lo;
    rHi = a->hi + b->hi;
    if (rLo < a->lo) { /* Carry from low part */
        rHi++;
    }
    res->lo = rLo;
    res->hi = rHi;
}

/* 64-bit Multiplication (Shift and Add) */
void mul64(Uint64* a, Uint64* b, Uint64* res) {
    Uint64 tA, tB;
    res->lo = 0;
    res->hi = 0;
    copy64(&tA, a);
    copy64(&tB, b);

    while (tB.lo != 0 || tB.hi != 0) {
        if (tB.lo & 1) {
            add64(res, &tA, res);
        }
        /* Shift Left tA */
        if (tA.lo & 0x80000000UL)
            tA.hi = (tA.hi << 1) | 1;
        else
            tA.hi = (tA.hi << 1);
        tA.lo = (tA.lo << 1);

        /* Shift Right tB */
        if (tB.hi & 1)
            tB.lo = (tB.lo >> 1) | 0x80000000UL;
        else
            tB.lo = (tB.lo >> 1);
        tB.hi = (tB.hi >> 1);
    }
}

/* --- Printing --- */

void print_u64_hex(Uint64* n) {
    /* Print HI bytes */
    print_hex_byte((unsigned char)(n->hi >> 24));
    print_hex_byte((unsigned char)(n->hi >> 16));
    print_hex_byte((unsigned char)(n->hi >> 8));
    print_hex_byte((unsigned char)n->hi);
    /* Print LO bytes */
    print_hex_byte((unsigned char)(n->lo >> 24));
    print_hex_byte((unsigned char)(n->lo >> 16));
    print_hex_byte((unsigned char)(n->lo >> 8));
    print_hex_byte((unsigned char)n->lo);
}

/* Initialization with MASSIVE values */
void init_matrices(void) {
    /* Matrix A: Varied patterns */
    /* 1. A "small" number (relative) */
    set64_direct(&matA[0][0], 0x00000000, 0x00000005);
    /* 2. A number bordering the 32-bit limit */
    set64_direct(&matA[0][1], 0x00000000, 0xFFFFFFFF);
    /* 3. A number using ONLY the high part (very large) */
    set64_direct(&matA[0][2], 0x00000001, 0x00000000);
    /* 4. Alternate bit pattern */
    set64_direct(&matA[0][3], 0xAAAAAAAA, 0x55555555);

    /* Fill the rest with large incremental values */
    set64_direct(&matA[1][0], 0x00000010, 0x00000000);
    set64_direct(&matA[1][1], 0x00000100, 0x00000001);
    set64_direct(&matA[1][2], 0x00001000, 0x00000002);
    set64_direct(&matA[1][3], 0x00010000, 0x00000003);

    set64_direct(&matA[2][0], 0x00100000, 0x00000004);
    set64_direct(&matA[2][1], 0x01000000, 0x00000005);
    set64_direct(&matA[2][2], 0x10000000, 0x00000006);
    set64_direct(&matA[2][3], 0x20000000, 0x00000007);

    set64_direct(&matA[3][0], 0x40000000, 0x00000008);
    set64_direct(&matA[3][1], 0x80000000, 0x00000009); /* Active highest bit */
    set64_direct(&matA[3][2], 0xFFFFFFFF, 0xFFFFFFF0); /* Almost max 64bit */
    set64_direct(&matA[3][3], 0x12345678, 0x9ABCDEF0);

    /* Matrix B: Scaled identity and large prime numbers */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (i == j) {
                /* Diagonal: Large values */
                set64_direct(&matB[i][j], 0x00000002, 0x00000000);
            } else {
                /* Rest: Base value 1 */
                set64_direct(&matB[i][j], 0x00000000, 0x00000001);
            }
        }
    }

    /* Matrix C: Simple to avoid execution timeout */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            set64_direct(&matC[i][j], 0x00000000, 0x00000002);
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

    /* 1. Multiply MatA x MatB = MatTemp */
    print_str("Calculating AxB...\n");
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            set64_direct(&acc, 0, 0);
            for (k = 0; k < 4; k++) {
                mul64(&matA[i][k], &matB[k][j], &term);
                add64(&acc, &term, &acc);
            }
            copy64(&matTemp[i][j], &acc);
        }
        bios_putchar('.'); /* Visual progress feedback */
    }
    print_str("\nAxB Ready.\n");

    /* 2. Multiply MatTemp x MatC = MatFinal */
    print_str("Calculating (AxB)xC...\n");
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            set64_direct(&acc, 0, 0);
            for (k = 0; k < 4; k++) {
                mul64(&matTemp[i][k], &matC[k][j], &term);
                add64(&acc, &term, &acc);
            }
            copy64(&matFinal[i][j], &acc);
        }
        bios_putchar('.');
    }
    print_str("\nFinished.\n");

    /* Print Final Matrix */
    print_str("Resulting Matrix (Hex 64bit):\n");
    for (i = 0; i < 4; i++) {
        bios_putchar('[');
        for (j = 0; j < 4; j++) {
            print_u64_hex(&matFinal[i][j]);
            if (j < 3) print_str(", ");
        }
        print_str("]\n");
    }

    return 0;
}