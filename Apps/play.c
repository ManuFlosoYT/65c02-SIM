/*
 * play.c — microDOS SID Player (v2 compressed streaming playback)
 * Compile with: ./compile-bin.sh play --microDOS
 */
#include <stdint.h>
#include <string.h>

#include "Libs/app-bios.h"

#define CHUNK_SIZE 512
#define LZ_WINDOW_SIZE 2048
#define LZ_WINDOW_MASK 0x07FF

#define SID_BASE 0x4800
#define SID_WRITE_REG(r, v) (*(volatile uint8_t*)(SID_BASE + (r)) = (v))

extern char _CORE_SIZE__[];

static SD_FILE file;
static uint8_t* buffer_pool;
static uint8_t num_bufs;
#define BUF_PTR(idx) (buffer_pool + ((uint16_t)(idx) << 9))

static uint8_t head = 0;
static uint8_t tail = 0;
static uint8_t count = 0;
static uint16_t b_idx = 0;
static uint8_t eof_reached = 0;

static uint8_t next_byte(void) {
    int16_t r;
    if (b_idx >= CHUNK_SIZE) {
        tail = (tail + 1) % num_bufs;
        count--;
        b_idx = 0;
    }
    if (count == 0) {
        if (eof_reached) return 0xFF;
        r = sd_read(&file, BUF_PTR(head), CHUNK_SIZE);
        if (r <= 0) {
            eof_reached = 1;
            return 0xFF;
        }
        head = (head + 1) % num_bufs;
        count++;
    }
    return BUF_PTR(tail)[b_idx++];
}

static uint8_t bit_buf = 0;
static uint8_t bit_mask = 0;

static uint8_t read_bit(void) {
    uint8_t b;
    if (bit_mask == 0) {
        bit_buf = next_byte();
        bit_mask = 0x80;
    }
    b = (bit_buf & bit_mask) ? 1 : 0;
    bit_mask >>= 1;
    return b;
}

static uint16_t read_gamma(void) {
    uint8_t zeros = 0;
    uint16_t val = 1;
    while (read_bit() == 0) {
        zeros++;
        if (zeros > 16) return 1;
    }
    while (zeros--) {
        val = (val << 1) | read_bit();
    }
    return val;
}

static uint8_t lz_window[LZ_WINDOW_SIZE];
static uint16_t lz_head = 0;
static uint16_t lz_last_offset = 0;
static uint16_t lz_lit_rem = 0;
static uint16_t lz_match_rem = 0;
static uint16_t lz_match_offset = 0;

static uint8_t next_semantic_byte(void) {
    uint8_t b;
    uint8_t tag;
    uint8_t sub;
    uint16_t src;
    uint16_t off_hi;
    uint8_t off_lo;

    if (lz_lit_rem > 0) {
        b = next_byte();
        lz_lit_rem--;
        lz_window[lz_head] = b;
        lz_head = (lz_head + 1) & LZ_WINDOW_MASK;
        return b;
    }
    if (lz_match_rem > 0) {
        src = (lz_head - lz_match_offset) & LZ_WINDOW_MASK;
        b = lz_window[src];
        lz_match_rem--;
        lz_window[lz_head] = b;
        lz_head = (lz_head + 1) & LZ_WINDOW_MASK;
        return b;
    }

    tag = read_bit();
    if (tag == 0) {
        lz_lit_rem = read_gamma();
        b = next_byte();
        lz_lit_rem--;
        lz_window[lz_head] = b;
        lz_head = (lz_head + 1) & LZ_WINDOW_MASK;
        return b;
    } else {
        sub = read_bit();
        if (sub == 0) {
            off_hi = read_gamma() - 1;
            off_lo = next_byte();
            lz_match_offset = ((off_hi << 8) | off_lo) + 1;
            lz_match_rem = read_gamma() + 1;
            lz_last_offset = lz_match_offset;
        } else {
            lz_match_offset = lz_last_offset;
            lz_match_rem = read_gamma() + 1;
        }
        src = (lz_head - lz_match_offset) & LZ_WINDOW_MASK;
        b = lz_window[src];
        lz_match_rem--;
        lz_window[lz_head] = b;
        lz_head = (lz_head + 1) & LZ_WINDOW_MASK;
        return b;
    }
}

