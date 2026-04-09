#include "Libs/GPUDoubleBuffer.h"
#include "Libs/LCD.h"

#define NUM_VERTICES 21
#define NUM_FACES 30

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

signed char vx[NUM_VERTICES], vy[NUM_VERTICES], vz[NUM_VERTICES];
signed char px[NUM_VERTICES], py[NUM_VERTICES];

const signed char cvx[NUM_VERTICES] = {
    -100, 100, 100, -100, -100, 100, 100, -100,
    -80, -20, -20, -80, -80, -20, -20, -80,
    20, 80, 80, 20, 50};

const signed char cvy[NUM_VERTICES] = {
    -30, -30, 90, 90, -30, -30, 90, 90,
    -30, -30, 30, 30, -30, -30, 30, 30,
    -30, -30, -30, -30, 50};

const signed char cvz[NUM_VERTICES] = {
    -100, -100, -100, -100, 100, 100, 100, 100,
    -40, -40, -40, -40, 20, 20, 20, 20,
    -40, -40, 20, 20, -10};

const unsigned char faces[NUM_FACES][3] = {
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
    {19, 20, 16}
};

const unsigned char face_colors[NUM_FACES] = {
    0x92, 0x92, 0x54, 0x54, 0x3F, 0x3F, 0x4A, 0x4A, 0x4A, 0x4A, 0x92, 0x92,
    0x03, 0x03, 0x03, 0x03, 0x07, 0x07, 0x23, 0x23, 0x47, 0x47, 0x02, 0x02,
    0x1C, 0x1C, 0x0C, 0x1D, 0x09, 0x18};

typedef struct {
    unsigned char id;
    int z_depth;
} RenderFace;

RenderFace visible_faces[NUM_FACES];

void rotate_all(unsigned char ax, unsigned char ay, unsigned char az) {
    register unsigned char i;
    int x, y, z;
    int tx, ty, tz;
    int sin_x = ISIN(ax), cos_x = ICOS(ax);
    int sin_y = ISIN(ay), cos_y = ICOS(ay);
    int sin_z = ISIN(az), cos_z = ICOS(az);

    for (i = 0; i < NUM_VERTICES; ++i) {
        x = cvx[i];
        y = cvy[i];
        z = cvz[i];

        ty = (y * cos_x - z * sin_x) >> 7;
        tz = (y * sin_x + z * cos_x) >> 7;
        y = ty;
        z = tz;

        tx = (x * cos_y + z * sin_y) >> 7;
        tz = (-x * sin_y + z * cos_y) >> 7;
        x = tx;
        z = tz;

        tx = (x * cos_z - y * sin_z) >> 7;
        ty = (x * sin_z + y * cos_z) >> 7;

        vx[i] = tx;
        vy[i] = ty;
        vz[i] = z;
    }
}

void project_all(void) {
    register unsigned char i;
    const int distance = 150;
    const int factor = 120;
    int z_shifted;
    int factor_precalc;
    int p;

    for (i = 0; i < NUM_VERTICES; ++i) {
        z_shifted = vz[i] + distance;
        if (z_shifted < 5) z_shifted = 5;

        factor_precalc = (int)vx[i] * factor;
        p = (factor_precalc / z_shifted) + (GPU_WIDTH / 2);
        if (p > 125) p = 125;
        if (p < -125) p = -125;
        px[i] = (signed char)p;

        factor_precalc = (int)vy[i] * factor;
        p = (GPU_HEIGHT / 2) - (factor_precalc / z_shifted);
        if (p > 125) p = 125;
        if (p < -125) p = -125;
        py[i] = (signed char)p;
    }
}

void init_graphics(void) {
    lcd_init();
    gpu_fill_screen(0x00);
}

int main(void) {
    unsigned char ang_y = 0;
    register unsigned char i;
    unsigned char f, p1, p2, p3;
    unsigned char count;
    int v1x, v1y, v2x, v2y;
    long cross;
    RenderFace temp_face;
    int j_sort;

    init_graphics();
    lcd_print("Room 3D");

    while (1) {
        ang_y += 1;

        rotate_all(0, ang_y, 0);
        project_all();

        gpu_fill_screen(0x00);

        count = 0;
        for (i = 0; i < NUM_FACES; ++i) {
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

            while (j_sort >= 0 &&
                   visible_faces[j_sort].z_depth < temp_face.z_depth) {
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

            gpu_draw_tri(px[p1], py[p1], px[p2], py[p2], px[p3], py[p3],
                         face_colors[f], 1);
        }

        drawFrame();
    }
    return 0;
}
