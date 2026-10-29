#include <stdbool.h>
#include <stdint.h>

#include "Libs/app-bios.h"
#include "Libs/GPU.h"
#include "Libs/LCD.h"
enum { NOTE_E5 = 0x2B33 };

static inline void sid_voice_freq(uint8_t voice, uint16_t freq) {
    sid_write((uint8_t)((voice - 1U) * 7U + FREQ_LO_1), (uint8_t)(freq & 0xFFU));
    sid_write((uint8_t)((voice - 1U) * 7U + FREQ_HI_1), (uint8_t)(freq >> 8U));
}

static inline void sid_voice_control(uint8_t voice, uint8_t ctrl) {
    sid_write((uint8_t)((voice - 1U) * 7U + CTRL_1), ctrl);
}

#define sid_voice_adsr sid_set_envelope
#define sid_voice_pw sid_set_pw
#include "Libs/VIA.h"

enum TetrisTiming {
    CLOCK_HZ        = 1000000UL,
    TARGET_FPS      = 60,
    VIA_T1_COUNT    = (uint16_t)(1000000UL / 60),
};

enum TetrisBoard {
    BLOCK_SIZE      = 3,
    BOARD_W         = 10,
    BOARD_H         = 18,
    BOARD_Y_OFFSET  = 5,
    BOARD_X_OFFSET  = 35,
    NEXT_X_OFFSET   = 8,
    NEXT_Y_OFFSET   = 18,
    STATS_X_OFFSET  = 72,
    STATS_BOX_W     = 24,
    STATS_BOX_H     = 7,
    STATS_Y_RECORD  = 2,
    STATS_Y_SCORE   = 22,
    STATS_Y_PIECES  = 42,
};

enum TetrisColors {
    COL_BG          = 0x00U,
    COL_BORDER      = 0xFFU,
    COL_TEXT        = 0xFFU,
    COL_I           = 0x1FU,
    COL_O           = 0xFCU,
    COL_T           = 0xE3U,
    COL_S           = 0x1CU,
    COL_Z           = 0xE0U,
    COL_J           = 0x03U,
    COL_L           = 0x90U,
};

static const int8_t SHAPES[7][4][4][2] = {
    {
        {{-1, 0}, { 0, 0}, { 1, 0}, { 2, 0}},
        {{ 1,-1}, { 1, 0}, { 1, 1}, { 1, 2}},
        {{-1, 1}, { 0, 1}, { 1, 1}, { 2, 1}},
        {{ 0,-1}, { 0, 0}, { 0, 1}, { 0, 2}},
    },
    {
        {{ 0, 0}, { 1, 0}, { 0, 1}, { 1, 1}},
        {{ 0, 0}, { 1, 0}, { 0, 1}, { 1, 1}},
        {{ 0, 0}, { 1, 0}, { 0, 1}, { 1, 1}},
        {{ 0, 0}, { 1, 0}, { 0, 1}, { 1, 1}},
    },
    {
        {{-1, 0}, { 0, 0}, { 1, 0}, { 0, 1}},
        {{ 0,-1}, { 0, 0}, { 1, 0}, { 0, 1}},
        {{-1, 0}, { 0, 0}, { 1, 0}, { 0,-1}},
        {{ 0,-1}, { 0, 0}, {-1, 0}, { 0, 1}},
    },
    {
        {{-1, 1}, { 0, 1}, { 0, 0}, { 1, 0}},
        {{ 1,-1}, { 1, 0}, { 0, 0}, { 0, 1}},
        {{-1, 1}, { 0, 1}, { 0, 0}, { 1, 0}},
        {{ 1,-1}, { 1, 0}, { 0, 0}, { 0, 1}},
    },
    {
        {{-1, 0}, { 0, 0}, { 0, 1}, { 1, 1}},
        {{ 1, 1}, { 1, 0}, { 0, 0}, { 0,-1}},
        {{-1, 0}, { 0, 0}, { 0, 1}, { 1, 1}},
        {{ 1, 1}, { 1, 0}, { 0, 0}, { 0,-1}},
    },
    {
        {{-1, 0}, { 0, 0}, { 1, 0}, { 1, 1}},
        {{ 0,-1}, { 0, 0}, { 0, 1}, {-1, 1}},
        {{-1,-1}, {-1, 0}, { 0, 0}, { 1, 0}},
        {{ 1,-1}, { 0,-1}, { 0, 0}, { 0, 1}},
    },
    {
        {{-1, 0}, { 0, 0}, { 1, 0}, {-1, 1}},
        {{ 0,-1}, { 0, 0}, { 0, 1}, {-1,-1}},
        {{ 1,-1}, {-1, 0}, { 0, 0}, { 1, 0}},
        {{ 1, 1}, { 0, 1}, { 0, 0}, { 0,-1}},
    },
};

