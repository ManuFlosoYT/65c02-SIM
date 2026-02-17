#include <stdlib.h>

#include "Include/BIOS.h"
#include "Include/GPU.h"
#include "Include/LCD.h"
#include "Include/VIA.h"

#define CLOCK_HZ 1000000UL
#define TARGET_FPS 30
#define VIA_T1_COUNT ((unsigned int)(CLOCK_HZ / TARGET_FPS))

#define FP_SHIFT 8
#define FP_SCALE 256

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

void via_init() {
    via_disable_interrupt(0x7F);
    VIA_ACR = 0x40;
    via_set_timer1(VIA_T1_COUNT);
    asm("cli");
}

/* (Standard 3D):
   0: Back-Bottom-Left   (-x, -y, -z)
   1: Back-Bottom-Right  (+x, -y, -z)
   2: Back-Top-Right     (+x, +y, -z)
   3: Back-Top-Left      (-x, +y, -z)
   4: Front-Bottom-Left  (-x, -y, +z)
   5: Front-Bottom-Right (+x, -y, +z)
   6: Front-Top-Right    (+x, +y, +z)
   7: Front-Top-Left     (-x, +y, +z)
*/
int vx[8], vy[8], vz[8];
int px[8], py[8];

#define C_SIZE (22 * FP_SCALE)

const int cvx[8] = {-C_SIZE, C_SIZE, C_SIZE, -C_SIZE,
                    -C_SIZE, C_SIZE, C_SIZE, -C_SIZE};
const int cvy[8] = {-C_SIZE, -C_SIZE, C_SIZE, C_SIZE,
                    -C_SIZE, -C_SIZE, C_SIZE, C_SIZE};
const int cvz[8] = {-C_SIZE, -C_SIZE, -C_SIZE, -C_SIZE,
                    C_SIZE,  C_SIZE,  C_SIZE,  C_SIZE};

const unsigned char faces[12][3] = {
    /* Front (+Z) */
    {4, 5, 6},
    {4, 6, 7},
    /* Back (-Z) */
    {1, 0, 3},
    {1, 3, 2},
    /* Right (+X) */
    {5, 1, 2},
    {5, 2, 6},
    /* Left (-X) */
    {0, 4, 7},
    {0, 7, 3},
    /* Top (+Y) */
    {7, 6, 2},
    {7, 2, 3},
    /* Bottom (-Y) */
    {0, 1, 5},
    {0, 5, 4}};
const unsigned char face_colors[12] = {
    0xE0, 0xE0,  // Front
    0x03, 0x03,  // Back
    0xE3, 0xE3,  // Right
    0x1C, 0x1C,  // Left
    0xFC, 0xFC,  // Top
    0x93, 0x93   // Bottom
};

void rotate_all(unsigned char ax, unsigned char ay, unsigned char az) {
    unsigned char i;
    int x, y, z;
    int tx, ty, tz;
    int sin_x = ISIN(ax);
    int cos_x = ICOS(ax);
    int sin_y = ISIN(ay);
    int cos_y = ICOS(ay);
    int sin_z = ISIN(az);
    int cos_z = ICOS(az);

    for (i = 0; i < 8; ++i) {
        x = cvx[i];
        y = cvy[i];
        z = cvz[i];

        // X Rot
        ty = (int)(((long)y * cos_x - (long)z * sin_x) >> FP_SHIFT);
        tz = (int)(((long)y * sin_x + (long)z * cos_x) >> FP_SHIFT);
        y = ty;
        z = tz;

        // Y Rot
        tx = (int)(((long)x * cos_y + (long)z * sin_y) >> FP_SHIFT);
        tz = (int)(((long)-x * sin_y + (long)z * cos_y) >> FP_SHIFT);
        x = tx;
        z = tz;

        // Z Rot
        tx = (int)(((long)x * cos_z - (long)y * sin_z) >> FP_SHIFT);
        ty = (int)(((long)x * sin_z + (long)y * cos_z) >> FP_SHIFT);
        x = tx;
        y = ty;

        vx[i] = x;
        vy[i] = y;
        vz[i] = z;
    }
}

void project_all() {
    unsigned char i;
    long distance = 80;
    long factor = 60;

    int z_shifted;
    long z_world_scaled;

    for (i = 0; i < 8; ++i) {
        z_shifted = (vz[i] >> 8) + distance;

        if (z_shifted < 10) z_shifted = 10;

        z_world_scaled = (long)z_shifted * 256;

        px[i] = (int)(( (long)vx[i] * factor ) / z_world_scaled) + (GPU_WIDTH / 2);

        py[i] = (int)(((long)vy[i] * factor) / z_world_scaled);
        py[i] = (GPU_HEIGHT / 2) - py[i];
    }
}

int get_face_z(unsigned char f) {
    return (vz[faces[f][0]] + vz[faces[f][1]] + vz[faces[f][2]]) / 3;
}

void init_graphics() {
    INIT_BUFFER();
    lcd_init();
    lcd_print("Rotating Cube");
    gpu_fill_screen(0x00);
}

int main(void) {
    unsigned char ang_x, ang_y, ang_z;
    char speed_x, speed_y, speed_z;
    unsigned char i, j;
    unsigned char f, p1, p2, p3;
    int t_d;
    unsigned char t_o;
    int v1x, v1y, v2x, v2y;
    long cross;
    unsigned char order[12];
    int depth[12];
    unsigned int seed;

    init_graphics();
    via_init();

    /* Initialize random seed using VIA timer */
    seed = (unsigned int)VIA_T1C_L | ((unsigned int)VIA_T1C_H << 8);
    srand(seed);

    /* Random initial angles */
    ang_x = (unsigned char)(rand() & 0xFF);
    ang_y = (unsigned char)(rand() & 0xFF);
    ang_z = (unsigned char)(rand() & 0xFF);

    /* Random rotation speeds (range: -4 to +4, excluding 0) */
    speed_x = (char)((rand() % 8) + 1);
    if (rand() & 1) speed_x = -speed_x;

    speed_y = (char)((rand() % 8) + 1);
    if (rand() & 1) speed_y = -speed_y;

    speed_z = (char)((rand() % 8) + 1);
    if (rand() & 1) speed_z = -speed_z;

    while (1) {
        gpu_fill_screen(0x00);

        ang_x += speed_x;
        ang_y += speed_y;
        ang_z += speed_z;

        rotate_all(ang_x, ang_y, ang_z);
        project_all();

        for (i = 0; i < 12; ++i) {
            order[i] = i;
            depth[i] = get_face_z(i);
        }

        for (i = 0; i < 11; ++i) {
            for (j = 0; j < 11 - i; ++j) {
                if (depth[j] < depth[j + 1]) {
                    t_d = depth[j];
                    depth[j] = depth[j + 1];
                    depth[j + 1] = t_d;
                    t_o = order[j];
                    order[j] = order[j + 1];
                    order[j + 1] = t_o;
                }
            }
        }

        for (i = 0; i < 12; ++i) {
            f = order[i];
            p1 = faces[f][0];
            p2 = faces[f][1];
            p3 = faces[f][2];

            v1x = px[p2] - px[p1];
            v1y = py[p2] - py[p1];
            v2x = px[p3] - px[p1];
            v2y = py[p3] - py[p1];

            cross = (long)v1x * v2y - (long)v1y * v2x;

            if (cross >= 0) {
                gpu_draw_tri(
                    (int)px[p1], (int)py[p1],
                    (int)px[p2], (int)py[p2],
                    (int)px[p3], (int)py[p3],
                    face_colors[f],
                    1 
                );
            }
        }

        via_wait_frame();
    }
    return 0;
}