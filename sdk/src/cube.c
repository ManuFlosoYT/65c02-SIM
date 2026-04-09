#include <stdlib.h>

#include "Libs/BIOS.h"
#include "Libs/GPUDoubleBuffer.h"
#include "Libs/LCD.h"
#include "Libs/VIA.h"

#define CLOCK_HZ 1000000UL
#define TARGET_FPS 30
#define VIA_T1_COUNT ((unsigned int)(CLOCK_HZ / TARGET_FPS))

#define FP_SCALE 1

const signed char SIN_TABLE[256] = {
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
    -10,  -8,   -5,   -2};

#define ISIN(x) (SIN_TABLE[(unsigned char)(x)])
#define ICOS(x) (SIN_TABLE[(unsigned char)((x) + 64)])

void via_init(void) {
    via_disable_interrupt(0x7F);
    VIA_ACR = 0x40;
    via_set_timer1(VIA_T1_COUNT);
    asm("cli");
}

signed char vx[8], vy[8], vz[8];
signed char px[8], py[8];

#define C_SIZE 22

const signed char cvx[8] = {-C_SIZE, C_SIZE, C_SIZE, -C_SIZE,
                            -C_SIZE, C_SIZE, C_SIZE, -C_SIZE};
const signed char cvy[8] = {-C_SIZE, -C_SIZE, C_SIZE, C_SIZE,
                            -C_SIZE, -C_SIZE, C_SIZE, C_SIZE};
const signed char cvz[8] = {-C_SIZE, -C_SIZE, -C_SIZE, -C_SIZE,
                            C_SIZE,  C_SIZE,  C_SIZE,  C_SIZE};

const unsigned char faces[12][3] = {
    {4, 5, 6}, {4, 6, 7}, /* Front */
    {1, 0, 3}, {1, 3, 2}, /* Back */
    {5, 1, 2}, {5, 2, 6}, /* Right */
    {0, 4, 7}, {0, 7, 3}, /* Left */
    {7, 6, 2}, {7, 2, 3}, /* Top */
    {0, 1, 5}, {0, 5, 4}  /* Bottom */
};

const unsigned char face_colors[12] = {
    0xE0, 0xE0, 0x03, 0x03, 0xE3, 0xE3, 
    0x1C, 0x1C, 0xFC, 0xFC, 0x93, 0x93
};

typedef struct {
    unsigned char id;
    int z_depth;
} RenderFace;

RenderFace visible_faces[12];

void rotate_all(unsigned char ax, unsigned char ay, unsigned char az) {
    register unsigned char i;
    int x, y, z;
    int tx, ty, tz;
    int sin_x = ISIN(ax), cos_x = ICOS(ax);
    int sin_y = ISIN(ay), cos_y = ICOS(ay);
    int sin_z = ISIN(az), cos_z = ICOS(az);

    for (i = 0; i < 8; ++i) {
        x = cvx[i];
        y = cvy[i];
        z = cvz[i];

        /* X Rot */
        ty = (y * cos_x - z * sin_x) >> 7;
        tz = (y * sin_x + z * cos_x) >> 7;
        y = ty;
        z = tz;

        /* Y Rot */
        tx = (x * cos_y + z * sin_y) >> 7;
        tz = (-x * sin_y + z * cos_y) >> 7;
        x = tx;
        z = tz;

        /* Z Rot */
        tx = (x * cos_z - y * sin_z) >> 7;
        ty = (x * sin_z + y * cos_z) >> 7;

        vx[i] = tx;
        vy[i] = ty;
        vz[i] = z;
    }
}

void project_all(void) {
    register unsigned char i;
    const signed char distance = 80;
    const signed char factor = 60;
    int z_shifted;
    int factor_precalc;

    for (i = 0; i < 8; ++i) {
        z_shifted = vz[i] + distance;
        if (z_shifted < 10) z_shifted = 10;

        factor_precalc = (int)vx[i] * factor;
        px[i] = (signed char)((factor_precalc / z_shifted) + (GPU_WIDTH / 2));

        factor_precalc = (int)vy[i] * factor;
        py[i] = (signed char)((GPU_HEIGHT / 2) - (factor_precalc / z_shifted));
    }
}