static const uint8_t PIECE_COLORS[7] = {
    COL_I, COL_O, COL_T, COL_S, COL_Z, COL_J, COL_L,
};

static const uint8_t FONT_NUMS[10][5] = {
    {0x7U, 0x5U, 0x5U, 0x5U, 0x7U},
    {0x2U, 0x6U, 0x2U, 0x2U, 0x7U},
    {0x7U, 0x1U, 0x7U, 0x4U, 0x7U},
    {0x7U, 0x1U, 0x3U, 0x1U, 0x7U},
    {0x5U, 0x5U, 0x7U, 0x1U, 0x1U},
    {0x7U, 0x4U, 0x7U, 0x1U, 0x7U},
    {0x7U, 0x4U, 0x7U, 0x5U, 0x7U},
    {0x7U, 0x1U, 0x2U, 0x4U, 0x4U},
    {0x7U, 0x5U, 0x7U, 0x5U, 0x7U},
    {0x7U, 0x5U, 0x7U, 0x1U, 0x7U},
};

static uint8_t board[BOARD_H][BOARD_W];
static uint8_t next_queue[2];
static int8_t px;
static int8_t py;
static uint8_t p_type;
static uint8_t p_rot;
static bool game_over;
static uint16_t score = 0;
static uint16_t high_score = 0;
static uint16_t piece_count = 0;

static uint16_t voice_timer[4];
static bool voice_active[4];
static uint16_t gameover_freq;
static bool sfx_gameover_flag = false;

void sfx_init(void) {
    sid_set_volume(15);

    sid_voice_adsr(1, 0, 5, 0, 0);
    sid_voice_pw(1, 0x800U);

    sid_voice_adsr(2, 0, 9, 5, 5);
    sid_voice_adsr(3, 0, 9, 8, 9);

    voice_timer[1] = 0;
    voice_active[1] = false;
    voice_timer[2] = 0;
    voice_active[2] = false;
    voice_timer[3] = 0;
    voice_active[3] = false;
    sfx_gameover_flag = false;
}

void sfx_update(void) {
    if (voice_active[1]) {
        if (voice_timer[1] > 0) {
            voice_timer[1]--;
        } else {
            sid_voice_control(1, WAVE_PULSE);
            voice_active[1] = false;
        }
    }

    if (voice_active[2]) {
        if (voice_timer[2] > 0) {
            voice_timer[2]--;
        } else {
            sid_voice_control(2, WAVE_SAW);
            voice_active[2] = false;
        }
    }

    if (sfx_gameover_flag) {
        if (gameover_freq > 0x0200U) {
            gameover_freq -= 0x0100U;
            sid_voice_freq(3, gameover_freq);
        } else {
            sid_voice_control(3, WAVE_NOISE);
            sfx_gameover_flag = false;
        }
    }
}

void sfx_place(void) {
    sid_voice_control(1, WAVE_PULSE);
    sid_voice_freq(1, NOTE_C4);
    sid_voice_control(1, (uint8_t)((unsigned int)WAVE_PULSE | (unsigned int)WAVE_GATE));

    voice_active[1] = true;
    voice_timer[1] = 5;
}

void sfx_line(void) {
    sid_voice_control(2, WAVE_SAW);
    sid_voice_freq(2, NOTE_E5);
    sid_voice_control(2, (uint8_t)((unsigned int)WAVE_SAW | (unsigned int)WAVE_GATE));

    voice_active[2] = true;
    voice_timer[2] = 20;
}

void sfx_gameover(void) {
    gameover_freq = 0x5000U;
    sid_voice_freq(3, gameover_freq);
    sid_voice_control(3, (uint8_t)((unsigned int)WAVE_NOISE | (unsigned int)WAVE_GATE));

    sfx_gameover_flag = true;
}

void via_init(void) {
    via_disable_interrupt(0x7FU);
    VIA_ACR = 0x40U;
    via_set_timer1(VIA_T1_COUNT);
    __asm__("cli");
}

uint8_t get_random_piece(void) {
    return (uint8_t)(VIA_T1C_L % 7U);
}