static uint8_t pw_step = 128;
static uint8_t ntbl_first = 0;
static uint8_t ntbl_count = 0;
static uint16_t ntbl_freq[128];
static uint8_t dict_count = 0;
static uint16_t dict_delays[128];
static uint16_t sched_len = 0;
static uint8_t sched_table[512];

static uint8_t shadow_regs[25];
static uint16_t shadow_pw[3] = {0x0800, 0x0800, 0x0800};
static uint8_t last_ctrl[3] = {0, 0, 0};

static uint16_t last_timer;
static void timer_init(void) {
    VIA_ACR &= ~0x40;
    VIA_T1L_L = 0xFF;
    VIA_T1L_H = 0xFF;
    VIA_T1C_L = 0xFF;
    VIA_T1C_H = 0xFF;
    last_timer = (uint16_t)VIA_T1C_L | ((uint16_t)VIA_T1C_H << 8);
}

static uint16_t timer_delta(void) {
    uint8_t lo = VIA_T1C_L;
    uint8_t hi = VIA_T1C_H;
    uint16_t now = (uint16_t)lo | ((uint16_t)hi << 8);
    uint16_t delta = last_timer - now;
    last_timer = now;
    return delta;
}

static uint16_t get_note_freq(uint8_t note) {
    uint8_t idx = note - ntbl_first;
    if (idx < ntbl_count) {
        return ntbl_freq[idx];
    }
    return 0;
}

