#include "Libs/BIOS.h"
#include "Libs/GPU.h"
#include "Libs/LCD.h"
#include "Libs/VIA.h"
#include "Libs/SID.h"

/* --- CONFIGURACIÓN --- */
#define CLOCK_HZ        1000000UL
#define TARGET_FPS      60
#define VIA_T1_COUNT    ((unsigned int)(CLOCK_HZ / TARGET_FPS))

/* --- CONSTANTES VISUALES --- */
#define BLOCK_SIZE      3
#define BOARD_W         10
#define BOARD_H         18

#define BOARD_Y_OFFSET  5
#define BOARD_X_OFFSET  35
#define NEXT_X_OFFSET   8
#define NEXT_Y_OFFSET   18
#define STATS_X_OFFSET  72
#define STATS_BOX_W     24
#define STATS_BOX_H     7
#define STATS_Y_RECORD  2
#define STATS_Y_SCORE   22
#define STATS_Y_PIECES  42

/* --- COLORES --- */
#define COL_BG          0x00
#define COL_BORDER      0xFF
#define COL_TEXT        0xFF
#define COL_I           0x1F
#define COL_O           0xFC
#define COL_T           0xE3
#define COL_S           0x1C
#define COL_Z           0xE0
#define COL_J           0x03
#define COL_L           0x90

/* --- DATOS DE PIEZAS --- */
/* Formateado para visualizar mejor las rotaciones */
const signed char SHAPES[7][4][4][2] = {
    {   // I
        {{-1, 0}, { 0, 0}, { 1, 0}, { 2, 0}},
        {{ 1,-1}, { 1, 0}, { 1, 1}, { 1, 2}},
        {{-1, 1}, { 0, 1}, { 1, 1}, { 2, 1}},
        {{ 0,-1}, { 0, 0}, { 0, 1}, { 0, 2}}
    },
    {   // O
        {{ 0, 0}, { 1, 0}, { 0, 1}, { 1, 1}},
        {{ 0, 0}, { 1, 0}, { 0, 1}, { 1, 1}},
        {{ 0, 0}, { 1, 0}, { 0, 1}, { 1, 1}},
        {{ 0, 0}, { 1, 0}, { 0, 1}, { 1, 1}}
    },
    {   // T
        {{-1, 0}, { 0, 0}, { 1, 0}, { 0, 1}},
        {{ 0,-1}, { 0, 0}, { 1, 0}, { 0, 1}},
        {{-1, 0}, { 0, 0}, { 1, 0}, { 0,-1}},
        {{ 0,-1}, { 0, 0}, {-1, 0}, { 0, 1}}
    },
    {   // S
        {{-1, 1}, { 0, 1}, { 0, 0}, { 1, 0}},
        {{ 1,-1}, { 1, 0}, { 0, 0}, { 0, 1}},
        {{-1, 1}, { 0, 1}, { 0, 0}, { 1, 0}},
        {{ 1,-1}, { 1, 0}, { 0, 0}, { 0, 1}}
    },
    {   // Z
        {{-1, 0}, { 0, 0}, { 0, 1}, { 1, 1}},
        {{ 1, 1}, { 1, 0}, { 0, 0}, { 0,-1}},
        {{-1, 0}, { 0, 0}, { 0, 1}, { 1, 1}},
        {{ 1, 1}, { 1, 0}, { 0, 0}, { 0,-1}}
    },
    {   // J
        {{-1, 0}, { 0, 0}, { 1, 0}, { 1, 1}},
        {{ 0,-1}, { 0, 0}, { 0, 1}, {-1, 1}},
        {{-1,-1}, {-1, 0}, { 0, 0}, { 1, 0}},
        {{ 1,-1}, { 0,-1}, { 0, 0}, { 0, 1}}
    },
    {   // L
        {{-1, 0}, { 0, 0}, { 1, 0}, {-1, 1}},
        {{ 0,-1}, { 0, 0}, { 0, 1}, {-1,-1}},
        {{ 1,-1}, {-1, 0}, { 0, 0}, { 1, 0}},
        {{ 1, 1}, { 0, 1}, { 0, 0}, { 0,-1}}
    }
};