void draw_pixel_safe(int16_t x, int16_t y, uint8_t color) {
    if (x >= 0 && x < GPU_WIDTH && y >= 0 && y < GPU_HEIGHT) {
        gpu_put_pixel((int8_t)x, (int8_t)y, color);
    }
}

void draw_block(int16_t x, int16_t y, uint8_t color) {
    int16_t screen_x = BOARD_X_OFFSET + (x * BLOCK_SIZE);
    int16_t screen_y = BOARD_Y_OFFSET + (y * BLOCK_SIZE);
    gpu_draw_rect((int8_t)screen_x, (int8_t)screen_y, BLOCK_SIZE, BLOCK_SIZE, color);
}

void draw_char_tiny(int16_t x, int16_t y, const uint8_t* bitmap, uint8_t color) {
    for (int16_t r = 0; r < 5; r++) {
        uint8_t row = bitmap[r];
        for (int16_t c = 0; c < 3; c++) {
            if ((row & (uint8_t)(1U << (2U - c))) != 0U) {
                draw_pixel_safe(x + c, y + r, color);
            }
        }
    }
}

void draw_number(int16_t x, int16_t y, uint16_t num) {
    char buffer[6];
    uint8_t i = 0;
    int16_t xx = x;

    if (num == 0U) {
        draw_char_tiny(xx, y, FONT_NUMS[0], COL_TEXT);
        return;
    }

    while (num > 0U && i < 5U) {
        buffer[i++] = (char)(num % 10U);
        num /= 10U;
    }

    for (int16_t j = (int16_t)(i - 1U); j >= 0; j--) {
        draw_char_tiny(xx, y, FONT_NUMS[(uint8_t)buffer[j]], COL_TEXT);
        xx += 4;
    }
}

void draw_label_despues(int16_t x, int16_t y) {
    const uint8_t L_D[] = {6, 5, 5, 5, 6};
    const uint8_t L_E[] = {7, 4, 7, 4, 7};
    const uint8_t L_S[] = {7, 4, 7, 1, 7};
    const uint8_t L_P[] = {6, 5, 6, 4, 4};
    const uint8_t L_U[] = {5, 5, 5, 5, 7};
    int16_t c = x;

    draw_char_tiny(c, y, L_D, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_E, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_S, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_P, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_U, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_E, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_S, COL_TEXT);
}

void draw_label_record(int16_t x, int16_t y) {
    const uint8_t L_R[] = {6, 5, 6, 5, 5};
    const uint8_t L_E[] = {7, 4, 7, 4, 7};
    const uint8_t L_C[] = {7, 4, 4, 4, 7};
    const uint8_t L_O[] = {7, 5, 5, 5, 7};
    const uint8_t L_D[] = {6, 5, 5, 5, 6};
    int16_t c = x;

    draw_char_tiny(c, y, L_R, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_E, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_C, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_O, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_R, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_D, COL_TEXT);
}

void draw_label_puntos(int16_t x, int16_t y) {
    const uint8_t L_P[] = {6, 5, 6, 4, 4};
    const uint8_t L_U[] = {5, 5, 5, 5, 7};
    const uint8_t L_N[] = {6, 5, 5, 5, 5};
    const uint8_t L_T[] = {7, 2, 2, 2, 2};
    const uint8_t L_O[] = {7, 5, 5, 5, 7};
    const uint8_t L_S[] = {7, 4, 7, 1, 7};
    int16_t c = x;

    draw_char_tiny(c, y, L_P, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_U, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_N, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_T, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_O, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_S, COL_TEXT);
}

void draw_label_piezas(int16_t x, int16_t y) {
    const uint8_t L_P[] = {6, 5, 6, 4, 4};
    const uint8_t L_I[] = {7, 2, 2, 2, 7};
    const uint8_t L_E[] = {7, 4, 7, 4, 7};
    const uint8_t L_Z[] = {7, 1, 2, 4, 7};
    const uint8_t L_A[] = {2, 5, 7, 5, 5};
    const uint8_t L_S[] = {7, 4, 7, 1, 7};
    int16_t c = x;

    draw_char_tiny(c, y, L_P, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_I, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_E, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_Z, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_A, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_S, COL_TEXT);
}