int main(void) {
    uint8_t cmd, i, v, b, b2, b3;
    char path[32];
    char** args = _args_ptr;
    uint8_t pathp, argp;
    uint16_t loops;
    uint16_t sched_idx = 0;
    uint16_t pause_counter = 0;
    uint16_t next_read_pause = 0;
    uint8_t next_read_blocks = 0;
    int32_t delay_cycles_left = 0;
    uint16_t core_size;
    uint8_t initial_fill;
    uint8_t min_bufs;
    uint8_t flo, fhi;
    uint8_t dlo, dhi;
    uint8_t slo, shi;
    uint8_t note, ctrl;
    uint16_t freq, pw;
    uint8_t base;
    uint8_t e;
    int16_t r;

    core_size = (uint16_t)_CORE_SIZE__;

    println("microDOS SID Player v2");

    buffer_pool = (uint8_t*)(APP_SAFE_RAM_START + core_size);
    num_bufs = (APP_SAFE_RAM_END - (uint16_t)buffer_pool + 1) / CHUNK_SIZE;

    if (num_bufs == 0) {
        println("Error: Not enough RAM for buffers");
        return 1;
    }

    if (arg_count < 2) {
        println("Usage: play <file.sid>");
        return 1;
    }

    if (sd_open(&file, args[1], SD_READ) == 0) {
        strcpy(path, "/sid/");
        pathp = 5;
        argp = 0;

        while (args[1][argp] && pathp < 28) {
            path[pathp++] = args[1][argp++];
        }
        path[pathp] = '\0';

        if (pathp > 4 &&
            !(path[pathp - 4] == '.' && path[pathp - 3] == 's' && path[pathp - 2] == 'i' && path[pathp - 1] == 'd')) {
            path[pathp++] = '.';
            path[pathp++] = 's';
            path[pathp++] = 'i';
            path[pathp++] = 'd';
            path[pathp] = '\0';
        }

        if (sd_open(&file, path, SD_READ) == 0) {
            print_str("Error: File not found -> ");
            println(args[1]);
            return 1;
        }
    }

    print_str("Playing -> ");
    println(args[1]);

    head = 0;
    tail = 0;
    count = 0;
    b_idx = 0;
    eof_reached = 0;

    initial_fill = num_bufs;
    for (i = 0; i < initial_fill; i++) {
        if (sd_read(&file, BUF_PTR(head), CHUNK_SIZE) <= 0) break;
        head = (head + 1) % num_bufs;
        count++;
    }

    if (next_byte() != 'S' || next_byte() != 'D' || next_byte() != 'Z' || next_byte() != 0x02) {
        println("Error: Invalid or outdated SID format");
        sd_close(&file);
        return 1;
    }

    next_byte();
    min_bufs = next_byte();
    if (num_bufs < min_bufs) {
        print_str("Warning: Low RAM (");
        print_num(num_bufs);
        print_str("/");
        print_num(min_bufs);
        println(" bufs)");
    }

    pw_step = next_byte();
    ntbl_first = next_byte();
    ntbl_count = next_byte();
    for (i = 0; i < ntbl_count; i++) {
        flo = next_byte();
        fhi = next_byte();
        ntbl_freq[i] = (uint16_t)flo | ((uint16_t)fhi << 8);
    }

    dict_count = next_byte();
    for (i = 0; i < dict_count; i++) {
        dlo = next_byte();
        dhi = next_byte();
        dict_delays[i] = (uint16_t)dlo | ((uint16_t)dhi << 8);
    }

    slo = next_byte();
    shi = next_byte();
    sched_len = (uint16_t)slo | ((uint16_t)shi << 8);
    for (loops = 0; loops < sched_len && loops < 512; loops++) {
        sched_table[loops] = next_byte();
    }
    for (; loops < sched_len; loops++) {
        next_byte();
    }
    if (sched_len > 512) sched_len = 512;

    sched_idx = 0;
    next_read_pause = 0;
    while (sched_idx < sched_len && sched_table[sched_idx] == 0x3F) {
        next_read_pause += 63;
        sched_idx++;
    }
    if (sched_idx < sched_len) {
        e = sched_table[sched_idx];
        next_read_pause += (e & 0x3F);
        next_read_blocks = ((e >> 6) & 0x03) + 1;
    }

    for (i = 0; i < 25; i++) shadow_regs[i] = 0;
    for (i = 0; i < 3; i++) {
        shadow_pw[i] = 0x0800;
        last_ctrl[i] = 0;
    }
    lz_head = 0;
    lz_lit_rem = 0;
    lz_match_rem = 0;
    lz_last_offset = 0;
    bit_buf = 0;
    bit_mask = 0;

    sid_reset();
    timer_init();

    while (1) {
        cmd = next_semantic_byte();
        if (cmd == 0xFF) break;

        if (cmd <= 0x18) {
            b = next_semantic_byte();
            SID_WRITE_REG(cmd, b);
            shadow_regs[cmd] = b;
            v = cmd / 7;
            if (v < 3 && (cmd % 7 == 2 || cmd % 7 == 3)) {
                base = v * 7;
                shadow_pw[v] = (uint16_t)shadow_regs[base + 2] | ((uint16_t)shadow_regs[base + 3] << 8);
            }
        } else if (cmd >= 0x19 && cmd <= 0x1B) {
            v = cmd - 0x19;
            note = next_semantic_byte();
            ctrl = next_semantic_byte();
            freq = get_note_freq(note);
            base = v * 7;
            SID_WRITE_REG(base + 0, freq & 0xFF);
            SID_WRITE_REG(base + 1, (freq >> 8) & 0xFF);
            SID_WRITE_REG(base + 4, ctrl);
            shadow_regs[base + 0] = freq & 0xFF;
            shadow_regs[base + 1] = (freq >> 8) & 0xFF;
            shadow_regs[base + 4] = ctrl;
            last_ctrl[v] = ctrl;
        } else if (cmd >= 0x1C && cmd <= 0x1E) {
            v = cmd - 0x1C;
            note = next_semantic_byte();
            ctrl = last_ctrl[v] | 0x01;
            freq = get_note_freq(note);
            base = v * 7;
            SID_WRITE_REG(base + 0, freq & 0xFF);
            SID_WRITE_REG(base + 1, (freq >> 8) & 0xFF);
            SID_WRITE_REG(base + 4, ctrl);
            shadow_regs[base + 0] = freq & 0xFF;
            shadow_regs[base + 1] = (freq >> 8) & 0xFF;
            shadow_regs[base + 4] = ctrl;
            last_ctrl[v] = ctrl;
        } else if (cmd >= 0x1F && cmd <= 0x21) {
            v = cmd - 0x1F;
            b = next_semantic_byte();
            b2 = next_semantic_byte();
            b3 = next_semantic_byte();
            base = v * 7;
            SID_WRITE_REG(base + 0, b);
            SID_WRITE_REG(base + 1, b2);
            SID_WRITE_REG(base + 4, b3);
            shadow_regs[base + 0] = b;
            shadow_regs[base + 1] = b2;
            shadow_regs[base + 4] = b3;
            last_ctrl[v] = b3;
        } else if (cmd >= 0x22 && cmd <= 0x24) {
            v = cmd - 0x22;
            base = v * 7;
            ctrl = shadow_regs[base + 4] & ~0x01;
            SID_WRITE_REG(base + 4, ctrl);
            shadow_regs[base + 4] = ctrl;
        } else if (cmd >= 0x25 && cmd <= 0x27) {
            v = cmd - 0x25;
            base = v * 7;
            pw = shadow_pw[v] + pw_step;
            shadow_pw[v] = pw;
            SID_WRITE_REG(base + 2, pw & 0xFF);
            SID_WRITE_REG(base + 3, (pw >> 8) & 0xFF);
            shadow_regs[base + 2] = pw & 0xFF;
            shadow_regs[base + 3] = (pw >> 8) & 0xFF;
        } else if (cmd >= 0x28 && cmd <= 0x2A) {
            v = cmd - 0x28;
            base = v * 7;
            pw = shadow_pw[v] - pw_step;
            shadow_pw[v] = pw;
            SID_WRITE_REG(base + 2, pw & 0xFF);
            SID_WRITE_REG(base + 3, (pw >> 8) & 0xFF);
            shadow_regs[base + 2] = pw & 0xFF;
            shadow_regs[base + 3] = (pw >> 8) & 0xFF;
        } else if (cmd >= 0x2B && cmd <= 0x2D) {
            v = cmd - 0x2B;
            base = v * 7;
            b = next_semantic_byte();
            b2 = next_semantic_byte();
            shadow_pw[v] = (uint16_t)b | ((uint16_t)b2 << 8);
            SID_WRITE_REG(base + 2, b);
            SID_WRITE_REG(base + 3, b2);
            shadow_regs[base + 2] = b;
            shadow_regs[base + 3] = b2;
        } else if (cmd >= 0x2E && cmd <= 0x30) {
            v = cmd - 0x2E;
            base = v * 7;
            b = next_semantic_byte();
            b2 = next_semantic_byte();
            SID_WRITE_REG(base + 0, b);
            SID_WRITE_REG(base + 1, b2);
            shadow_regs[base + 0] = b;
            shadow_regs[base + 1] = b2;
        } else if (cmd >= 0x31 && cmd <= 0x33) {
            v = cmd - 0x31;
            base = v * 7;
            note = next_semantic_byte();
            freq = get_note_freq(note);
            SID_WRITE_REG(base + 0, freq & 0xFF);
            SID_WRITE_REG(base + 1, (freq >> 8) & 0xFF);
            shadow_regs[base + 0] = freq & 0xFF;
            shadow_regs[base + 1] = (freq >> 8) & 0xFF;
        } else if (cmd == 0x34 || cmd == 0x35 || (cmd >= 0x80 && cmd <= 0xFE)) {
            if (cmd == 0x34) {
                loops = next_semantic_byte();
            } else if (cmd == 0x35) {
                b = next_semantic_byte();
                b2 = next_semantic_byte();
                loops = (uint16_t)b | ((uint16_t)b2 << 8);
            } else {
                loops = dict_delays[cmd - 0x80];
            }

            delay_cycles_left += (int32_t)loops * 15;

            if (sched_idx < sched_len && pause_counter == next_read_pause) {
                for (i = 0; i < next_read_blocks; i++) {
                    if (!eof_reached && count < num_bufs) {
                        r = sd_read(&file, BUF_PTR(head), CHUNK_SIZE);
                        if (r > 0) {
                            head = (head + 1) % num_bufs;
                            count++;
                        } else {
                            eof_reached = 1;
                            break;
                        }
                    }
                }
                sched_idx++;
                next_read_pause++;
                while (sched_idx < sched_len && sched_table[sched_idx] == 0x3F) {
                    next_read_pause += 63;
                    sched_idx++;
                }
                if (sched_idx < sched_len) {
                    e = sched_table[sched_idx];
                    next_read_pause += (e & 0x3F);
                    next_read_blocks = ((e >> 6) & 0x03) + 1;
                }
            }
            pause_counter++;

            while (delay_cycles_left > 0) {
                delay_cycles_left -= timer_delta();
            }
            if (delay_cycles_left < -8000) {
                delay_cycles_left = 0;
            }
        }
    }

    sd_close(&file);
    sid_reset();
    println("Playback finished.");
    return 0;
}
