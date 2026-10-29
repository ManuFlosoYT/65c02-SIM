#include <stdbool.h>
#include <stdint.h>

#include "Libs/app-bios.h"
#include "Libs/GPUDoubleBuffer.h"
#include "Libs/LCD.h"

enum RoomConfig {
    NUM_VERTICES = 21,
    NUM_FACES    = 30,
};

static const int8_t SIN_TABLE[256] = {
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

static int8_t vx[NUM_VERTICES];
static int8_t vy[NUM_VERTICES];
static int8_t vz[NUM_VERTICES];
static int8_t px[NUM_VERTICES];
static int8_t py[NUM_VERTICES];

static const int8_t cvx[NUM_VERTICES] = {
    -100, 100, 100, -100, -100, 100, 100, -100,
    -80, -20, -20, -80, -80, -20, -20, -80,
    20, 80, 80, 20, 50,
};

static const int8_t cvy[NUM_VERTICES] = {
    -30, -30, 90, 90, -30, -30, 90, 90,
    -30, -30, 30, 30, -30, -30, 30, 30,
    -30, -30, -30, -30, 50,
};

static const int8_t cvz[NUM_VERTICES] = {
    -100, -100, -100, -100, 100, 100, 100, 100,
    -40, -40, -40, -40, 20, 20, 20, 20,
    -40, -40, 20, 20, -10,
};

static const uint8_t faces[NUM_FACES][3] = {
    {4, 7, 6},
    {4, 6, 5},
    {0, 4, 5},
    {0, 5, 1},
    {3, 2, 6},
    {3, 6, 7},
    {4, 0, 3},
    {4, 3, 7},
    {1, 5, 6},
    {1, 6, 2},
    {0, 1, 2},
    {0, 2, 3},
    {12, 13, 14},
    {12, 14, 15},
    {9, 8, 11},
    {9, 11, 10},
    {13, 9, 10},
    {13, 10, 14},
    {8, 12, 15},
    {8, 15, 11},
    {15, 14, 10},
    {15, 10, 11},
    {8, 9, 13},
    {8, 13, 12},
    {16, 19, 18},
    {16, 18, 17},
    {16, 20, 17},
    {17, 20, 18},
    {18, 20, 19},
    {19, 20, 16},
};

static const uint8_t face_colors[NUM_FACES] = {
    0x92U, 0x92U, 0x54U, 0x54U, 0x3FU, 0x3FU, 0x4AU, 0x4AU, 0x4AU, 0x4AU, 0x92U, 0x92U,
    0x03U, 0x03U, 0x03U, 0x03U, 0x07U, 0x07U, 0x23U, 0x23U, 0x47U, 0x47U, 0x02U, 0x02U,
    0x1CU, 0x1CU, 0x0CU, 0x1DU, 0x09U, 0x18U,
};

typedef struct {
    uint8_t id;
    int16_t z_depth;
} RenderFace;

static RenderFace visible_faces[NUM_FACES];

void rotate_all(uint8_t ax, uint8_t ay, uint8_t az) {
    int8_t sin_x = ISIN(ax);
    int8_t cos_x = ICOS(ax);
    int8_t sin_y = ISIN(ay);
    int8_t cos_y = ICOS(ay);
    int8_t sin_z = ISIN(az);
    int8_t cos_z = ICOS(az);

    for (int8_t i = 0; i < NUM_VERTICES; ++i) {
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
    const int16_t distance = 150;
    const int8_t factor = 120;

    for (int8_t i = 0; i < NUM_VERTICES; ++i) {
        int16_t z_shifted = vz[i] + distance;
        if (z_shifted < 5) {
            z_shifted = 5;
        }

        int16_t factor_precalc = vx[i] * factor;
        int16_t p = (factor_precalc / z_shifted) + (GPU_WIDTH / 2);
        if (p > 125) {
            p = 125;
        }
        if (p < -125) {
            p = -125;
        }
        px[i] = (int8_t)p;

        factor_precalc = vy[i] * factor;
        p = (GPU_HEIGHT / 2) - (factor_precalc / z_shifted);
        if (p > 125) {
            p = 125;
        }
        if (p < -125) {
            p = -125;
        }
        py[i] = (int8_t)p;
    }
}

void init_graphics(void) {
    lcd_init();
    gpu_fill_screen(0x00U);
}

int main(void) {
    uint8_t ang_y = 0;

    init_graphics();
    lcd_print("Room 3D");
    println("Ctrl + C to exit");

    while (true) {
        if (bios_getchar_nb() == 3) {
            break;
        }

        ang_y = (uint8_t)(ang_y + 1U);

        rotate_all(0, ang_y, 0);
        project_all();

        gpu_fill_screen(0x00U);

        int8_t count = 0;
        for (int8_t i = 0; i < NUM_FACES; ++i) {
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

        for (int8_t i = 1; i < count; ++i) {
            RenderFace temp_face = visible_faces[i];
            int8_t j_sort = i - 1;

            while (j_sort >= 0 &&
                   visible_faces[j_sort].z_depth < temp_face.z_depth) {
                visible_faces[j_sort + 1] = visible_faces[j_sort];
                j_sort--;
            }
            visible_faces[j_sort + 1] = temp_face;
        }

        for (int8_t i = 0; i < count; ++i) {
            uint8_t f = visible_faces[i].id;
            uint8_t p1 = faces[f][0];
            uint8_t p2 = faces[f][1];
            uint8_t p3 = faces[f][2];

            gpu_draw_tri(px[p1], py[p1], px[p2], py[p2], px[p3], py[p3],
                         face_colors[f], true);
        }

        drawFrame();
    }
    return 0;
}