void draw_stat_box(int16_t y, uint16_t value) {
    gpu_draw_rect(STATS_X_OFFSET, (int8_t)y, STATS_BOX_W, STATS_BOX_H, COL_BORDER);
    gpu_draw_rect(STATS_X_OFFSET + 1, (int8_t)(y + 1), STATS_BOX_W - 2, STATS_BOX_H - 2, COL_BG);
    draw_number(STATS_X_OFFSET + 3, (int8_t)(y + 1), value);
}

void update_stats_panel(void) {
    draw_stat_box(STATS_Y_RECORD + 6, high_score);
    draw_stat_box(STATS_Y_SCORE + 6, score);
    draw_stat_box(STATS_Y_PIECES + 6, piece_count);
}

void add_score(uint16_t points) {
    score += points;
    if (score > high_score) {
        high_score = score;
    }
    update_stats_panel();
}

void draw_next_pieces(void) {
    gpu_draw_rect(NEXT_X_OFFSET - 2, NEXT_Y_OFFSET - 2, 22, 1, COL_BORDER);
    gpu_draw_rect(NEXT_X_OFFSET - 2, NEXT_Y_OFFSET + 32, 22, 1, COL_BORDER);
    gpu_draw_rect(NEXT_X_OFFSET - 2, NEXT_Y_OFFSET - 2, 1, 35, COL_BORDER);
    gpu_draw_rect(NEXT_X_OFFSET + 19, NEXT_Y_OFFSET - 2, 1, 35, COL_BORDER);
    gpu_draw_rect(NEXT_X_OFFSET - 1, NEXT_Y_OFFSET - 1, 20, 33, COL_BG);

    for (uint8_t k = 0; k < 2; k++) {
        uint8_t type = next_queue[k];
        int16_t base_x = NEXT_X_OFFSET + 4;
        int16_t base_y = NEXT_Y_OFFSET + 4 + (k * 14);

        for (uint8_t j = 0; j < 4; j++) {
            int16_t bx = base_x + ((int16_t)SHAPES[type][0][j][0] * BLOCK_SIZE);
            int16_t by = base_y + ((int16_t)SHAPES[type][0][j][1] * BLOCK_SIZE);
            gpu_draw_rect((int8_t)bx, (int8_t)by, BLOCK_SIZE, BLOCK_SIZE, PIECE_COLORS[type]);
        }
    }
}

void draw_tetromino_fast(int16_t x, int16_t y, uint8_t rot, uint8_t type, uint8_t color) {
    for (uint8_t i = 0; i < 4; i++) {
        int16_t bx = x + (int16_t)SHAPES[type][rot][i][0];
        int16_t by = y + (int16_t)SHAPES[type][rot][i][1];

        if (by >= 0 && by < BOARD_H && bx >= 0 && bx < BOARD_W) {
            draw_block(bx, by, color);
        }
    }
}

void draw_full_board(void) {
    gpu_draw_rect(BOARD_X_OFFSET, BOARD_Y_OFFSET, BOARD_W * BLOCK_SIZE, BOARD_H * BLOCK_SIZE, COL_BG);

    for (int16_t y = 0; y < BOARD_H; y++) {
        for (int16_t x = 0; x < BOARD_W; x++) {
            if (board[y][x] != 0U) {
                draw_block(x, y, board[y][x]);
            }
        }
    }
}

bool check_collision(int16_t new_x, int16_t new_y, uint8_t new_rot) {
    for (uint8_t i = 0; i < 4; i++) {
        int16_t bx = new_x + (int16_t)SHAPES[p_type][new_rot][i][0];
        int16_t by = new_y + (int16_t)SHAPES[p_type][new_rot][i][1];

        if (bx < 0 || bx >= BOARD_W || by >= BOARD_H) {
            return true;
        }
        if (by >= 0 && board[by][bx] != 0U) {
            return true;
        }
    }
    return false;
}

void lock_piece(void) {
    for (uint8_t i = 0; i < 4; i++) {
        int16_t bx = px + (int16_t)SHAPES[p_type][p_rot][i][0];
        int16_t by = py + (int16_t)SHAPES[p_type][p_rot][i][1];

        if (by >= 0 && by < BOARD_H && bx >= 0 && bx < BOARD_W) {
            board[by][bx] = PIECE_COLORS[p_type];
        }
    }
}

