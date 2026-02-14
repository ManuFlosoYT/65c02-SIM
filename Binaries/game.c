#include "Include/BIOS.h"
#include "Include/GPU.h"
#include "Include/LCD.h"
#include "Include/VIA.h"

// --- VIA Configuration ---
#define CLOCK_HZ 1000000UL
#define TARGET_FPS 60
#define VIA_T1_COUNT ((unsigned int)(CLOCK_HZ / TARGET_FPS))

void via_init() {
    // 1. Disable all VIA Interrupts (IER) so we don't trigger BIOS IRQ
    //    0x7F = 0111 1111 (Bit 7=0 clears the bits)
    via_disable_interrupt(0x7F);

    // 2. Configure ACR for Continuous Mode on T1 (Bit 6 = 1)
    //    0x40 = 0100 0000
    VIA_ACR = 0x40;

    // 3. Set Timer 1 Latch
    via_set_timer1(VIA_T1_COUNT);

    // 4. Enable CPU Interrupts (CLI) to allow ACIA (Keyboard) to work
    asm("cli");

    // Note: We do NOT enable VIA interrupt in IER.
    // We will poll VIA_IFR & 0x40 in the main loop.
    // The flag gets set regardless of IER.
}

// --- Constants ---
#define PLAYER_COLOR 0xE0  // Red
#define BG_COLOR 0x03      // Dark Blue-ish
#define GROUND_COLOR 0x90  // Brown-ish
#define GROUND_Y (GPU_HEIGHT - 2)
#define PLATFORM_Y (GROUND_Y - 15)  // 3 player heights (5*3=15) above ground
#define PLATFORM_MID_Y (PLATFORM_Y - 15)      // 3 player heights above previous
#define PLATFORM_TOP_Y (PLATFORM_MID_Y - 15)  // 3 player heights above previous
#define PLATFORM_W 20
#define PLATFORM_H 2

#define GRAVITY 1
#define JUMP_FORCE -7
#define MOVE_SPEED 2

// Player State
int p_x = 50;
int p_y = 10;
int p_dx = 0;
int p_dy = 0;

void init_game() {
    INIT_BUFFER();
    lcd_init();
    lcd_print("Super Game 6502");

    gpu_fill_screen(BG_COLOR);

    // Draw Ground
    gpu_draw_rect(0, GROUND_Y, GPU_WIDTH, 2, GROUND_COLOR);

    // Draw Platforms Level 1
    // Left
    gpu_draw_rect(5, PLATFORM_Y, PLATFORM_W, PLATFORM_H, GROUND_COLOR);
    // Center
    gpu_draw_rect(40, PLATFORM_Y, PLATFORM_W, PLATFORM_H, GROUND_COLOR);
    // Right
    gpu_draw_rect(75, PLATFORM_Y, PLATFORM_W, PLATFORM_H, GROUND_COLOR);

    // Draw Platforms Level 2 (Brick Pattern)
    // Between Left and Center
    gpu_draw_rect(22, PLATFORM_MID_Y, PLATFORM_W, PLATFORM_H, GROUND_COLOR);
    // Between Center and Right
    gpu_draw_rect(57, PLATFORM_MID_Y, PLATFORM_W, PLATFORM_H, GROUND_COLOR);

    // Draw Platforms Level 3 (Top - Same as Level 1)
    // Left
    gpu_draw_rect(5, PLATFORM_TOP_Y, PLATFORM_W, PLATFORM_H, GROUND_COLOR);
    // Center
    gpu_draw_rect(40, PLATFORM_TOP_Y, PLATFORM_W, PLATFORM_H, GROUND_COLOR);
    // Right
    gpu_draw_rect(75, PLATFORM_TOP_Y, PLATFORM_W, PLATFORM_H, GROUND_COLOR);

    // Initial Player Draw
    gpu_draw_rect(p_x, p_y, 5, 5, PLAYER_COLOR);
}