const unsigned char PIECE_COLORS[7] = {
    COL_I, COL_O, COL_T, COL_S, COL_Z, COL_J, COL_L
};

/* --- FUENTE (Tiny Font 3x5) --- */
const unsigned char FONT_NUMS[10][5] = {
    {0x7, 0x5, 0x5, 0x5, 0x7}, // 0
    {0x2, 0x6, 0x2, 0x2, 0x7}, // 1
    {0x7, 0x1, 0x7, 0x4, 0x7}, // 2
    {0x7, 0x1, 0x3, 0x1, 0x7}, // 3
    {0x5, 0x5, 0x7, 0x1, 0x1}, // 4
    {0x7, 0x4, 0x7, 0x1, 0x7}, // 5
    {0x7, 0x4, 0x7, 0x5, 0x7}, // 6
    {0x7, 0x1, 0x2, 0x4, 0x4}, // 7
    {0x7, 0x5, 0x7, 0x5, 0x7}, // 8
    {0x7, 0x5, 0x7, 0x1, 0x7}  // 9
};

/* --- VARIABLES GLOBALES --- */
unsigned char board[BOARD_H][BOARD_W];
int next_queue[2];
int px, py, p_type, p_rot, game_over;
unsigned int score = 0, high_score = 0, piece_count = 0;

/* --- MOTOR DE SONIDO (NO BLOQUEANTE) --- */

// Estado de cada canal de voz
unsigned int voice_timer[4];    // Cuántos frames le quedan a la nota (1..3)
unsigned char voice_active[4];  // Si está sonando
unsigned int gameover_freq;     // Para el efecto de caída
unsigned char sfx_gameover_flag = 0;

void sfx_init() {
    sid_set_volume(15);

    // Configuración base de ADSR
    sid_voice_adsr(1, 0, 5, 0, 0);  // Voz 1: Pieza (Corto)
    sid_voice_pw(1, 0x800);

    sid_voice_adsr(2, 0, 9, 5, 5);  // Voz 2: Línea (Melódico)
    sid_voice_adsr(3, 0, 9, 8, 9);  // Voz 3: Game Over (Largo)

    // Limpiar estados
    voice_timer[1] = 0; voice_active[1] = 0;
    voice_timer[2] = 0; voice_active[2] = 0;
    voice_timer[3] = 0; voice_active[3] = 0;
    sfx_gameover_flag = 0;
}

// Llamado una vez por frame (60Hz) para actualizar el sonido
void sfx_update() {
    // Canal 1: Golpe de pieza
    if (voice_active[1]) {
        if (voice_timer[1] > 0) {
            voice_timer[1]--;
        } else {
            // Tiempo agotado: Soltar nota
            sid_voice_control(1, WAVE_PULSE); // Gate OFF
            voice_active[1] = 0;
        }
    }

    // Canal 2: Línea
    if (voice_active[2]) {
        if (voice_timer[2] > 0) {
            voice_timer[2]--;
        } else {
            sid_voice_control(2, WAVE_SAW); // Gate OFF
            voice_active[2] = 0;
        }
    }

    // Canal 3: Game Over (Efecto especial de caída)
    if (sfx_gameover_flag) {
        if (gameover_freq > 0x0200) {
            gameover_freq -= 0x0100; // Bajar frecuencia cada frame
            sid_voice_freq(3, gameover_freq);
        } else {
            // Fin del efecto
            sid_voice_control(3, WAVE_NOISE); // Gate OFF
            sfx_gameover_flag = 0;
        }
    }
}

// Disparador: Colocar pieza (Voz 1)
void sfx_place() {
    sid_voice_control(1, WAVE_PULSE); // Gate OFF rápido
    sid_voice_freq(1, NOTE_C4);
    sid_voice_control(1, WAVE_PULSE | WAVE_GATE); // Gate ON

    voice_active[1] = 1;
    voice_timer[1] = 5; // Duración: 5 frames (~80ms)
}

