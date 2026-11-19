#include <stdbool.h>
#include <stdint.h>

#include "Libs/BIOS.h"
#include "Libs/GPU.h"
#include "Libs/LCD.h"
#include "Libs/VIA.h"

enum GameTiming {
    CLOCK_HZ        = 1000000UL,
    TARGET_FPS      = 60,
    VIA_T1_COUNT    = (uint16_t)(1000000UL / 60),
};

void via_init(void) {
    via_disable_interrupt(0x7FU);
    VIA_ACR = 0x40U;
    via_set_timer1(VIA_T1_COUNT);
    asm("cli");
}

enum GameColors {
    PLAYER_COLOR = 0xE0U,
    BG_COLOR     = 0x03U,
    GROUND_COLOR = 0x90U,
};

enum GameDimensions {
    GROUND_Y        = (GPU_HEIGHT - 2),
    PLATFORM_Y      = (GPU_HEIGHT - 2 - 15),
    PLATFORM_MID_Y  = (GPU_HEIGHT - 2 - 30),
    PLATFORM_TOP_Y  = (GPU_HEIGHT - 2 - 45),
    PLATFORM_W      = 20,
    PLATFORM_H      = 2,
};

enum GamePhysics {
    GRAVITY    = 1,
    JUMP_FORCE = -7,
    MOVE_SPEED = 2,
};

static int16_t p_x = 50;
static int16_t p_y = 10;
static int16_t p_dx = 0;
static int16_t p_dy = 0;

void init_game(void) {
    INIT_BUFFER();
    lcd_init();
    lcd_print("Super Game 6502");

    gpu_fill_screen(BG_COLOR);

    gpu_draw_rect(0, GROUND_Y, GPU_WIDTH, 2, GROUND_COLOR);

    gpu_draw_rect(5, PLATFORM_Y, PLATFORM_W, PLATFORM_H, GROUND_COLOR);
    gpu_draw_rect(40, PLATFORM_Y, PLATFORM_W, PLATFORM_H, GROUND_COLOR);
    gpu_draw_rect(75, PLATFORM_Y, PLATFORM_W, PLATFORM_H, GROUND_COLOR);

    gpu_draw_rect(22, PLATFORM_MID_Y, PLATFORM_W, PLATFORM_H, GROUND_COLOR);
    gpu_draw_rect(57, PLATFORM_MID_Y, PLATFORM_W, PLATFORM_H, GROUND_COLOR);

    gpu_draw_rect(5, PLATFORM_TOP_Y, PLATFORM_W, PLATFORM_H, GROUND_COLOR);
    gpu_draw_rect(40, PLATFORM_TOP_Y, PLATFORM_W, PLATFORM_H, GROUND_COLOR);
    gpu_draw_rect(75, PLATFORM_TOP_Y, PLATFORM_W, PLATFORM_H, GROUND_COLOR);

    gpu_draw_rect((int8_t)p_x, (int8_t)p_y, 5, 5, PLAYER_COLOR);
}

