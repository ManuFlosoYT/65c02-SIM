#include <stdlib.h>

#include "Include/BIOS.h"
#include "Include/GPU.h"
#include "Include/LCD.h"
#include "Include/VIA.h"

#define CLOCK_HZ 1000000UL
#define TARGET_FPS 30
#define VIA_T1_COUNT ((unsigned int)(CLOCK_HZ / TARGET_FPS))

#define FP_SHIFT 8
#define FP_SCALE (1 << FP_SHIFT)

const int SIN_TABLE[256] = {
    0,    6,    12,   18,   25,   31,   37,   43,   49,   56,   62,   68,
    74,   80,   86,   92,   97,   103,  109,  114,  120,  125,  131,  136,
    141,  146,  150,  155,  159,  164,  168,  172,  176,  180,  183,  187,
    190,  193,  196,  198,  201,  203,  206,  208,  210,  212,  213,  215,
    216,  217,  218,  219,  219,  220,  220,  220,  220,  219,  219,  218,
    217,  216,  215,  213,  212,  210,  208,  206,  203,  201,  198,  196,
    193,  190,  187,  183,  180,  176,  172,  168,  164,  159,  155,  150,
    146,  141,  136,  131,  125,  120,  114,  109,  103,  97,   92,   86,
    80,   74,   68,   62,   56,   49,   43,   37,   31,   25,   18,   12,
    6,    0,    -6,   -12,  -18,  -25,  -31,  -37,  -43,  -49,  -56,  -62,
    -68,  -74,  -80,  -86,  -92,  -97,  -103, -109, -114, -120, -125, -131,
    -136, -141, -146, -150, -155, -159, -164, -168, -172, -176, -180, -183,
    -187, -190, -193, -196, -198, -201, -203, -206, -208, -210, -212, -213,
    -215, -216, -217, -218, -219, -219, -220, -220, -220, -220, -219, -219,
    -218, -217, -216, -215, -213, -212, -210, -208, -206, -203, -201, -198,
    -196, -193, -190, -187, -183, -180, -176, -172, -168, -164, -159, -155,
    -150, -146, -141, -136, -131, -125, -120, -114, -109, -103, -97,  -92,
    -86,  -80,  -74,  -68,  -62,  -56,  -49,  -43,  -37,  -31,  -25,  -18,
    -12,  -6};

#define ISIN(x) (SIN_TABLE[(unsigned char)(x)])
#define ICOS(x) (SIN_TABLE[(unsigned char)((x) + 64)])

void via_init(void) {
    via_disable_interrupt(0x7F);
    VIA_ACR = 0x40;
    via_set_timer1(VIA_T1_COUNT);
    asm("cli");
}

int vx[8], vy[8], vz[8];
signed char px[8], py[8];

#define C_SIZE (22 * FP_SCALE)

const int cvx[8] = {-C_SIZE, C_SIZE, C_SIZE, -C_SIZE, -C_SIZE, C_SIZE, C_SIZE, -C_SIZE};
const int cvy[8] = {-C_SIZE, -C_SIZE, C_SIZE, C_SIZE, -C_SIZE, -C_SIZE, C_SIZE, C_SIZE};
const int cvz[8] = {-C_SIZE, -C_SIZE, -C_SIZE, -C_SIZE, C_SIZE, C_SIZE, C_SIZE, C_SIZE};

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
        ty = (int)(((long)y * cos_x - (long)z * sin_x) >> FP_SHIFT);
        tz = (int)(((long)y * sin_x + (long)z * cos_x) >> FP_SHIFT);
        y = ty;
        z = tz;

        /* Y Rot */
        tx = (int)(((long)x * cos_y + (long)z * sin_y) >> FP_SHIFT);
        tz = (int)(((long)-x * sin_y + (long)z * cos_y) >> FP_SHIFT);
        x = tx;
        z = tz;

        /* Z Rot */
        tx = (int)(((long)x * cos_z - (long)y * sin_z) >> FP_SHIFT);
        ty = (int)(((long)x * sin_z + (long)y * cos_z) >> FP_SHIFT);

        vx[i] = tx;
        vy[i] = ty;
        vz[i] = z;
    }
}

void project_all(void) {
    register unsigned char i;
    const long distance = 80;
    const long factor = 60;
    int z_shifted;
    long factor_precalc;

    for (i = 0; i < 8; ++i) {
        z_shifted = (vz[i] >> 8) + distance;
        if (z_shifted < 10) z_shifted = 10;

        factor_precalc = (long)vx[i] * factor;
        px[i] = (signed char)((int)((factor_precalc >> 8) / z_shifted) + (GPU_WIDTH / 2));

        factor_precalc = (long)vy[i] * factor;
        py[i] = (signed char)((GPU_HEIGHT / 2) - (int)((factor_precalc >> 8) / z_shifted));
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

        via_wait_frame();
    }
    return 0;
}