// Disparador: Línea (Voz 2)
void sfx_line() {
    sid_voice_control(2, WAVE_SAW);
    sid_voice_freq(2, NOTE_E5);
    sid_voice_control(2, WAVE_SAW | WAVE_GATE);

    voice_active[2] = 1;
    voice_timer[2] = 20; // Duración: 20 frames (~330ms)
}

// Disparador: Game Over (Voz 3)
void sfx_gameover() {
    gameover_freq = 0x5000; // Frecuencia inicial alta
    sid_voice_freq(3, gameover_freq);
    sid_voice_control(3, WAVE_NOISE | WAVE_GATE);

    sfx_gameover_flag = 1; // Activar efecto en sfx_update
}

/* --- INICIALIZACIÓN --- */
void via_init() {
    via_disable_interrupt(0x7F);
    VIA_ACR = 0x40;
    via_set_timer1(VIA_T1_COUNT);
    asm("cli");
}

int get_random_piece() {
    return (VIA_T1C_L) % 7;
}

/* --- GRÁFICOS --- */
void draw_pixel_safe(int x, int y, unsigned char color) {
    if (x >= 0 && x < GPU_WIDTH && y >= 0 && y < GPU_HEIGHT) {
        gpu_put_pixel(x, y, color);
    }
}

void draw_block(int x, int y, unsigned char color) {
    int screen_x = BOARD_X_OFFSET + (x * BLOCK_SIZE);
    int screen_y = BOARD_Y_OFFSET + (y * BLOCK_SIZE);
    gpu_draw_rect(screen_x, screen_y, BLOCK_SIZE, BLOCK_SIZE, color);
}

void draw_char_tiny(int x, int y, const unsigned char* bitmap, unsigned char color) {
    int r, c;
    for (r = 0; r < 5; r++) {
        unsigned char row = bitmap[r];
        for (c = 0; c < 3; c++) {
            if (row & (1 << (2 - c))) {
                draw_pixel_safe(x + c, y + r, color);
            }
        }
    }
}

void draw_number(int x, int y, unsigned int num) {
    char buffer[6];
    int i = 0, j, xx = x;
    
    if (num == 0) { 
        draw_char_tiny(xx, y, FONT_NUMS[0], COL_TEXT); 
        return; 
    }
    
    while (num > 0 && i < 5) { 
        buffer[i++] = num % 10; 
        num /= 10; 
    }
    
    for (j = i - 1; j >= 0; j--) {
        draw_char_tiny(xx, y, FONT_NUMS[(int)buffer[j]], COL_TEXT);
        xx += 4;
    }
}

/* --- ETIQUETAS --- */
void draw_label_despues(int x, int y) {
    const unsigned char L_D[] = {6, 5, 5, 5, 6};
    const unsigned char L_E[] = {7, 4, 7, 4, 7};
    const unsigned char L_S[] = {7, 4, 7, 1, 7};
    const unsigned char L_P[] = {6, 5, 6, 4, 4};
    const unsigned char L_U[] = {5, 5, 5, 5, 7};
    int c = x;
    
    draw_char_tiny(c, y, L_D, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_E, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_S, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_P, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_U, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_E, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_S, COL_TEXT);
}

void draw_label_record(int x, int y) {
    const unsigned char L_R[] = {6, 5, 6, 5, 5};
    const unsigned char L_E[] = {7, 4, 7, 4, 7};
    const unsigned char L_C[] = {7, 4, 4, 4, 7};
    const unsigned char L_O[] = {7, 5, 5, 5, 7};
    const unsigned char L_D[] = {6, 5, 5, 5, 6};
    int c = x;
    
    draw_char_tiny(c, y, L_R, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_E, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_C, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_O, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_R, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_D, COL_TEXT);
}