int main(void) {
    int16_t old_x = 0;
    int16_t old_y = 0;
    int16_t inertia_cnt = 0;
    bool on_ground = false;

    init_game();
    via_init();

    while (true) {
        char key = bios_getchar_nb();

        if (key == 'a' || key == 'A') {
            if (p_dx > -3) {
                p_dx--;
            }
            inertia_cnt = on_ground ? 5 : 50;
        } else if (key == 'd' || key == 'D') {
            if (p_dx < 3) {
                p_dx++;
            }
            inertia_cnt = on_ground ? 5 : 50;
        } else if (key == 'q' || key == 'Q') {
            break;
        }

        if (inertia_cnt > 0) {
            inertia_cnt--;
        }

        p_dy += GRAVITY;

        if (p_dy > 3) {
            p_dy = 3;
        }

        old_x = p_x;
        old_y = p_y;

        p_x += p_dx;
        p_y += p_dy;

        on_ground = false;

        if (p_y >= GROUND_Y - 5) {
            p_y = GROUND_Y - 5;
            p_dy = 0;
            on_ground = true;
        }

        if (p_x < 0) {
            p_x = 0;
        }
        if (p_x > GPU_WIDTH - 5) {
            p_x = GPU_WIDTH - 5;
        }
        if (p_y < 0) {
            p_y = 0;
            p_dy = 0;
        }

        if (p_dy >= 0 && (old_y + 5) <= PLATFORM_Y && (p_y + 5) >= PLATFORM_Y) {
            if (((p_x + 5 > 5) && (p_x < 5 + PLATFORM_W)) ||
                ((p_x + 5 > 40) && (p_x < 40 + PLATFORM_W)) ||
                ((p_x + 5 > 75) && (p_x < 75 + PLATFORM_W))) {
                p_y = PLATFORM_Y - 5;
                p_dy = 0;
                on_ground = true;
            }
        } else if (p_dy < 0 && old_y >= (PLATFORM_Y + PLATFORM_H) &&
                   p_y < (PLATFORM_Y + PLATFORM_H) &&
                   (((p_x + 5 > 5) && (p_x < 5 + PLATFORM_W)) ||
                    ((p_x + 5 > 40) && (p_x < 40 + PLATFORM_W)) ||
                    ((p_x + 5 > 75) && (p_x < 75 + PLATFORM_W)))) {
            p_y = PLATFORM_Y + PLATFORM_H;
            p_dy = 0;
        }

        if (p_dy >= 0 && (old_y + 5) <= PLATFORM_MID_Y &&
            (p_y + 5) >= PLATFORM_MID_Y) {
            if (((p_x + 5 > 22) && (p_x < 22 + PLATFORM_W)) ||
                ((p_x + 5 > 57) && (p_x < 57 + PLATFORM_W))) {
                p_y = PLATFORM_MID_Y - 5;
                p_dy = 0;
                on_ground = true;
            }
        } else if (p_dy < 0 && old_y >= (PLATFORM_MID_Y + PLATFORM_H) &&
                   p_y < (PLATFORM_MID_Y + PLATFORM_H) &&
                   (((p_x + 5 > 22) && (p_x < 22 + PLATFORM_W)) ||
                    ((p_x + 5 > 57) && (p_x < 57 + PLATFORM_W)))) {
            p_y = PLATFORM_MID_Y + PLATFORM_H;
            p_dy = 0;
        }

        if (p_dy >= 0 && (old_y + 5) <= PLATFORM_TOP_Y &&
            (p_y + 5) >= PLATFORM_TOP_Y) {
            if (((p_x + 5 > 5) && (p_x < 5 + PLATFORM_W)) ||
                ((p_x + 5 > 40) && (p_x < 40 + PLATFORM_W)) ||
                ((p_x + 5 > 75) && (p_x < 75 + PLATFORM_W))) {
                p_y = PLATFORM_TOP_Y - 5;
                p_dy = 0;
                on_ground = true;
            }
        } else if (p_dy < 0 && old_y >= (PLATFORM_TOP_Y + PLATFORM_H) &&
                   p_y < (PLATFORM_TOP_Y + PLATFORM_H) &&
                   (((p_x + 5 > 5) && (p_x < 5 + PLATFORM_W)) ||
                    ((p_x + 5 > 40) && (p_x < 40 + PLATFORM_W)) ||
                    ((p_x + 5 > 75) && (p_x < 75 + PLATFORM_W)))) {
            p_y = PLATFORM_TOP_Y + PLATFORM_H;
            p_dy = 0;
        }

        if ((key == 'w' || key == 'W') && on_ground) {
            p_dy = JUMP_FORCE;
            on_ground = false;
        }

        if (inertia_cnt == 0 && on_ground) {
            if (p_dx > 0) {
                p_dx--;
            } else if (p_dx < 0) {
                p_dx++;
            }
        }

        if (p_x != old_x || p_y != old_y) {
            gpu_draw_rect((int8_t)p_x, (int8_t)p_y, 5, 5, PLAYER_COLOR);

            for (int16_t j = old_y; j < old_y + 5; j++) {
                for (int16_t i = old_x; i < old_x + 5; i++) {
                    if (i >= p_x && i < p_x + 5 && j >= p_y && j < p_y + 5) {
                        continue;
                    }
                    gpu_put_pixel((int8_t)i, (int8_t)j, BG_COLOR);
                }
            }
        }

        via_wait_frame();
    }

    return 0;
}
