#ifndef MICRODOS
#define MICRODOS 1
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#pragma clang section text=".overlay_3"
#include "Libs/GPU.h"
#include "Libs/Font3x5.h"
#include "Libs/Mouse.h"
#pragma clang section text=""

#include "Libs/app-bios.h"

MICRODOS_APP_ICON({
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F
});

#define OV1       __attribute__((section(".overlay_1"), noinline))
#define OV2       __attribute__((section(".overlay_2"), noinline))
#define OV3       __attribute__((section(".overlay_3"), noinline))

#define OV1_ENTRY __attribute__((section(".overlay_1"), used, noinline))
#define OV2_ENTRY __attribute__((section(".overlay_2"), used, noinline))
#define OV3_ENTRY __attribute__((section(".overlay_3"), used, noinline))

#define OVSTUB(page, name) typeof(name##_ov) name; \
    __asm__(".pushsection .text." #name ",\"ax\"\n.globl " #name "\n" #name ":\n" \
            "\tjsr overlay_dispatcher\n\t.byte " #page "\n\t.word " #name "_ov\n.popsection")

enum {
    MAX_LINES      = 256,
    MAX_LINE_CHARS = 240,
    VIS_ROWS       = 8,
    VIS_COLS       = 23
};

#define RAW_SWAP_PATH "/SYSTEM/SWAP/BROWRAW.SWP"
#define FMT_SWAP_PATH "/SYSTEM/SWAP/BROWFMT.SWP"

static char     url_buf[128];
static char     status_msg[24];
static uint16_t line_offsets[MAX_LINES];
static uint8_t  line_lens[MAX_LINES];
static uint8_t  line_colors[MAX_LINES];
static uint16_t total_lines;
static uint8_t  max_line_len;
static uint16_t scroll_y;
static uint8_t  scroll_x;

static char     vis_lines[VIS_ROWS][24];
static uint8_t  vis_colors[VIS_ROWS];

static uint8_t  cursor_bg[8][8];
static int16_t  prev_mx = -1;
static int16_t  prev_my = -1;
static bool     cursor_saved;

static bool     url_focused;
static uint8_t  url_len;
static uint8_t  url_scroll;
static bool     raw_mode;
static bool     needs_redraw;
static bool     viewport_dirty;
static bool     dragging_vscroll;
static bool     dragging_hscroll;
static bool     was_mouse_down;
static bool     running;

static SD_FILE  swap_fp;
static SD_FILE  fmt_fp;
static char     net_host[64];
static char     net_path[96];
static char     chunk_buf[128];
static char     fmt_buf[64];

static char to_lower_ascii(char c) {
    if (c >= 'A' && c <= 'Z') {
        return (char)(c + ('a' - 'A'));
    }
    return c;
}

static void set_status(const char* msg) {
    uint8_t i = 0;
    while (msg[i] && i < sizeof(status_msg) - 1U) {
        status_msg[i] = msg[i];
        ++i;
    }
    status_msg[i] = '\0';
}

static uint8_t u16_to_dec(uint16_t val, char* dst) {
    char tmp[6];
    uint8_t n = 0;
    if (val == 0U) {
        dst[0] = '0';
        dst[1] = '\0';
        return 1U;
    }
    while (val > 0U && n < 5U) {
        tmp[n++] = (char)('0' + (val % 10U));
        val /= 10U;
    }
    for (uint8_t i = 0; i < n; ++i) {
        dst[i] = tmp[n - 1U - i];
    }
    dst[n] = '\0';
    return n;
}

static void sync_url_scroll(void) {
    url_len = (uint8_t)strlen(url_buf);
    if (url_len > 19U) {
        url_scroll = (uint8_t)(url_len - 19U);
    } else {
        url_scroll = 0U;
    }
}

static void scroll_by_y(int16_t delta) {
    if (total_lines <= VIS_ROWS) {
        if (scroll_y != 0U) {
            scroll_y = 0U;
            viewport_dirty = true;
            needs_redraw = true;
        }
        return;
    }
    int16_t max_sy = (int16_t)(total_lines - VIS_ROWS);
    int16_t next_sy = (int16_t)scroll_y + delta;
    if (next_sy < 0) {
        next_sy = 0;
    } else if (next_sy > max_sy) {
        next_sy = max_sy;
    }
    if ((uint16_t)next_sy != scroll_y) {
        scroll_y = (uint16_t)next_sy;
        viewport_dirty = true;
        needs_redraw = true;
    }
}

static void scroll_by_x(int16_t delta) {
    if (max_line_len <= VIS_COLS) {
        if (scroll_x != 0U) {
            scroll_x = 0U;
            viewport_dirty = true;
            needs_redraw = true;
        }
        return;
    }
    int16_t max_sx = (int16_t)(max_line_len - VIS_COLS);
    int16_t next_sx = (int16_t)scroll_x + delta;
    if (next_sx < 0) {
        next_sx = 0;
    } else if (next_sx > max_sx) {
        next_sx = max_sx;
    }
    if ((uint8_t)next_sx != scroll_x) {
        scroll_x = (uint8_t)next_sx;
        viewport_dirty = true;
        needs_redraw = true;
    }
}

static bool fetch_url_ov(void);
OVSTUB(0, fetch_url);
static void init_welcome_swap_ov(void);
OVSTUB(0, init_welcome_swap);

static void parse_document_ov(void);
OVSTUB(1, parse_document);
static void load_viewport_ov(void);
OVSTUB(1, load_viewport);

static void render_browser_ov(void);
OVSTUB(2, render_browser);
static void handle_input_ov(void);
OVSTUB(2, handle_input);
static void draw_loading_banner_ov(const char* msg);
OVSTUB(2, draw_loading_banner);
static void clear_screen_ov(void);
OVSTUB(2, clear_screen);

static void browser_reparse(void) {
    scroll_x = 0U;
    scroll_y = 0U;
    parse_document();
    load_viewport();
    needs_redraw = true;
}

static void browser_navigate(void) {
    set_status("CONNECTING...");
    draw_loading_banner(status_msg);
    if (fetch_url()) {
        draw_loading_banner("PARSING...");
        browser_reparse();
        set_status("READY");
    } else {
        needs_redraw = true;
    }
}

/* =========================================================================
 * OVERLAY 1: HTTPS/HTTP Networking & Swap Initialization
 * ========================================================================= */

static const char HTTP_REQ_MID[]  = " HTTP/1.1\r\nHost: ";
static const char HTTP_REQ_TAIL[] = "\r\nUser-Agent: microDOS-Browser/1.0\r\nAccept: text/html,text/plain,*/*\r\nConnection: close\r\n\r\n";

static const char WELCOME_HTML[] =
    "<html><head><title>microDOS Web Browser</title></head><body>"
    "<h1>microDOS Browser 1.0</h1>"
    "<hr>"
    "<p>Welcome to the 65c02 graphical HTTP/HTTPS browser! Click the URL bar above or press GO to fetch any web page via ESP8266 SSL/TCP.</p>"
    "<h2>Controls &amp; Shortcuts</h2>"
    "<ul>"
    "<li>Click URL bar (or press U / Tab) to edit URL</li>"
    "<li>Click GO (or press Enter / G) to load page</li>"
    "<li>Use scrollbars, WASD, or Arrow keys to scroll</li>"
    "<li>Click bottom-right corner (or press R) for Raw HTML</li>"
    "<li>Click X (or press Q / ESC) to exit</li>"
    "</ul>"
    "<hr>"
    "<p>Link example: <a href=\"https://example.com\">example.com</a> &mdash; Ready for browsing.</p>"
    "</body></html>\n";

OV1_ENTRY static void init_welcome_swap_ov(void) {
    sd_mkdir("/SYSTEM");
    sd_mkdir("/SYSTEM/SWAP");
    if (sd_open(&swap_fp, RAW_SWAP_PATH, SD_WRITE | SD_CREATE_ALWAYS)) {
        sd_write(&swap_fp, WELCOME_HTML, (uint16_t)(sizeof(WELCOME_HTML) - 1U));
        sd_close(&swap_fp);
    }
}

OV1 static void net_drain_rx(uint16_t loops) {
    for (uint16_t i = 0; i < loops; ++i) {
        while (net_has_data()) {
            (void)net_getc();
        }
    }
}

OV1 static bool net_wait_byte(char* out, uint16_t timeout_ticks) {
    for (uint16_t t = 0; t < timeout_ticks; ++t) {
        for (uint16_t k = 0; k < 2000U; ++k) {
            if (net_has_data()) {
                *out = (char)net_getc();
                return true;
            }
        }
        char key = bios_getchar_nb();
        if (key == 27 || key == 3) {
            set_status("CANCELLED");
            return false;
        }
    }
    set_status("ERR: TIMEOUT");
    return false;
}

OV1 static void update_rx_status(uint16_t bytes_count) {
    strcpy(status_msg, "RX: ");
    uint8_t n = u16_to_dec(bytes_count, status_msg + 4);
    status_msg[4 + n] = 'B';
    status_msg[5 + n] = '\0';
}

OV1_ENTRY static bool fetch_url_ov(void) {
    const char* p = url_buf;
    while (*p == ' ') {
        ++p;
    }
    if (*p == '\0') {
        set_status("ERR: EMPTY URL");
        return false;
    }

    bool is_ssl = true;
    uint16_t port = 443U;

    if (strncmp(p, "http://", 7) == 0 || strncmp(p, "HTTP://", 7) == 0) {
        is_ssl = false;
        port = 80U;
        p += 7;
    } else if (strncmp(p, "https://", 8) == 0 || strncmp(p, "HTTPS://", 8) == 0) {
        is_ssl = true;
        port = 443U;
        p += 8;
    }

    uint8_t h_idx = 0;
    while (*p && *p != '/' && *p != ':' && *p != '?' && *p != ' ' && h_idx < sizeof(net_host) - 1U) {
        net_host[h_idx++] = *p++;
    }
    net_host[h_idx] = '\0';

    if (net_host[0] == '\0') {
        set_status("ERR: BAD HOST");
        return false;
    }

    if (*p == ':') {
        ++p;
        uint16_t custom_port = 0U;
        while (*p >= '0' && *p <= '9') {
            custom_port = (uint16_t)(custom_port * 10U + (uint8_t)(*p - '0'));
            ++p;
        }
        if (custom_port > 0U) {
            port = custom_port;
        }
    }

    if (*p == '/') {
        uint8_t path_idx = 0;
        while (*p && *p != ' ' && path_idx < sizeof(net_path) - 1U) {
            net_path[path_idx++] = *p++;
        }
        net_path[path_idx] = '\0';
    } else if (*p == '?') {
        net_path[0] = '/';
        uint8_t path_idx = 1;
        while (*p && *p != ' ' && path_idx < sizeof(net_path) - 1U) {
            net_path[path_idx++] = *p++;
        }
        net_path[path_idx] = '\0';
    } else {
        net_path[0] = '/';
        net_path[1] = '\0';
    }

    net_drain_rx(50U);
    net_cmd("AT+CIPCLOSE");
    net_drain_rx(300U);

    if (is_ssl) {
        net_send("AT+CIPSTART=\"SSL\",\"");
    } else {
        net_send("AT+CIPSTART=\"TCP\",\"");
    }
    net_send(net_host);
    net_send("\",");
    net_send_num(port);
    net_send("\r\n");

    char c0 = 0, c1 = 0, c2 = 0, c3 = 0;
    char chr = 0;
    while (true) {
        if (!net_wait_byte(&chr, 600U)) {
            net_cmd("AT+CIPCLOSE");
            return false;
        }
        c0 = c1;
        c1 = c2;
        c2 = c3;
        c3 = chr;
        if (c2 == 'O' && c3 == 'K') {
            while (net_wait_byte(&chr, 50U) && chr != '\n') {}
            break;
        }
        if ((c0 == 'E' && c1 == 'R' && c2 == 'R' && c3 == 'O') ||
            (c0 == 'F' && c1 == 'A' && c2 == 'I' && c3 == 'L')) {
            while (net_wait_byte(&chr, 50U) && chr != '\n') {}
            set_status("ERR: CONNECT");
            return false;
        }
    }

    uint16_t req_len = (uint16_t)(4U + strlen(net_path) + (sizeof(HTTP_REQ_MID) - 1U) +
                                  strlen(net_host) + (sizeof(HTTP_REQ_TAIL) - 1U));
    net_send("AT+CIPSEND=");
    net_send_num(req_len);
    net_send("\r\n");

    c0 = 0; c1 = 0; c2 = 0; c3 = 0;
    while (true) {
        if (!net_wait_byte(&chr, 400U)) {
            net_cmd("AT+CIPCLOSE");
            return false;
        }
        if (chr == '>') {
            break;
        }
        c0 = c1; c1 = c2; c2 = c3; c3 = chr;
        if (c0 == 'E' && c1 == 'R' && c2 == 'R' && c3 == 'O') {
            set_status("ERR: CIPSEND");
            net_cmd("AT+CIPCLOSE");
            return false;
        }
    }

    net_send("GET ");
    net_send(net_path);
    net_send(HTTP_REQ_MID);
    net_send(net_host);
    net_send(HTTP_REQ_TAIL);

    sd_mkdir("/SYSTEM");
    sd_mkdir("/SYSTEM/SWAP");
    if (!sd_open(&swap_fp, RAW_SWAP_PATH, SD_WRITE | SD_CREATE_ALWAYS)) {
        set_status("ERR: SD OPEN");
        net_cmd("AT+CIPCLOSE");
        return false;
    }

    uint32_t bytes_to_read = 0U;
    bool in_packet = false;
    bool header_finished = false;
    uint8_t lf_count = 0U;
    uint8_t chunk_len = 0U;
    uint16_t total_rx = 0U;
    c0 = 0; c1 = 0; c2 = 0; c3 = 0;

    while (true) {
        uint16_t wait_ticks = (total_rx > 0U || header_finished) ? 1500U : 6000U;
        if (!net_wait_byte(&chr, wait_ticks)) {
            if (total_rx > 0U || header_finished) {
                break;
            }
            sd_close(&swap_fp);
            net_cmd("AT+CIPCLOSE");
            return false;
        }

        if (!in_packet) {
            if (chr == '+') {
                char i1 = 0, i2 = 0, i3 = 0, i4 = 0;
                if (net_wait_byte(&i1, 100U) && i1 == 'I' &&
                    net_wait_byte(&i2, 100U) && i2 == 'P' &&
                    net_wait_byte(&i3, 100U) && i3 == 'D' &&
                    net_wait_byte(&i4, 100U) && i4 == ',') {
                    bytes_to_read = 0U;
                    while (net_wait_byte(&chr, 100U)) {
                        if (chr == ':') {
                            break;
                        }
                        if (chr >= '0' && chr <= '9') {
                            bytes_to_read = (bytes_to_read * 10U) + (uint32_t)(chr - '0');
                        }
                    }
                    if (bytes_to_read > 0U) {
                        in_packet = true;
                    }
                    continue;
                }
            }

            c0 = c1; c1 = c2; c2 = c3; c3 = chr;
            if ((c0 == 'C' && c1 == 'L' && c2 == 'O' && c3 == 'S') ||
                (c0 == 'E' && c1 == 'R' && c2 == 'R' && c3 == 'O')) {
                break;
            }
            continue;
        }

        if (header_finished) {
            if (total_rx < 65000U) {
                chunk_buf[chunk_len++] = chr;
                ++total_rx;
                if (chunk_len == sizeof(chunk_buf)) {
                    sd_write(&swap_fp, chunk_buf, sizeof(chunk_buf));
                    chunk_len = 0U;
                    update_rx_status(total_rx);
                }
            }
        } else {
            if (chr == '\n') {
                ++lf_count;
                if (lf_count >= 2U) {
                    header_finished = true;
                }
            } else if (chr != '\r') {
                lf_count = 0U;
            }
        }

        --bytes_to_read;
        if (bytes_to_read == 0U) {
            in_packet = false;
            c0 = 0; c1 = 0; c2 = 0; c3 = 0;
        }
    }

    if (chunk_len > 0U) {
        sd_write(&swap_fp, chunk_buf, chunk_len);
    }
    sd_close(&swap_fp);
    net_cmd("AT+CIPCLOSE");
    update_rx_status(total_rx);
    return true;
}

/* =========================================================================
 * OVERLAY 2: HTML Parser, Formatter & Viewport Loader
 * ========================================================================= */

static uint8_t  rd_idx;
static uint8_t  rd_len;
static uint8_t  wr_idx;
static uint16_t fmt_pos;
static uint16_t cur_line_offset;
static uint8_t  cur_line_len;
static uint8_t  cur_line_color;
static uint8_t  active_color;
static bool     last_was_space;
static bool     last_line_blank;
static bool     in_pre;

OV2 static int16_t raw_next_char(void) {
    if (rd_idx >= rd_len) {
        int16_t n = sd_read(&swap_fp, chunk_buf, sizeof(chunk_buf));
        if (n <= 0) {
            return -1;
        }
        rd_len = (uint8_t)n;
        rd_idx = 0U;
    }
    return (uint8_t)chunk_buf[rd_idx++];
}

OV2 static void fmt_write_byte(char c) {
    fmt_buf[wr_idx++] = c;
    if (wr_idx == sizeof(fmt_buf)) {
        sd_write(&fmt_fp, fmt_buf, sizeof(fmt_buf));
        wr_idx = 0U;
    }
    ++fmt_pos;
}

OV2 static void fmt_flush_buf(void) {
    if (wr_idx > 0U) {
        sd_write(&fmt_fp, fmt_buf, wr_idx);
        wr_idx = 0U;
    }
}

OV2 static void fmt_flush_line(bool allow_empty) {
    if (total_lines >= MAX_LINES || fmt_pos >= 65000U) {
        return;
    }
    if (cur_line_len == 0U) {
        if (!allow_empty || last_line_blank) {
            return;
        }
    }
    line_offsets[total_lines] = cur_line_offset;
    line_lens[total_lines] = cur_line_len;
    line_colors[total_lines] = cur_line_color;
    if (cur_line_len > max_line_len) {
        max_line_len = cur_line_len;
    }
    ++total_lines;
    fmt_write_byte('\n');
    last_line_blank = (cur_line_len == 0U);
    cur_line_offset = fmt_pos;
    cur_line_len = 0U;
    cur_line_color = active_color;
    last_was_space = true;
}

OV2 static void fmt_emit_char(char c) {
    if (total_lines >= MAX_LINES || fmt_pos >= 65000U) {
        return;
    }
    if (in_pre) {
        if (c == '\r') {
            return;
        }
        if (c == '\n') {
            fmt_flush_line(true);
            return;
        }
    }
    if ((uint8_t)c < 32U || (uint8_t)c > 126U) {
        c = ' ';
    }
    if (!in_pre) {
        if (c == ' ') {
            if (last_was_space || cur_line_len == 0U) {
                return;
            }
            last_was_space = true;
        } else {
            last_was_space = false;
        }
    }
    if (cur_line_len == 0U) {
        cur_line_offset = fmt_pos;
        cur_line_color = active_color;
    }
    fmt_write_byte(c);
    ++cur_line_len;
    last_line_blank = false;
    if (cur_line_len >= MAX_LINE_CHARS) {
        fmt_flush_line(false);
    }
}

OV2 static void fmt_emit_str(const char* s) {
    while (*s) {
        fmt_emit_char(*s++);
    }
}

OV2 static void parse_raw_document(void) {
    if (!sd_open(&swap_fp, RAW_SWAP_PATH, SD_READ)) {
        return;
    }
    rd_idx = 0U;
    rd_len = 0U;
    uint16_t file_pos = 0U;
    uint16_t line_start = 0U;
    uint8_t line_len = 0U;
    uint8_t line_col = 0x00U;

    int16_t ch;
    while ((ch = raw_next_char()) >= 0 && total_lines < MAX_LINES && file_pos < 65200U) {
        char c = (char)ch;
        if (c == '\r') {
            if (line_len == 0U) {
                line_start = (uint16_t)(file_pos + 1U);
            }
        } else if (c == '\n') {
            line_offsets[total_lines] = line_start;
            line_lens[total_lines] = line_len;
            line_colors[total_lines] = line_col;
            if (line_len > max_line_len) {
                max_line_len = line_len;
            }
            ++total_lines;
            line_start = (uint16_t)(file_pos + 1U);
            line_len = 0U;
            line_col = 0x00U;
        } else {
            if (line_len == 0U) {
                line_start = file_pos;
            }
            if (c == '<') {
                line_col = 0x03U;
            }
            ++line_len;
            if (line_len >= MAX_LINE_CHARS) {
                line_offsets[total_lines] = line_start;
                line_lens[total_lines] = line_len;
                line_colors[total_lines] = line_col;
                if (line_len > max_line_len) {
                    max_line_len = line_len;
                }
                ++total_lines;
                line_start = (uint16_t)(file_pos + 1U);
                line_len = 0U;
                line_col = 0x00U;
            }
        }
        ++file_pos;
    }

    if (line_len > 0U && total_lines < MAX_LINES) {
        line_offsets[total_lines] = line_start;
        line_lens[total_lines] = line_len;
        line_colors[total_lines] = line_col;
        if (line_len > max_line_len) {
            max_line_len = line_len;
        }
        ++total_lines;
    }
    sd_close(&swap_fp);
}

OV2 static void decode_and_emit_entity(void) {
    char ent[10];
    uint8_t len = 0U;
    int16_t ch = 0;
    bool ended_semi = false;

    while (len < sizeof(ent) - 1U) {
        ch = raw_next_char();
        if (ch < 0) {
            break;
        }
        if (ch == ';') {
            ended_semi = true;
            break;
        }
        if (ch == ' ' || ch == '<' || ch == '\n' || ch == '\r' || ch == '&') {
            break;
        }
        ent[len++] = to_lower_ascii((char)ch);
    }
    ent[len] = '\0';

    if (!ended_semi) {
        fmt_emit_char('&');
        fmt_emit_str(ent);
        if (ch >= 0 && ch != '<') {
            fmt_emit_char((char)ch);
        }
        return;
    }

    if (strcmp(ent, "amp") == 0) {
        fmt_emit_char('&');
    } else if (strcmp(ent, "lt") == 0) {
        fmt_emit_char('<');
    } else if (strcmp(ent, "gt") == 0) {
        fmt_emit_char('>');
    } else if (strcmp(ent, "quot") == 0) {
        fmt_emit_char('"');
    } else if (strcmp(ent, "apos") == 0 || strcmp(ent, "#39") == 0) {
        fmt_emit_char('\'');
    } else if (strcmp(ent, "nbsp") == 0) {
        fmt_emit_char(' ');
    } else if (strcmp(ent, "copy") == 0) {
        fmt_emit_str("(c)");
    } else if (strcmp(ent, "mdash") == 0 || strcmp(ent, "ndash") == 0 || strcmp(ent, "#8212") == 0 || strcmp(ent, "#8211") == 0) {
        fmt_emit_char('-');
    } else if (strcmp(ent, "bull") == 0) {
        fmt_emit_char('*');
    } else if (ent[0] == '#') {
        uint16_t code = 0U;
        if (ent[1] == 'x') {
            for (uint8_t i = 2U; ent[i]; ++i) {
                char d = ent[i];
                code <<= 4U;
                if (d >= '0' && d <= '9') {
                    code |= (uint16_t)(d - '0');
                } else if (d >= 'a' && d <= 'f') {
                    code |= (uint16_t)(d - 'a' + 10);
                }
            }
        } else {
            for (uint8_t i = 1U; ent[i]; ++i) {
                if (ent[i] >= '0' && ent[i] <= '9') {
                    code = (uint16_t)(code * 10U + (uint8_t)(ent[i] - '0'));
                }
            }
        }
        if (code >= 32U && code <= 126U) {
            fmt_emit_char((char)code);
        } else {
            fmt_emit_char('-');
        }
    }
}

OV2_ENTRY static void parse_document_ov(void) {
    total_lines = 0U;
    max_line_len = 0U;
    viewport_dirty = true;

    if (raw_mode) {
        parse_raw_document();
        return;
    }

    if (!sd_open(&swap_fp, RAW_SWAP_PATH, SD_READ)) {
        return;
    }
    if (!sd_open(&fmt_fp, FMT_SWAP_PATH, SD_WRITE | SD_CREATE_ALWAYS)) {
        sd_close(&swap_fp);
        return;
    }

    rd_idx = 0U;
    rd_len = 0U;
    wr_idx = 0U;
    fmt_pos = 0U;
    cur_line_offset = 0U;
    cur_line_len = 0U;
    cur_line_color = 0x00U;
    active_color = 0x00U;
    last_was_space = true;
    last_line_blank = true;
    in_pre = false;

    uint8_t skip_mode = 0U; // 0=none, 1=script, 2=style, 3=svg
    int16_t ch;

    while ((ch = raw_next_char()) >= 0 && total_lines < MAX_LINES && fmt_pos < 65000U) {
        if (ch == '<') {
            int16_t n1 = raw_next_char();
            if (n1 < 0) {
                break;
            }
            if (n1 == '!') {
                int16_t n2 = raw_next_char();
                int16_t n3 = raw_next_char();
                if (n2 == '-' && n3 == '-') {
                    int16_t p0 = 0, p1 = 0, p2 = 0;
                    while ((p2 = raw_next_char()) >= 0) {
                        if (p0 == '-' && p1 == '-' && p2 == '>') {
                            break;
                        }
                        p0 = p1;
                        p1 = p2;
                    }
                } else {
                    while (n3 >= 0 && n3 != '>') {
                        n3 = raw_next_char();
                    }
                }
                continue;
            }

            char tag[12];
            uint8_t tlen = 0U;
            int16_t cur = n1;
            while (cur == ' ' || cur == '\t' || cur == '\r' || cur == '\n') {
                cur = raw_next_char();
            }
            while (cur >= 0 && cur != '>' && cur != ' ' && cur != '\t' && cur != '\r' && cur != '\n') {
                if (tlen < sizeof(tag) - 1U) {
                    tag[tlen++] = to_lower_ascii((char)cur);
                }
                cur = raw_next_char();
            }
            if (tlen > 1U && tag[tlen - 1U] == '/') {
                --tlen;
            }
            tag[tlen] = '\0';

            while (cur >= 0 && cur != '>') {
                cur = raw_next_char();
            }

            if (skip_mode != 0U) {
                if ((skip_mode == 1U && strcmp(tag, "/script") == 0) ||
                    (skip_mode == 2U && strcmp(tag, "/style") == 0) ||
                    (skip_mode == 3U && strcmp(tag, "/svg") == 0)) {
                    skip_mode = 0U;
                }
                continue;
            }

            if (strcmp(tag, "script") == 0) {
                skip_mode = 1U;
            } else if (strcmp(tag, "style") == 0) {
                skip_mode = 2U;
            } else if (strcmp(tag, "svg") == 0) {
                skip_mode = 3U;
            } else if (strcmp(tag, "title") == 0) {
                fmt_flush_line(false);
                active_color = 0x30U;
                cur_line_color = 0x30U;
                fmt_emit_str("[TITLE] ");
            } else if (strcmp(tag, "/title") == 0) {
                fmt_flush_line(false);
                active_color = 0x00U;
                fmt_flush_line(true);
            } else if (tag[0] == 'h' && tag[1] >= '1' && tag[1] <= '6' && tag[2] == '\0') {
                fmt_flush_line(false);
                fmt_flush_line(true);
                active_color = (tag[1] == '1') ? 0x30U : 0x20U;
                cur_line_color = active_color;
                fmt_emit_str("# ");
            } else if (tag[0] == '/' && tag[1] == 'h' && tag[2] >= '1' && tag[2] <= '6' && tag[3] == '\0') {
                fmt_flush_line(false);
                active_color = 0x00U;
                fmt_flush_line(true);
            } else if (strcmp(tag, "p") == 0 || strcmp(tag, "/p") == 0) {
                fmt_flush_line(false);
                active_color = 0x00U;
                fmt_flush_line(true);
            } else if (strcmp(tag, "br") == 0 || strcmp(tag, "div") == 0 || strcmp(tag, "/div") == 0 ||
                       strcmp(tag, "tr") == 0 || strcmp(tag, "/tr") == 0 || strcmp(tag, "/table") == 0 ||
                       strcmp(tag, "header") == 0 || strcmp(tag, "/header") == 0 ||
                       strcmp(tag, "footer") == 0 || strcmp(tag, "/footer") == 0 ||
                       strcmp(tag, "section") == 0 || strcmp(tag, "/section") == 0 ||
                       strcmp(tag, "article") == 0 || strcmp(tag, "/article") == 0) {
                fmt_flush_line(false);
            } else if (strcmp(tag, "hr") == 0) {
                fmt_flush_line(false);
                active_color = 0x15U;
                cur_line_color = 0x15U;
                fmt_emit_str("-----------------------");
                fmt_flush_line(false);
                active_color = 0x00U;
            } else if (strcmp(tag, "li") == 0) {
                fmt_flush_line(false);
                active_color = 0x00U;
                fmt_emit_str("* ");
            } else if (strcmp(tag, "/li") == 0 || strcmp(tag, "ul") == 0 || strcmp(tag, "/ul") == 0 ||
                       strcmp(tag, "ol") == 0 || strcmp(tag, "/ol") == 0) {
                fmt_flush_line(false);
            } else if (strcmp(tag, "a") == 0) {
                active_color = 0x03U;
                if (cur_line_len == 0U) {
                    cur_line_color = 0x03U;
                }
                fmt_emit_char('[');
            } else if (strcmp(tag, "/a") == 0) {
                fmt_emit_char(']');
                active_color = 0x00U;
            } else if (strcmp(tag, "pre") == 0) {
                fmt_flush_line(false);
                in_pre = true;
                active_color = 0x15U;
            } else if (strcmp(tag, "/pre") == 0) {
                fmt_flush_line(false);
                in_pre = false;
                active_color = 0x00U;
            } else if (strcmp(tag, "td") == 0 || strcmp(tag, "th") == 0) {
                fmt_emit_char(' ');
            }
            continue;
        }

        if (skip_mode != 0U) {
            continue;
        }

        if (ch == '&') {
            decode_and_emit_entity();
        } else {
            fmt_emit_char((char)ch);
        }
    }

    fmt_flush_line(false);
    fmt_flush_buf();
    sd_close(&fmt_fp);
    sd_close(&swap_fp);
}

OV2_ENTRY static void load_viewport_ov(void) {
    for (uint8_t r = 0U; r < VIS_ROWS; ++r) {
        vis_lines[r][0] = '\0';
        vis_colors[r] = 0x00U;
    }

    const char* active_path = raw_mode ? RAW_SWAP_PATH : FMT_SWAP_PATH;
    if (total_lines > 0U && sd_open(&swap_fp, active_path, SD_READ)) {
        for (uint8_t r = 0U; r < VIS_ROWS; ++r) {
            uint16_t line_idx = (uint16_t)(scroll_y + r);
            if (line_idx >= total_lines) {
                break;
            }
            vis_colors[r] = line_colors[line_idx];
            uint8_t llen = line_lens[line_idx];
            if (scroll_x < llen) {
                uint8_t avail = (uint8_t)(llen - scroll_x);
                if (avail > VIS_COLS) {
                    avail = VIS_COLS;
                }
                sd_seek(&swap_fp, (uint32_t)line_offsets[line_idx] + (uint32_t)scroll_x);
                int16_t got = sd_read(&swap_fp, vis_lines[r], avail);
                if (got < 0) {
                    got = 0;
                }
                for (int16_t k = 0; k < got; ++k) {
                    uint8_t uc = (uint8_t)vis_lines[r][k];
                    if (uc < 32U || uc > 126U) {
                        vis_lines[r][k] = ' ';
                    }
                }
                vis_lines[r][got] = '\0';
            }
        }
        sd_close(&swap_fp);
    }
    viewport_dirty = false;
}

/* =========================================================================
 * OVERLAY 3: GPU Canvas, Font3x5 UI, Scrollbars & Mouse/Keyboard Input
 * ========================================================================= */

#pragma clang section text=".overlay_3"

OV3 static void restore_cursor_bg(void) {
    if (!cursor_saved) {
        return;
    }
    for (uint8_t r = 0U; r < 8U; ++r) {
        int16_t py = prev_my + (int16_t)r;
        if (py < 0 || py >= GPU_HEIGHT) {
            continue;
        }
        uint8_t* vram_row = (uint8_t*)(GPU_VRAM_START + ((uint16_t)py << 7U));
        for (uint8_t c = 0U; c < 8U; ++c) {
            int16_t px = prev_mx + (int16_t)c;
            if (px >= 0 && px < GPU_WIDTH) {
                vram_row[(uint8_t)px] = cursor_bg[r][c];
            }
        }
    }
    cursor_saved = false;
}

OV3 static void save_and_draw_cursor(uint8_t mx, uint8_t my) {
    for (uint8_t r = 0U; r < 8U; ++r) {
        int16_t py = (int16_t)my + (int16_t)r;
        if (py < 0 || py >= GPU_HEIGHT) {
            continue;
        }
        uint8_t* vram_row = (uint8_t*)(GPU_VRAM_START + ((uint16_t)py << 7U));
        for (uint8_t c = 0U; c < 8U; ++c) {
            int16_t px = (int16_t)mx + (int16_t)c;
            if (px >= 0 && px < GPU_WIDTH) {
                cursor_bg[r][c] = vram_row[(uint8_t)px];
            }
        }
    }
    prev_mx = (int16_t)mx;
    prev_my = (int16_t)my;
    cursor_saved = true;
    mouse_draw_cursor(mx, my);
}

OV3_ENTRY static void draw_loading_banner_ov(const char* msg) {
    restore_cursor_bg();
    gpu_draw_rect(6, 25, 84, 15, 0x03);
    gpu_draw_rect(7, 26, 82, 13, 0x00);
    font3x5_draw(10, 30, msg, 0x3F);
    save_and_draw_cursor(mouse_get_x(), mouse_get_y());
}

OV3_ENTRY static void clear_screen_ov(void) {
    gpu_fill_screen(0x00);
}

OV3_ENTRY static void render_browser_ov(void) {
    cursor_saved = false;

    // Top URL bar (Y = 0..8)
    gpu_draw_rect(0, 0, 100, 9, 0x15);
    gpu_draw_rect(0, 8, 100, 1, 0x2A);

    // URL input box (X = 1..81, Y = 1..7)
    gpu_draw_rect(1, 1, 81, 7, url_focused ? 0x01 : 0x00);
    char url_vis[21];
    uint8_t max_chars = url_focused ? 19U : 20U;
    uint8_t u_idx = 0U;
    while (u_idx < max_chars && (uint16_t)(url_scroll + u_idx) < url_len && url_buf[url_scroll + u_idx]) {
        url_vis[u_idx] = url_buf[url_scroll + u_idx];
        ++u_idx;
    }
    url_vis[u_idx] = '\0';
    font3x5_draw(2, 2, url_vis, 0x3F);
    if (url_focused) {
        gpu_draw_rect((int8_t)(2 + u_idx * 4U), 2, 2, 5, 0x0F);
    }

    // GO button (X = 83..93, Y = 1..7)
    gpu_draw_rect(83, 1, 11, 7, 0x0C);
    font3x5_draw(85, 2, "GO", 0x00);

    // Close X button (X = 95..98, Y = 1..7)
    gpu_draw_rect(95, 1, 4, 7, 0x30);
    font3x5_draw(96, 2, 'X', 0x3F);

    // HTML canvas (X = 0..95, Y = 9..59)
    gpu_draw_rect(0, 9, 96, 51, 0x3F);
    for (uint8_t r = 0U; r < VIS_ROWS; ++r) {
        if (vis_lines[r][0] != '\0') {
            font3x5_draw(2, (int8_t)(10 + r * 6U), vis_lines[r], vis_colors[r]);
        }
    }
    if (status_msg[0] == 'E' || status_msg[0] == 'C') {
        gpu_draw_rect(2, 51, 92, 7, 0x30);
        font3x5_draw(4, 52, status_msg, 0x3F);
    }

    // Vertical scrollbar (X = 96..99, Y = 9..59)
    gpu_draw_rect(96, 9, 4, 51, 0x2A);
    gpu_draw_rect(96, 9, 4, 4, 0x15);
    gpu_put_pixel(97, 10, 0x3F);
    gpu_put_pixel(98, 10, 0x3F);
    gpu_draw_rect(96, 56, 4, 4, 0x15);
    gpu_put_pixel(97, 58, 0x3F);
    gpu_put_pixel(98, 58, 0x3F);

    if (total_lines > VIS_ROWS) {
        uint16_t max_sy = (uint16_t)(total_lines - VIS_ROWS);
        uint8_t thumb_h = (uint8_t)(((uint16_t)43U * VIS_ROWS) / total_lines);
        if (thumb_h < 4U) {
            thumb_h = 4U;
        } else if (thumb_h > 43U) {
            thumb_h = 43U;
        }
        uint8_t travel = (uint8_t)(43U - thumb_h);
        uint8_t thumb_y = (uint8_t)(13U + ((uint32_t)scroll_y * travel) / max_sy);
        gpu_draw_rect(96, (int8_t)thumb_y, 4, (int8_t)thumb_h, 0x03);
    } else {
        gpu_draw_rect(96, 13, 4, 43, 0x2A);
    }

    // Horizontal scrollbar (X = 0..95, Y = 60..63)
    gpu_draw_rect(0, 60, 96, 4, 0x2A);
    gpu_draw_rect(0, 60, 4, 4, 0x15);
    gpu_put_pixel(1, 61, 0x3F);
    gpu_put_pixel(1, 62, 0x3F);
    gpu_draw_rect(92, 60, 4, 4, 0x15);
    gpu_put_pixel(94, 61, 0x3F);
    gpu_put_pixel(94, 62, 0x3F);

    if (max_line_len > VIS_COLS) {
        uint8_t max_sx = (uint8_t)(max_line_len - VIS_COLS);
        uint8_t thumb_w = (uint8_t)(((uint16_t)88U * VIS_COLS) / max_line_len);
        if (thumb_w < 6U) {
            thumb_w = 6U;
        } else if (thumb_w > 88U) {
            thumb_w = 88U;
        }
        uint8_t travel = (uint8_t)(88U - thumb_w);
        uint8_t thumb_x = (uint8_t)(4U + ((uint16_t)scroll_x * travel) / max_sx);
        gpu_draw_rect((int8_t)thumb_x, 60, (int8_t)thumb_w, 4, 0x03);
    } else {
        gpu_draw_rect(4, 60, 88, 4, 0x2A);
    }

    // Raw/Rendered mode toggle button (X = 96..99, Y = 60..63)
    gpu_draw_rect(96, 60, 4, 4, raw_mode ? 0x0F : 0x15);
    gpu_draw_rect(97, 61, 2, 2, raw_mode ? 0x00 : 0x3F);

    save_and_draw_cursor(mouse_get_x(), mouse_get_y());
    needs_redraw = false;
}

OV3_ENTRY static void handle_input_ov(void) {
    uint8_t mx = mouse_get_x();
    uint8_t my = mouse_get_y();
    bool down = mouse_btn_left();
    bool clicked = (down && !was_mouse_down);

    if (!down) {
        dragging_vscroll = false;
        dragging_hscroll = false;
    }

    if (clicked) {
        if (mx >= 1U && mx < 82U && my >= 1U && my < 8U) {
            if (!url_focused) {
                url_focused = true;
                sync_url_scroll();
                needs_redraw = true;
            }
        } else if (mx >= 83U && mx < 94U && my >= 1U && my < 8U) {
            url_focused = false;
            was_mouse_down = down;
            browser_navigate();
            return;
        } else if (mx >= 95U && mx < 99U && my >= 1U && my < 8U) {
            running = false;
            return;
        } else if (mx >= 96U && mx < 100U && my >= 60U && my < 64U) {
            raw_mode = !raw_mode;
            was_mouse_down = down;
            browser_reparse();
            return;
        } else if (mx >= 96U && mx < 100U && my >= 9U && my < 13U) {
            url_focused = false;
            scroll_by_y(-1);
        } else if (mx >= 96U && mx < 100U && my >= 56U && my < 60U) {
            url_focused = false;
            scroll_by_y(1);
        } else if (mx >= 96U && mx < 100U && my >= 13U && my < 56U) {
            url_focused = false;
            dragging_vscroll = true;
        } else if (mx < 4U && my >= 60U && my < 64U) {
            url_focused = false;
            scroll_by_x(-2);
        } else if (mx >= 92U && mx < 96U && my >= 60U && my < 64U) {
            url_focused = false;
            scroll_by_x(2);
        } else if (mx >= 4U && mx < 92U && my >= 60U && my < 64U) {
            url_focused = false;
            dragging_hscroll = true;
        } else if (mx < 96U && my >= 9U && my < 60U) {
            if (url_focused) {
                url_focused = false;
                needs_redraw = true;
            }
        }
    }

    if (down && dragging_vscroll && total_lines > VIS_ROWS) {
        uint8_t cy = (my < 13U) ? 13U : ((my > 55U) ? 55U : my);
        uint16_t max_sy = (uint16_t)(total_lines - VIS_ROWS);
        uint16_t next_sy = (uint16_t)(((uint32_t)(cy - 13U) * max_sy) / 42U);
        if (next_sy != scroll_y) {
            scroll_y = next_sy;
            viewport_dirty = true;
            needs_redraw = true;
        }
    }

    if (down && dragging_hscroll && max_line_len > VIS_COLS) {
        uint8_t cx = (mx < 4U) ? 4U : ((mx > 91U) ? 91U : mx);
        uint8_t max_sx = (uint8_t)(max_line_len - VIS_COLS);
        uint8_t next_sx = (uint8_t)(((uint16_t)(cx - 4U) * max_sx) / 87U);
        if (next_sx != scroll_x) {
            scroll_x = next_sx;
            viewport_dirty = true;
            needs_redraw = true;
        }
    }

    was_mouse_down = down;

    char ch = bios_getchar_nb();
    if (ch != 0) {
        if (ch == 3) {
            running = false;
            return;
        }
        if (ch == 27) {
            char seq1 = 0;
            for (uint16_t i = 0; i < 80U; ++i) {
                seq1 = bios_getchar_nb();
                if (seq1 != 0) {
                    break;
                }
            }
            if (seq1 == '[') {
                char seq2 = 0;
                for (uint16_t i = 0; i < 80U; ++i) {
                    seq2 = bios_getchar_nb();
                    if (seq2 != 0) {
                        break;
                    }
                }
                if (seq2 == 'A') {
                    scroll_by_y(-1);
                } else if (seq2 == 'B') {
                    scroll_by_y(1);
                } else if (seq2 == 'C') {
                    scroll_by_x(2);
                } else if (seq2 == 'D') {
                    scroll_by_x(-2);
                }
            } else {
                if (url_focused) {
                    url_focused = false;
                    needs_redraw = true;
                } else {
                    running = false;
                    return;
                }
            }
        } else if (url_focused) {
            if (ch == '\r' || ch == '\n') {
                url_focused = false;
                browser_navigate();
                return;
            } else if (ch == '\t') {
                url_focused = false;
                needs_redraw = true;
            } else if (ch == 8 || ch == 127) {
                if (url_len > 0U) {
                    --url_len;
                    url_buf[url_len] = '\0';
                    sync_url_scroll();
                    needs_redraw = true;
                }
            } else if (ch == 21) {
                url_len = 0U;
                url_buf[0] = '\0';
                url_scroll = 0U;
                needs_redraw = true;
            } else if ((uint8_t)ch >= 32U && (uint8_t)ch <= 126U) {
                if (url_len < sizeof(url_buf) - 2U) {
                    url_buf[url_len++] = ch;
                    url_buf[url_len] = '\0';
                    sync_url_scroll();
                    needs_redraw = true;
                }
            }
        } else {
            if (ch == 'w' || ch == 'W') {
                scroll_by_y(-1);
            } else if (ch == 's' || ch == 'S' || ch == ' ') {
                scroll_by_y(1);
            } else if (ch == 'a' || ch == 'A') {
                scroll_by_x(-2);
            } else if (ch == 'd' || ch == 'D') {
                scroll_by_x(2);
            } else if (ch == '\t' || ch == 'u' || ch == 'U' || ch == '/') {
                url_focused = true;
                sync_url_scroll();
                needs_redraw = true;
            } else if (ch == 'r' || ch == 'R') {
                raw_mode = !raw_mode;
                browser_reparse();
                return;
            } else if (ch == '\r' || ch == '\n' || ch == 'g' || ch == 'G') {
                browser_navigate();
                return;
            } else if (ch == 'q' || ch == 'Q') {
                running = false;
                return;
            }
        }
    }

    if (!needs_redraw && !viewport_dirty) {
        if (!cursor_saved || mx != (uint8_t)prev_mx || my != (uint8_t)prev_my) {
            restore_cursor_bg();
            save_and_draw_cursor(mx, my);
        }
    }
}

#pragma clang section text=""

int main(void) {
    char** args = _args_ptr;
    bool auto_fetch = false;

    if (arg_count > 1U && args && args[1] && args[1][0] != '\0') {
        strncpy(url_buf, args[1], sizeof(url_buf) - 1U);
        url_buf[sizeof(url_buf) - 1U] = '\0';
        auto_fetch = true;
    } else {
        strcpy(url_buf, "https://example.com");
    }

    sync_url_scroll();
    set_status("READY");
    raw_mode = false;
    url_focused = false;
    running = true;
    cursor_saved = false;
    prev_mx = -1;
    prev_my = -1;

    init_welcome_swap();
    browser_reparse();
    render_browser();

    if (auto_fetch) {
        browser_navigate();
    }

    while (running) {
        if (viewport_dirty) {
            load_viewport();
            needs_redraw = true;
        }
        if (needs_redraw) {
            render_browser();
        }
        handle_input();
    }

    sd_remove(RAW_SWAP_PATH);
    sd_remove(FMT_SWAP_PATH);
    clear_screen();
    return 0;
}
