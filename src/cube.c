#include <stdbool.h>
#include <stdint.h>

#include "Libs/BIOS.h"
#include "Libs/GPUDoubleBuffer.h"
#include "Libs/LCD.h"
#include "Libs/VIA.h"

enum CubeConfig {
    TARGET_FPS   = 30,
    VIA_T1_COUNT = 33333,
    C_SIZE       = 22,
};

static const int16_t SIN_TABLE[256] = {
    0,    2,    5,    8,    10,   13,   16,   18,   21,   24,   26,   29,
    31,   34,   37,   39,   42,   44,   47,   49,   51,   54,   56,   58,
    61,   63,   65,   67,   69,   71,   73,   75,   77,   79,   81,   83,
    85,   86,   88,   89,   91,   92,   94,   95,   97,   98,   99,   100,
    101,  102,  103,  104,  105,  106,  106,  107,  107,  108,  108,  109,
    109,  109,  109,  109,  110,  109,  109,  109,  109,  109,  108,  108,
    107,  107,  106,  106,  105,  104,  103,  102,  101,  100,  99,   98,
    97,   95,   94,   92,   91,   89,   88,   86,   85,   83,   81,   79,
    77,   75,   73,   71,   69,   67,   65,   63,   61,   58,   56,   54,
    51,   49,   47,   44,   42,   39,   37,   34,   31,   29,   26,   24,
    21,   18,   16,   13,   10,   8,    5,    2,    0,    -2,   -5,   -8,
    -10,  -13,  -16,  -18,  -21,  -24,  -26,  -29,  -31,  -34,  -37,  -39,
    -42,  -44,  -47,  -49,  -51,  -54,  -56,  -58,  -61,  -63,  -65,  -67,
    -69,  -71,  -73,  -75,  -77,  -79,  -81,  -83,  -85,  -86,  -88,  -89,
    -91,  -92,  -94,  -95,  -97,  -98,  -99,  -100, -101, -102, -103, -104,
    -105, -106, -106, -107, -107, -108, -108, -109, -109, -109, -109, -109,
    -110, -109, -109, -109, -109, -109, -108, -108, -107, -107, -106, -106,
    -105, -104, -103, -102, -101, -100, -99,  -98,  -97,  -95,  -94,  -92,
    -91,  -89,  -88,  -86,  -85,  -83,  -81,  -79,  -77,  -75,  -73,  -71,
    -69,  -67,  -65,  -63,  -61,  -58,  -56,  -54,  -51,  -49,  -47,  -44,
    -42,  -39,  -37,  -34,  -31,  -29,  -26,  -24,  -21,  -18,  -16,  -13,
    -10,  -8,   -5,   -2,
};

#define ISIN(x) (SIN_TABLE[(uint8_t)(x)])
#define ICOS(x) (SIN_TABLE[(uint8_t)((x) + 64)])

void via_init(void) {
    via_disable_interrupt(0x7FU);
    VIA_ACR = 0x40U;
    via_set_timer1(VIA_T1_COUNT);
    asm("cli");
}

static int8_t vx[8];
static int8_t vy[8];
static int8_t vz[8];
static int8_t px[8];
static int8_t py[8];

static const int16_t cvx[8] = {-C_SIZE, C_SIZE, C_SIZE, -C_SIZE,
                               -C_SIZE, C_SIZE, C_SIZE, -C_SIZE,};
static const int16_t cvy[8] = {-C_SIZE, -C_SIZE, C_SIZE, C_SIZE,
                               -C_SIZE, -C_SIZE, C_SIZE, C_SIZE,};
static const int16_t cvz[8] = {-C_SIZE, -C_SIZE, -C_SIZE, -C_SIZE,
                               C_SIZE,  C_SIZE,  C_SIZE,  C_SIZE,};

static const uint8_t faces[12][3] = {
    {4, 5, 6}, {4, 6, 7},
    {1, 0, 3}, {1, 3, 2},
    {5, 1, 2}, {5, 2, 6},
    {0, 4, 7}, {0, 7, 3},
    {7, 6, 2}, {7, 2, 3},
    {0, 1, 5}, {0, 5, 4},
};

static const uint8_t face_colors[12] = {
    0xE0U, 0xE0U, 0x03U, 0x03U, 0xE3U, 0xE3U,
    0x1CU, 0x1CU, 0xFCU, 0xFCU, 0x93U, 0x93U,
};

typedef struct {
    uint8_t id;
    int16_t z_depth;
} RenderFace;

static RenderFace visible_faces[12];
static uint16_t rng_state = 0xACE1U;

static uint16_t next_random(void) {
    rng_state ^= rng_state << 7U;
    rng_state ^= rng_state >> 9U;
    rng_state ^= rng_state << 8U;
    return rng_state;
}

void rotate_all(uint8_t ax, uint8_t ay, uint8_t az) {
    int16_t sin_x = ISIN(ax);
    int16_t cos_x = ICOS(ax);
    int16_t sin_y = ISIN(ay);
    int16_t cos_y = ICOS(ay);
    int16_t sin_z = ISIN(az);
    int16_t cos_z = ICOS(az);

    for (int16_t i = 0; i < 8; ++i) {
        int16_t x = cvx[i];
        int16_t y = cvy[i];
        int16_t z = cvz[i];

        int16_t ty = ((y * cos_x) - (z * sin_x)) / 128;
        int16_t tz = ((y * sin_x) + (z * cos_x)) / 128;
        y = ty;
        z = tz;

        int16_t tx = ((x * cos_y) + (z * sin_y)) / 128;
        tz = ((-x * sin_y) + (z * cos_y)) / 128;
        x = tx;
        z = tz;

        tx = ((x * cos_z) - (y * sin_z)) / 128;
        ty = ((x * sin_z) + (y * cos_z)) / 128;

        vx[i] = (int8_t)tx;
        vy[i] = (int8_t)ty;
        vz[i] = (int8_t)z;
    }
}