void draw_label_puntos(int x, int y) {
    const unsigned char L_P[] = {6, 5, 6, 4, 4};
    const unsigned char L_U[] = {5, 5, 5, 5, 7};
    const unsigned char L_N[] = {6, 5, 5, 5, 5};
    const unsigned char L_T[] = {7, 2, 2, 2, 2};
    const unsigned char L_O[] = {7, 5, 5, 5, 7};
    const unsigned char L_S[] = {7, 4, 7, 1, 7};
    int c = x;
    
    draw_char_tiny(c, y, L_P, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_U, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_N, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_T, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_O, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_S, COL_TEXT);
}

void draw_label_piezas(int x, int y) {
    const unsigned char L_P[] = {6, 5, 6, 4, 4};
    const unsigned char L_I[] = {7, 2, 2, 2, 7};
    const unsigned char L_E[] = {7, 4, 7, 4, 7};
    const unsigned char L_Z[] = {7, 1, 2, 4, 7};
    const unsigned char L_A[] = {2, 5, 7, 5, 5};
    const unsigned char L_S[] = {7, 4, 7, 1, 7};
    int c = x;
    
    draw_char_tiny(c, y, L_P, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_I, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_E, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_Z, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_A, COL_TEXT); c += 4;
    draw_char_tiny(c, y, L_S, COL_TEXT);
}

/* --- PANELES --- */
void draw_stat_box(int y, unsigned int value) {
    gpu_draw_rect(STATS_X_OFFSET, y, STATS_BOX_W, STATS_BOX_H, COL_BORDER);
    gpu_draw_rect(STATS_X_OFFSET + 1, y + 1, STATS_BOX_W - 2, STATS_BOX_H - 2, COL_BG);
    draw_number(STATS_X_OFFSET + 3, y + 1, value);
}

void update_stats_panel() {
    draw_stat_box(STATS_Y_RECORD + 6, high_score);
    draw_stat_box(STATS_Y_SCORE + 6, score);
    draw_stat_box(STATS_Y_PIECES + 6, piece_count);
}

void add_score(unsigned int points) {
    score += points;
    if (score > high_score) high_score = score;
    update_stats_panel();
}

void draw_next_pieces() {
    int k, j, type, bx, by, base_x, base_y;
    
    // Dibujar marco
    gpu_draw_rect(NEXT_X_OFFSET - 2, NEXT_Y_OFFSET - 2, 22, 1, COL_BORDER);
    gpu_draw_rect(NEXT_X_OFFSET - 2, NEXT_Y_OFFSET + 32, 22, 1, COL_BORDER);
    gpu_draw_rect(NEXT_X_OFFSET - 2, NEXT_Y_OFFSET - 2, 1, 35, COL_BORDER);
    gpu_draw_rect(NEXT_X_OFFSET + 19, NEXT_Y_OFFSET - 2, 1, 35, COL_BORDER);
    gpu_draw_rect(NEXT_X_OFFSET - 1, NEXT_Y_OFFSET - 1, 20, 33, COL_BG);

    for (k = 0; k < 2; k++) {
        type = next_queue[k];
        base_x = NEXT_X_OFFSET + 4;
        base_y = NEXT_Y_OFFSET + 4 + (k * 14);
        
        for (j = 0; j < 4; j++) {
            bx = base_x + ((int)SHAPES[type][0][j][0] * BLOCK_SIZE);
            by = base_y + ((int)SHAPES[type][0][j][1] * BLOCK_SIZE);
            gpu_draw_rect(bx, by, BLOCK_SIZE, BLOCK_SIZE, PIECE_COLORS[type]);
        }
    }
}

/* --- JUEGO --- */
void draw_tetromino_fast(int x, int y, int rot, int type, unsigned char color) {
    int i, bx, by;
    for (i = 0; i < 4; i++) {
        bx = x + (int)SHAPES[type][rot][i][0];
        by = y + (int)SHAPES[type][rot][i][1];
        
        if (by >= 0 && by < BOARD_H && bx >= 0 && bx < BOARD_W) {
            draw_block(bx, by, color);
        }
    }
}

void draw_full_board() {
    int x, y;
    gpu_draw_rect(BOARD_X_OFFSET, BOARD_Y_OFFSET, BOARD_W * BLOCK_SIZE, BOARD_H * BLOCK_SIZE, COL_BG);
    
    for (y = 0; y < BOARD_H; y++) {
        for (x = 0; x < BOARD_W; x++) {
            if (board[y][x] != 0) {
                draw_block(x, y, board[y][x]);
            }
        }
    }
}