int main(void) {
    char key;
    int old_x, old_y;
    int inertia_cnt = 0;
    int on_ground = 0;  // Initialize to avoid UB
    int i, j;

    init_game();
    via_init();

    while (1) {
        // --- Input ---
        key = bios_getchar_nb();

        if (key == 'a' || key == 'A') {
            if (p_dx > -3) p_dx--;
            inertia_cnt = on_ground ? 5 : 50;
        } else if (key == 'd' || key == 'D') {
            if (p_dx < 3) p_dx++;
            inertia_cnt = on_ground ? 5 : 50;
        } else if (key == 'q' || key == 'Q') {
            break;
        }

        // Apply friction if no recent horizontal input
        if (inertia_cnt > 0) {
            inertia_cnt--;
        }

        // --- Physics ---
        // Apply Gravity
        p_dy += GRAVITY;

        // Limit falling speed (terminal velocity)
        if (p_dy > 3) p_dy = 3;

        // Save old position for clearing
        old_x = p_x;
        old_y = p_y;

        // Update position
        p_x += p_dx;
        p_y += p_dy;

        // --- Collisions ---
        on_ground = 0;

        // Ground Collision
        if (p_y >= GROUND_Y - 5) {
            p_y = GROUND_Y - 5;
            p_dy = 0;
            on_ground = 1;
        }

        // Screen Borders
        if (p_x < 0) p_x = 0;
        if (p_x > GPU_WIDTH - 5) p_x = GPU_WIDTH - 5;
        if (p_y < 0) {
            p_y = 0;  // Ceiling
            p_dy = 0;
        }

        // Platform Collisions Level 1
        // 1. Landing on top
        if (p_dy >= 0 && (old_y + 5) <= PLATFORM_Y && (p_y + 5) >= PLATFORM_Y) {
            // Check Horizontal overlap
            if (((p_x + 5 > 5) && (p_x < 5 + PLATFORM_W)) ||
                ((p_x + 5 > 40) && (p_x < 40 + PLATFORM_W)) ||
                ((p_x + 5 > 75) && (p_x < 75 + PLATFORM_W))) {
                p_y = PLATFORM_Y - 5;
                p_dy = 0;
                on_ground = 1;
            }
        }
        // 2. Bumping from below
        else if (p_dy < 0 && old_y >= (PLATFORM_Y + PLATFORM_H) &&
                 p_y < (PLATFORM_Y + PLATFORM_H)) {
            if (((p_x + 5 > 5) && (p_x < 5 + PLATFORM_W)) ||
                ((p_x + 5 > 40) && (p_x < 40 + PLATFORM_W)) ||
                ((p_x + 5 > 75) && (p_x < 75 + PLATFORM_W))) {
                p_y = PLATFORM_Y + PLATFORM_H;
                p_dy = 0;
            }
        }

        // Platform Collisions Level 2
        // 1. Landing on top
        if (p_dy >= 0 && (old_y + 5) <= PLATFORM_MID_Y &&
            (p_y + 5) >= PLATFORM_MID_Y) {
            // Check Horizontal overlap
            // Plat A: 22, Plat B: 57
            if (((p_x + 5 > 22) && (p_x < 22 + PLATFORM_W)) ||
                ((p_x + 5 > 57) && (p_x < 57 + PLATFORM_W))) {
                p_y = PLATFORM_MID_Y - 5;
                p_dy = 0;
                on_ground = 1;
            }
        }
        // 2. Bumping from below
        else if (p_dy < 0 && old_y >= (PLATFORM_MID_Y + PLATFORM_H) &&
                 p_y < (PLATFORM_MID_Y + PLATFORM_H)) {
            if (((p_x + 5 > 22) && (p_x < 22 + PLATFORM_W)) ||
                ((p_x + 5 > 57) && (p_x < 57 + PLATFORM_W))) {
                p_y = PLATFORM_MID_Y + PLATFORM_H;
                p_dy = 0;
            }
        }

        // Platform Collisions Level 3 (Mirror of Level 1)
        // 1. Landing on top
        if (p_dy >= 0 && (old_y + 5) <= PLATFORM_TOP_Y &&
            (p_y + 5) >= PLATFORM_TOP_Y) {
            // Check Horizontal overlap
            if (((p_x + 5 > 5) && (p_x < 5 + PLATFORM_W)) ||
                ((p_x + 5 > 40) && (p_x < 40 + PLATFORM_W)) ||
                ((p_x + 5 > 75) && (p_x < 75 + PLATFORM_W))) {
                p_y = PLATFORM_TOP_Y - 5;
                p_dy = 0;
                on_ground = 1;
            }
        }
        // 2. Bumping from below
        else if (p_dy < 0 && old_y >= (PLATFORM_TOP_Y + PLATFORM_H) &&
                 p_y < (PLATFORM_TOP_Y + PLATFORM_H)) {
            if (((p_x + 5 > 5) && (p_x < 5 + PLATFORM_W)) ||
                ((p_x + 5 > 40) && (p_x < 40 + PLATFORM_W)) ||
                ((p_x + 5 > 75) && (p_x < 75 + PLATFORM_W))) {
                p_y = PLATFORM_TOP_Y + PLATFORM_H;
                p_dy = 0;
            }
        }

        // Delayed Input/Friction Logic that depends on on_ground
        // We handle jumping here because we need to know if we are on ground
        if ((key == 'w' || key == 'W') && on_ground) {
            p_dy = JUMP_FORCE;
            on_ground = 0;  // No longer on ground
        }

        // Apply friction if on ground and no input
        if (inertia_cnt == 0 && on_ground) {
            if (p_dx > 0)
                p_dx--;
            else if (p_dx < 0)
                p_dx++;
        }

        // --- Rendering ---
        if (p_x != old_x || p_y != old_y) {
            // Draw Player First
            gpu_draw_rect(p_x, p_y, 5, 5, PLAYER_COLOR);

            // Clear old pixels ONLY if they don't belong to the new player
            for (j = old_y; j < old_y + 5; j++) {
                for (i = old_x; i < old_x + 5; i++) {
                    // Check intersection
                    if (i >= p_x && i < p_x + 5 && j >= p_y && j < p_y + 5) {
                        continue;  // Overlaps
                    }
                    gpu_put_pixel(i, j, BG_COLOR);
                }
            }
        }

        // Delay for frame rate control
        via_wait_frame();
    }

    return 0;
}