void project_all(void) {
    const int16_t distance = 80;
    const int16_t factor = 60;

    for (int16_t i = 0; i < 8; ++i) {
        int16_t z_shifted = vz[i] + distance;
        if (z_shifted < 10) {
            z_shifted = 10;
        }

        int16_t factor_precalc = vx[i] * factor;
        px[i] = (int8_t)((factor_precalc / z_shifted) + (GPU_WIDTH / 2));

        factor_precalc = vy[i] * factor;
        py[i] = (int8_t)((GPU_HEIGHT / 2) - (factor_precalc / z_shifted));
    }
}

void init_graphics(void) {
    INIT_BUFFER();
    lcd_init();
    gpu_fill_screen(0x00U);
}

static const uint8_t BLACK_COLOR = 0x00U;

int main(void) {
    int8_t clear_min_x = 0;
    int8_t clear_max_x = 0;
    int8_t clear_min_y = 0;
    int8_t clear_max_y = 0;

    uint16_t seed = 0;

    init_graphics();
    via_init();

    print_str("PRESS ANY KEY TO GENERATE SEED: ");

    while (bios_getchar_nb() == 0) {
        seed++;
    }

    if (seed != 0U) {
        rng_state = seed;
    }

    gpu_fill_screen(0x00U);
    lcd_print("Rotating Cube");

    uint8_t ang_x = (uint8_t)next_random();
    uint8_t ang_y = (uint8_t)next_random();
    uint8_t ang_z = (uint8_t)next_random();

    int8_t speed_x = (int8_t)((next_random() % 8U) + 1U);
    if (((unsigned int)next_random() & 1U) != 0U) {
        speed_x = (int8_t)-speed_x;
    }

    int8_t speed_y = (int8_t)((next_random() % 8U) + 1U);
    if (((unsigned int)next_random() & 1U) != 0U) {
        speed_y = (int8_t)-speed_y;
    }

    int8_t speed_z = (int8_t)((next_random() % 8U) + 1U);
    if (((unsigned int)next_random() & 1U) != 0U) {
        speed_z = (int8_t)-speed_z;
    }

    while (true) {
        if (clear_max_x >= clear_min_x && clear_max_y >= clear_min_y) {
            gpu_draw_rect(clear_min_x, clear_min_y,
                          (int8_t)(clear_max_x - clear_min_x + 1),
                          (int8_t)(clear_max_y - clear_min_y + 1),
                          BLACK_COLOR);
        }

        ang_x = (uint8_t)(ang_x + speed_x);
        ang_y = (uint8_t)(ang_y + speed_y);
        ang_z = (uint8_t)(ang_z + speed_z);

        rotate_all(ang_x, ang_y, ang_z);
        project_all();

        int8_t min_x = 127;
        int8_t max_x = -128;
        int8_t min_y = 127;
        int8_t max_y = -128;

        for (int16_t i = 0; i < 8; ++i) {
            if (px[i] < min_x) {
                min_x = px[i];
            }
            if (px[i] > max_x) {
                max_x = px[i];
            }
            if (py[i] < min_y) {
                min_y = py[i];
            }
            if (py[i] > max_y) {
                max_y = py[i];
            }
        }

        if (min_x < 0) {
            min_x = 0;
        }
        if (max_x >= GPU_WIDTH) {
            max_x = GPU_WIDTH - 1;
        }
        if (min_y < 0) {
            min_y = 0;
        }
        if (max_y >= GPU_HEIGHT) {
            max_y = GPU_HEIGHT - 1;
        }

        clear_min_x = min_x;
        clear_max_x = max_x;
        clear_min_y = min_y;
        clear_max_y = max_y;

        int16_t count = 0;
        for (int16_t i = 0; i < 12; ++i) {
            uint8_t p1 = faces[i][0];
            uint8_t p2 = faces[i][1];
            uint8_t p3 = faces[i][2];

            int16_t v1x = (int16_t)(px[p2] - px[p1]);
            int16_t v1y = (int16_t)(py[p2] - py[p1]);
            int16_t v2x = (int16_t)(px[p3] - px[p1]);
            int16_t v2y = (int16_t)(py[p3] - py[p1]);

            int32_t cross = ((int32_t)v1x * v2y) - ((int32_t)v1y * v2x);

            if (cross >= 0) {
                visible_faces[count].id = (uint8_t)i;
                visible_faces[count].z_depth = (int16_t)(vz[p1] + vz[p2] + vz[p3]);
                count++;
            }
        }

        for (int16_t i = 1; i < count; ++i) {
            RenderFace temp_face = visible_faces[i];
            int16_t j_sort = i - 1;

            while (j_sort >= 0 && visible_faces[j_sort].z_depth < temp_face.z_depth) {
                visible_faces[j_sort + 1] = visible_faces[j_sort];
                j_sort--;
            }
            visible_faces[j_sort + 1] = temp_face;
        }

        for (int16_t i = 0; i < count; ++i) {
            uint8_t f = visible_faces[i].id;
            uint8_t p1 = faces[f][0];
            uint8_t p2 = faces[f][1];
            uint8_t p3 = faces[f][2];

            gpu_draw_tri(
                px[p1], py[p1],
                px[p2], py[p2],
                px[p3], py[p3],
                face_colors[f],
                true
            );
        }
        drawFrame();
        via_wait_frame();
    }
    return 0;
}