void init_graphics(void) {
    INIT_BUFFER();
    lcd_init();
    gpu_fill_screen(0x00);
}

const unsigned char black_color = 0x00;

int main(void) {
    unsigned char ang_x, ang_y, ang_z;
    char speed_x, speed_y, speed_z;
    register unsigned char i;
    unsigned char f, p1, p2, p3;
    unsigned char count;
    int v1x, v1y, v2x, v2y;
    long cross;
    RenderFace temp_face;
    int j_sort;

    /* Bounding box for clearing */
    signed char min_x, max_x, min_y, max_y;
    signed char clear_min_x = 0, clear_max_x = 0;
    signed char clear_min_y = 0, clear_max_y = 0;

    unsigned int seed = 0;

    init_graphics();
    via_init();

    print_str("PRESS ANY KEY TO GENERATE SEED: ");

    while (bios_getchar_nb() == 0) {
        seed++;
    }

    srand(seed);

    gpu_fill_screen(0x00);
    lcd_print("Rotating Cube");

    ang_x = (unsigned char)(rand());
    ang_y = (unsigned char)(rand());
    ang_z = (unsigned char)(rand());

    speed_x = (char)((rand() % 8) + 1);
    if (rand() & 1) speed_x = -speed_x;

    speed_y = (char)((rand() % 8) + 1);
    if (rand() & 1) speed_y = -speed_y;

    speed_z = (char)((rand() % 8) + 1);
    if (rand() & 1) speed_z = -speed_z;

    while (1) {
        /* Clear previous frame using bounding box */
        if (clear_max_x >= clear_min_x && clear_max_y >= clear_min_y) {
            gpu_draw_rect(clear_min_x, clear_min_y,
                          clear_max_x - clear_min_x + 1,
                          clear_max_y - clear_min_y + 1,
                          black_color);
        }

        ang_x += speed_x;
        ang_y += speed_y;
        ang_z += speed_z;

        rotate_all(ang_x, ang_y, ang_z);
        project_all();

        /* Calculate new bounding box for next clear */
        min_x = 127;
        max_x = -128;
        min_y = 127;
        max_y = -128;

        for (i = 0; i < 8; ++i) {
            if (px[i] < min_x) min_x = px[i];
            if (px[i] > max_x) max_x = px[i];
            if (py[i] < min_y) min_y = py[i];
            if (py[i] > max_y) max_y = py[i];
        }

        /* Clamp to screen area */
        if (min_x < 0) min_x = 0;
        if (max_x >= GPU_WIDTH) max_x = GPU_WIDTH - 1;
        if (min_y < 0) min_y = 0;
        if (max_y >= GPU_HEIGHT) max_y = GPU_HEIGHT - 1;

        clear_min_x = min_x;
        clear_max_x = max_x;
        clear_min_y = min_y;
        clear_max_y = max_y;

        count = 0;
        for (i = 0; i < 12; ++i) {
            p1 = faces[i][0];
            p2 = faces[i][1];
            p3 = faces[i][2];

            v1x = px[p2] - px[p1];
            v1y = py[p2] - py[p1];
            v2x = px[p3] - px[p1];
            v2y = py[p3] - py[p1];

            cross = (long)v1x * v2y - (long)v1y * v2x;

            if (cross >= 0) {
                visible_faces[count].id = i;
                visible_faces[count].z_depth = vz[p1] + vz[p2] + vz[p3];
                count++;
            }
        }
        for (i = 1; i < count; ++i) {
            temp_face = visible_faces[i];
            j_sort = i - 1;

            while (j_sort >= 0 && visible_faces[j_sort].z_depth < temp_face.z_depth) {
                visible_faces[j_sort + 1] = visible_faces[j_sort];
                j_sort--;
            }
            visible_faces[j_sort + 1] = temp_face;
        }

        for (i = 0; i < count; ++i) {
            f = visible_faces[i].id;
            p1 = faces[f][0];
            p2 = faces[f][1];
            p3 = faces[f][2];

            gpu_draw_tri(
                px[p1], py[p1],
                px[p2], py[p2],
                px[p3], py[p3],
                face_colors[f],
                1 
            );
        }
        drawFrame();
        via_wait_frame();
    }
    return 0;
}