void check_lines(void) {
    uint8_t lines_cleared = 0;

    for (int16_t y = 0; y < BOARD_H; y++) {
        bool full = true;
        for (int16_t x = 0; x < BOARD_W; x++) {
            if (board[y][x] == 0U) {
                full = false;
                break;
            }
        }

        if (full) {
            lines_cleared++;
            for (int16_t k = y; k > 0; k--) {
                for (int16_t x = 0; x < BOARD_W; x++) {
                    board[k][x] = board[k - 1][x];
                }
            }
            for (int16_t x = 0; x < BOARD_W; x++) {
                board[0][x] = 0U;
            }
            y--;
        }
    }

    if (lines_cleared > 0) {
        add_score((uint16_t)(lines_cleared * 50U));
        sfx_line();
        draw_full_board();
    }
}

void spawn_piece(void) {
    p_type = next_queue[0];
    next_queue[0] = next_queue[1];
    next_queue[1] = get_random_piece();

    piece_count++;
    update_stats_panel();
    draw_next_pieces();

    p_rot = 0;
    px = BOARD_W / 2;
    py = 0;

    if (check_collision(px, py, p_rot)) {
        game_over = true;
        if (score > high_score) {
            high_score = score;
        }
        update_stats_panel();

        sfx_gameover();

        lcd_clear();
        lcd_print("GAME OVER [R]");
    } else {
        draw_tetromino_fast(px, py, p_rot, p_type, PIECE_COLORS[p_type]);
    }
}

void init_game_state(void) {
    for (int16_t y = 0; y < BOARD_H; y++) {
        for (int16_t x = 0; x < BOARD_W; x++) {
            board[y][x] = 0U;
        }
    }

    score = 0;
    piece_count = 0;
    game_over = false;

    lcd_init();
    lcd_print("Consejo: \nNo mueras xD");

    sfx_init();

    gpu_fill_screen(0x10U);

    gpu_draw_rect(BOARD_X_OFFSET - 1, BOARD_Y_OFFSET - 1, (BOARD_W * BLOCK_SIZE) + 2, (BOARD_H * BLOCK_SIZE) + 2, COL_BORDER);

    draw_label_despues(NEXT_X_OFFSET - 4, NEXT_Y_OFFSET - 8);
    draw_label_record(STATS_X_OFFSET, STATS_Y_RECORD);
    draw_label_puntos(STATS_X_OFFSET, STATS_Y_SCORE);
    draw_label_piezas(STATS_X_OFFSET, STATS_Y_PIECES);

    next_queue[0] = get_random_piece();
    next_queue[1] = get_random_piece();

    update_stats_panel();
    draw_full_board();
    spawn_piece();
}

int main(void) {
    int16_t gravity_counter = 0;
    const int16_t gravity_max = 30;

    via_init();
    println("Ctrl + C to exit");

    while (true) {
        init_game_state();

        while (!game_over) {
            int8_t old_px = px;
            int8_t old_py = py;
            uint8_t old_rot = p_rot;

            char key = bios_getchar_nb();
            if (key == 3) {
                return 0;
            }

            if (key == 'a' || key == 'A') {
                if (!check_collision((int16_t)(px - 1), py, p_rot)) {
                    px--;
                }
            } else if (key == 'd' || key == 'D') {
                if (!check_collision((int16_t)(px + 1), py, p_rot)) {
                    px++;
                }
            } else if (key == 'w' || key == 'W') {
                uint8_t try_rot = (uint8_t)((p_rot + 1U) % 4U);
                if (!check_collision(px, py, try_rot)) {
                    p_rot = try_rot;
                }
            } else if ((key == 's' || key == 'S') &&
                       !check_collision(px, (int16_t)(py + 1), p_rot)) {
                py++;
            }

            gravity_counter++;
            if (gravity_counter >= gravity_max) {
                gravity_counter = 0;
                if (!check_collision(px, (int16_t)(py + 1), p_rot)) {
                    py++;
                } else {
                    lock_piece();
                    add_score(5);
                    sfx_place();

                    check_lines();
                    spawn_piece();

                    if (!game_over) {
                        old_px = px;
                        old_py = py;
                        old_rot = p_rot;
                    }
                }
            }

            if (!game_over && (px != old_px || py != old_py || p_rot != old_rot)) {
                draw_tetromino_fast(old_px, old_py, old_rot, p_type, COL_BG);
                draw_tetromino_fast(px, py, p_rot, p_type, PIECE_COLORS[p_type]);
            }

            sfx_update();

            via_wait_frame();
        }

        while (true) {
            char key = bios_getchar_nb();
            if (key == 3) {
                return 0;
            }
            if (key == 'r' || key == 'R') {
                break;
            }

            sfx_update();
            via_wait_frame();
        }
    }
    return 0;
}