int check_collision(int new_x, int new_y, int new_rot) {
    int i, bx, by;
    for (i = 0; i < 4; i++) {
        bx = new_x + (int)SHAPES[p_type][new_rot][i][0];
        by = new_y + (int)SHAPES[p_type][new_rot][i][1];
        
        if (bx < 0 || bx >= BOARD_W || by >= BOARD_H) return 1;
        if (by >= 0 && board[by][bx] != 0) return 1;
    }
    return 0;
}

void lock_piece() {
    int i, bx, by;
    for (i = 0; i < 4; i++) {
        bx = px + (int)SHAPES[p_type][p_rot][i][0];
        by = py + (int)SHAPES[p_type][p_rot][i][1];
        
        if (by >= 0 && by < BOARD_H && bx >= 0 && bx < BOARD_W) {
            board[by][bx] = PIECE_COLORS[p_type];
        }
    }
}

void check_lines() {
    int y, x, k, full, lines_cleared = 0;
    
    for (y = 0; y < BOARD_H; y++) {
        full = 1;
        for (x = 0; x < BOARD_W; x++) {
            if (board[y][x] == 0) {
                full = 0;
                break;
            }
        }
        
        if (full) {
            lines_cleared++;
            // Mover líneas hacia abajo
            for (k = y; k > 0; k--) {
                for (x = 0; x < BOARD_W; x++) {
                    board[k][x] = board[k - 1][x];
                }
            }
            // Limpiar línea superior
            for (x = 0; x < BOARD_W; x++) {
                board[0][x] = 0;
            }
            y--; // Re-checkear esta posición (ahora tiene la línea de arriba)
        }
    }

    if (lines_cleared > 0) {
        add_score(lines_cleared * 50);
        sfx_line();
        draw_full_board();
    }
}

void spawn_piece() {
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
        game_over = 1;
        if (score > high_score) high_score = score;
        update_stats_panel();

        sfx_gameover();

        lcd_clear();
        lcd_print("GAME OVER [R]");
    } else {
        draw_tetromino_fast(px, py, p_rot, p_type, PIECE_COLORS[p_type]);
    }
}

void init_game_state() {
    int x, y;
    for (y = 0; y < BOARD_H; y++) {
        for (x = 0; x < BOARD_W; x++) {
            board[y][x] = 0;
        }
    }

    score = 0;
    piece_count = 0;
    game_over = 0;

    INIT_BUFFER();
    lcd_init();
    lcd_print("Consejo: \nNo mueras xD");

    sfx_init();

    gpu_fill_screen(0x10);

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
    char key;
    int gravity_counter = 0, gravity_max = 30;
    int old_px, old_py, old_rot, try_rot;

    via_init();

    while (1) {
        init_game_state();
        
        while (!game_over) {
            old_px = px;
            old_py = py;
            old_rot = p_rot;
            
            key = bios_getchar_nb();

            if (key == 'a' || key == 'A') {
                if (!check_collision(px - 1, py, p_rot)) px--;
            } else if (key == 'd' || key == 'D') {
                if (!check_collision(px + 1, py, p_rot)) px++;
            } else if (key == 'w' || key == 'W') {
                try_rot = (p_rot + 1) % 4;
                if (!check_collision(px, py, try_rot)) p_rot = try_rot;
            } else if (key == 's' || key == 'S') {
                if (!check_collision(px, py + 1, p_rot)) py++;
            }

            gravity_counter++;
            if (gravity_counter >= gravity_max) {
                gravity_counter = 0;
                if (!check_collision(px, py + 1, p_rot)) {
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
        
        // Bucle de Game Over (esperar reinicio)
        while (1) {
            key = bios_getchar_nb();
            if (key == 'r' || key == 'R') break;

            sfx_update();
            via_wait_frame();
        }
    }
    return 0;
}