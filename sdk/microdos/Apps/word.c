/*
 * Word.c — TUI Word Processor with Streaming & Swap Paging for microDOS
 * Compile with: ./sdk/scripts/compile-bin.sh Word --microDOS
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "Libs/app-bios.h"
#include "Libs/tui.h"

enum : uint16_t {
    PAGE_CAP = 2048U,
    LOAD_CHUNK_SIZE = 1536U,
    SPLIT_KEEP_SIZE = 1024U
};

enum : uint8_t {
    MAX_PAGES = 32U,
    FILENAME_MAX_LEN = 32U,
    WIN_ROW = 2U,
    WIN_COL = 2U,
    WIN_WIDTH = 76U,
    WIN_HEIGHT = 22U,
    TEXT_ROW = 4U,
    TEXT_COL = 4U,
    TEXT_ROWS = 19U,
    TEXT_COLS = 72U,
    CTRL_N = 14U,
    CTRL_O = 15U,
    CTRL_P = 16U,
    CTRL_Q = 17U,
    CTRL_S = 19U,
    KEY_BACKSPACE = 8U,
    KEY_DEL_ASCII = 127U
};

static char page_buf[PAGE_CAP];
static char cache_buf[PAGE_CAP];
static char screen_cache[TEXT_ROWS][TEXT_COLS];
static uint8_t screen_row_len[TEXT_ROWS];
static uint8_t prev_row_len[TEXT_ROWS];

static uint16_t page_lens[MAX_PAGES];
static uint32_t page_file_offsets[MAX_PAGES];
static uint8_t page_in_swap[MAX_PAGES];
static uint8_t page_on_disk_src[MAX_PAGES];

static uint8_t total_pages = 1U;
static uint8_t cur_page = 0U;
static uint16_t cur_len = 0U;
static uint16_t cursor_idx = 0U;
static uint16_t scroll_row = 0U;
static uint16_t prev_scroll_row = 0xFFFFU;
static bool page_dirty = false;
static bool doc_modified = false;
static bool has_source_file = false;
static bool status_dirty = true;
static bool force_full_redraw = true;
static uint8_t ram_cache_page = 0xFFU;
static char current_file[FILENAME_MAX_LEN] = "UNTITLED.TXT";
static char status_msg[40] = "Ready";

static void invalidate_screen(void) {
    for (uint8_t r = 0U; r < TEXT_ROWS; ++r) {
        for (uint8_t c = 0U; c < TEXT_COLS; ++c) {
            screen_cache[r][c] = ' ';
        }
        screen_row_len[r] = 0U;
        prev_row_len[r] = TEXT_COLS;
    }
    force_full_redraw = true;
    status_dirty = true;
}

static void make_page_swap_name(uint8_t page, char* out_name) {
    out_name[0] = 'W';
    out_name[1] = 'P';
    out_name[2] = 'G';
    out_name[3] = (char)('0' + (page / 10U));
    out_name[4] = (char)('0' + (page % 10U));
    out_name[5] = '\0';
}

static void cleanup_swap_files(void) {
    char path[24];
    char sname[8];
    for (uint8_t p = 0U; p < MAX_PAGES; ++p) {
        if (page_in_swap[p] != 0U) {
            make_page_swap_name(p, sname);
            strcpy(path, "/SYSTEM/SWAP/");
            strcat(path, sname);
            strcat(path, ".SWP");
            sd_remove(path);
            page_in_swap[p] = 0U;
        }
    }
}

static void set_status(const char* msg) {
    strncpy(status_msg, msg, sizeof(status_msg) - 1U);
    status_msg[sizeof(status_msg) - 1U] = '\0';
    status_dirty = true;
}

static bool save_page_to_storage(uint8_t page, char* buf, uint16_t len) {
    page_lens[page] = len;
    if (len == 0U) {
        page_in_swap[page] = 0U;
        page_on_disk_src[page] = 0U;
        if (ram_cache_page == page) {
            ram_cache_page = 0xFFU;
        }
        return true;
    }
    if (ram_cache_page == 0xFFU || ram_cache_page == page) {
        memcpy(cache_buf, buf, len);
        ram_cache_page = page;
        page_on_disk_src[page] = 0U;
        return true;
    }
    char sname[8];
    make_page_swap_name(page, sname);
    if (os_swap_save(sname, buf, len) == 0U) {
        page_in_swap[page] = 1U;
        page_on_disk_src[page] = 0U;
        return true;
    }
    return false;
}

static bool load_page_from_storage(uint8_t page, char* buf, uint16_t* out_len) {
    uint16_t len = page_lens[page];
    *out_len = len;
    if (len == 0U) {
        return true;
    }
    if (ram_cache_page == page) {
        memcpy(buf, cache_buf, len);
        return true;
    }
    if (page_in_swap[page] != 0U) {
        char sname[8];
        make_page_swap_name(page, sname);
        return os_swap_load(sname, buf, len) == 0U;
    }
    if (page_on_disk_src[page] != 0U && has_source_file) {
        SD_FILE fp;
        if (sd_open(&fp, current_file, SD_READ) != 0U) {
            sd_seek(&fp, page_file_offsets[page]);
            int16_t r = sd_read(&fp, buf, len);
            sd_close(&fp);
            if (r > 0) {
                *out_len = (uint16_t)r;
                return true;
            }
        }
        return false;
    }
    return true;
}

static bool switch_to_page(uint8_t target_page) {
    if (target_page >= total_pages) {
        return false;
    }
    if (target_page == cur_page) {
        return true;
    }
    if (page_dirty || (page_in_swap[cur_page] == 0U && page_on_disk_src[cur_page] == 0U &&
                       ram_cache_page != cur_page)) {
        if (!save_page_to_storage(cur_page, page_buf, cur_len)) {
            set_status("Err: Swap save failed");
            return false;
        }
        page_dirty = false;
    }
    uint16_t new_len = 0U;
    if (!load_page_from_storage(target_page, page_buf, &new_len)) {
        set_status("Err: Swap/stream load failed");
        return false;
    }
    cur_page = target_page;
    cur_len = new_len;
    if (cursor_idx > cur_len) {
        cursor_idx = cur_len;
    }
    scroll_row = 0U;
    set_status(page_in_swap[cur_page] != 0U ? "Loaded from SWAP" : "Loaded chunk");
    return true;
}

static bool split_current_page(void) {
    if (total_pages >= MAX_PAGES) {
        set_status("Err: Max pages reached");
        return false;
    }

    uint16_t split_at = SPLIT_KEEP_SIZE;
    for (uint16_t i = SPLIT_KEEP_SIZE; i > 256U; --i) {
        if (page_buf[i - 1U] == '\n') {
            split_at = i;
            break;
        }
    }

    uint16_t tail_len = (uint16_t)(cur_len - split_at);

    for (uint8_t p = total_pages; p > (uint8_t)(cur_page + 1U); --p) {
        uint8_t prev = (uint8_t)(p - 1U);
        page_lens[p] = page_lens[prev];
        page_file_offsets[p] = page_file_offsets[prev];
        page_on_disk_src[p] = page_on_disk_src[prev];
        if (ram_cache_page == prev) {
            ram_cache_page = p;
        }
        page_in_swap[p] = page_in_swap[prev];
    }

    uint8_t next_p = (uint8_t)(cur_page + 1U);
    if (next_p < total_pages) {
        save_page_to_storage(cur_page, page_buf, split_at);
        memmove(page_buf, &page_buf[split_at], tail_len);
        char tmp_name[8];
        make_page_swap_name(total_pages, tmp_name);
        os_swap_save(tmp_name, page_buf, tail_len);

        for (uint8_t p = total_pages; p > next_p; --p) {
            uint8_t src_p = (uint8_t)(p - 1U);
            if (page_in_swap[src_p] != 0U) {
                char s_src[8];
                char s_dst[8];
                make_page_swap_name(src_p, s_src);
                make_page_swap_name(p, s_dst);
                os_swap_load(s_src, page_buf, page_lens[p]);
                os_swap_save(s_dst, page_buf, page_lens[p]);
            }
        }
        char s_next[8];
        make_page_swap_name(next_p, s_next);
        os_swap_load(tmp_name, page_buf, tail_len);
        os_swap_save(s_next, page_buf, tail_len);
        page_lens[next_p] = tail_len;
        page_in_swap[next_p] = 1U;
        page_on_disk_src[next_p] = 0U;
        ++total_pages;

        load_page_from_storage(cur_page, page_buf, &cur_len);
    } else {
        ++total_pages;
        save_page_to_storage(next_p, &page_buf[split_at], tail_len);
        cur_len = split_at;
        page_lens[cur_page] = cur_len;
        page_dirty = true;
    }

    if (cursor_idx >= split_at) {
        cursor_idx = (uint16_t)(cursor_idx - split_at);
        switch_to_page(next_p);
    }
    set_status("Split chunk to SWAP");
    return true;
}

static void reset_document(void) {
    cleanup_swap_files();
    for (uint8_t i = 0U; i < MAX_PAGES; ++i) {
        page_lens[i] = 0U;
        page_file_offsets[i] = 0U;
        page_in_swap[i] = 0U;
        page_on_disk_src[i] = 0U;
    }
    ram_cache_page = 0xFFU;
    total_pages = 1U;
    cur_page = 0U;
    cur_len = 0U;
    cursor_idx = 0U;
    scroll_row = 0U;
    page_dirty = false;
    doc_modified = false;
    has_source_file = false;
    status_dirty = true;
}

static bool load_file_streaming(const char* filename) {
    SD_FILE fp;
    if (sd_open(&fp, filename, SD_READ) == 0U) {
        set_status("New file");
        strncpy(current_file, filename, FILENAME_MAX_LEN - 1U);
        current_file[FILENAME_MAX_LEN - 1U] = '\0';
        reset_document();
        return false;
    }

    reset_document();
    strncpy(current_file, filename, FILENAME_MAX_LEN - 1U);
    current_file[FILENAME_MAX_LEN - 1U] = '\0';
    has_source_file = true;

    uint32_t offset = 0U;
    uint8_t p = 0U;
    while (p < MAX_PAGES) {
        sd_seek(&fp, offset);
        int16_t n = sd_read(&fp, page_buf, LOAD_CHUNK_SIZE);
        if (n <= 0) {
            break;
        }
        uint16_t chunk_len = (uint16_t)n;
        if (n == (int16_t)LOAD_CHUNK_SIZE) {
            for (uint16_t i = chunk_len; i > (LOAD_CHUNK_SIZE - 128U); --i) {
                if (page_buf[i - 1U] == '\n') {
                    chunk_len = i;
                    break;
                }
            }
        }
        page_file_offsets[p] = offset;
        page_lens[p] = chunk_len;
        page_on_disk_src[p] = 1U;
        offset += chunk_len;
        ++p;
        if (n < (int16_t)LOAD_CHUNK_SIZE) {
            break;
        }
    }
    sd_close(&fp);

    if (p == 0U) {
        total_pages = 1U;
        cur_page = 0U;
        cur_len = 0U;
    } else {
        total_pages = p;
        cur_page = 0U;
        load_page_from_storage(0U, page_buf, &cur_len);
    }

    cursor_idx = 0U;
    scroll_row = 0U;
    page_dirty = false;
    doc_modified = false;
    set_status(total_pages > 1U ? "Streamed chunks ready" : "File loaded");
    return true;
}

static void ensure_parent_dirs(const char* path) {
    char dir[FILENAME_MAX_LEN];
    uint8_t i = 0U;
    while (path[i] != '\0' && i < (FILENAME_MAX_LEN - 1U)) {
        dir[i] = path[i];
        if (i > 0U && (path[i] == '/' || path[i] == '\\') && path[i - 1U] != '/' && path[i - 1U] != '\\') {
            dir[i] = '\0';
            sd_mkdir(dir);
            dir[i] = path[i];
        }
        ++i;
    }
}

static bool save_file_streaming(const char* filename) {
    if (page_dirty || (total_pages > 1U && page_in_swap[cur_page] == 0U && ram_cache_page != cur_page)) {
        if (!save_page_to_storage(cur_page, page_buf, cur_len)) {
            set_status("Err: Swap full on save");
            return false;
        }
        page_dirty = false;
    } else {
        page_lens[cur_page] = cur_len;
    }

    if (has_source_file && strcmp(filename, current_file) == 0) {
        for (uint8_t p = 0U; p < total_pages; ++p) {
            if (p != cur_page && page_on_disk_src[p] != 0U && page_in_swap[p] == 0U && ram_cache_page != p) {
                uint16_t l = 0U;
                if (load_page_from_storage(p, page_buf, &l)) {
                    save_page_to_storage(p, page_buf, l);
                }
            }
        }
    }

    ensure_parent_dirs(filename);

    SD_FILE fp;
    if (sd_open(&fp, filename, SD_WRITE | SD_CREATE_ALWAYS) == 0U) {
        load_page_from_storage(cur_page, page_buf, &cur_len);
        set_status("Err: Cannot write file");
        return false;
    }

    uint32_t new_offset = 0U;
    for (uint8_t p = 0U; p < total_pages; ++p) {
        uint16_t l = page_lens[p];
        if (l == 0U) {
            continue;
        }
        if (total_pages == 1U && p == cur_page) {
            sd_write(&fp, page_buf, cur_len);
        } else {
            load_page_from_storage(p, page_buf, &l);
            sd_write(&fp, page_buf, l);
        }
        page_file_offsets[p] = new_offset;
        new_offset += l;
    }
    sd_close(&fp);

    strncpy(current_file, filename, FILENAME_MAX_LEN - 1U);
    current_file[FILENAME_MAX_LEN - 1U] = '\0';
    has_source_file = true;

    load_page_from_storage(cur_page, page_buf, &cur_len);
    doc_modified = false;
    set_status("Saved OK");
    return true;
}

static void compute_cursor_row_col(uint16_t* out_row, uint16_t* out_col) {
    uint16_t r = 0U;
    uint16_t c = 0U;
    for (uint16_t i = 0U; i < cursor_idx && i < cur_len; ++i) {
        if (page_buf[i] == '\n') {
            ++r;
            c = 0U;
        } else {
            ++c;
            if (c >= TEXT_COLS) {
                ++r;
                c = 0U;
            }
        }
    }
    *out_row = r;
    *out_col = c;
}

static void adjust_scroll(uint16_t cur_r) {
    if (cur_r < scroll_row) {
        scroll_row = cur_r;
    } else if (cur_r >= (uint16_t)(scroll_row + TEXT_ROWS)) {
        scroll_row = (uint16_t)(cur_r - TEXT_ROWS + 1U);
    }
}

static void draw_chrome(void) {
    print_str("\x1b[?25l");
    tui_set_color(TUI_COLOR_WHITE, TUI_COLOR_BLUE);
    tui_fill_rect(1U, 1U, 80U, 24U, ' ');

    tui_set_cursor(1U, 1U);
    print_str("\x1b[30;46m MicroWord   [ESC: Menu]  [^O: Open]  [^S: Save]  [^N/^P: Page]                 \x1b[0m");

    tui_draw_window(WIN_ROW, WIN_COL, WIN_WIDTH, WIN_HEIGHT, current_file);

    tui_set_cursor(WIN_ROW + 1U, TEXT_COL);
    print_str("\x1b[34;47m[----1----2----3----4----5----6----7-]\x1b[0m");

    invalidate_screen();
    print_str("\x1b[?25h");
}

static void draw_status_bar(void) {
    print_str("\x1b[?25l");
    tui_set_cursor(25U, 1U);
    print_str("\x1b[30;47m ");
    print_str(current_file);
    bios_putchar(doc_modified ? '*' : ' ');
    print_str(" | Pg:");
    print_num(cur_page + 1U);
    bios_putchar('/');
    print_num(total_pages);
    print_str(" | ");
    print_num(cur_len);
    print_str("B | ");
    uint8_t msg_len = 0U;
    while (status_msg[msg_len] != '\0' && msg_len < 34U) {
        bios_putchar(status_msg[msg_len]);
        ++msg_len;
    }
    while (msg_len < 34U) {
        bios_putchar(' ');
        ++msg_len;
    }
    print_str("\x1b[0m\x1b[?25h");
    status_dirty = false;
}

static void render_editor(void) {
    uint16_t cur_r = 0U;
    uint16_t cur_c = 0U;
    compute_cursor_row_col(&cur_r, &cur_c);
    adjust_scroll(cur_r);

    if (scroll_row != prev_scroll_row) {
        force_full_redraw = true;
        prev_scroll_row = scroll_row;
    }

    uint16_t doc_r = 0U;
    uint16_t doc_c = 0U;
    uint16_t i = 0U;

    while (i < cur_len && doc_r < scroll_row) {
        if (page_buf[i] == '\n') {
            ++doc_r;
            doc_c = 0U;
        } else {
            ++doc_c;
            if (doc_c >= TEXT_COLS) {
                ++doc_r;
                doc_c = 0U;
            }
        }
        ++i;
    }

    bool color_set = false;
    for (uint8_t vis_r = 0U; vis_r < TEXT_ROWS; ++vis_r) {
        char row_text[TEXT_COLS];
        uint8_t col_len = 0U;

        while (col_len < TEXT_COLS && i < cur_len) {
            char ch = page_buf[i];
            ++i;
            if (ch == '\n') {
                break;
            }
            row_text[col_len] = (ch >= 32 && ch <= 126) ? ch : ' ';
            ++col_len;
        }
        for (uint8_t c = col_len; c < TEXT_COLS; ++c) {
            row_text[c] = ' ';
        }

        uint8_t max_check = (col_len > prev_row_len[vis_r]) ? col_len : prev_row_len[vis_r];
        if (force_full_redraw && max_check < prev_row_len[vis_r]) {
            max_check = prev_row_len[vis_r];
        }

        uint8_t first_diff = 0xFFU;
        uint8_t last_diff = 0U;
        for (uint8_t c = 0U; c < max_check; ++c) {
            if (force_full_redraw || screen_cache[vis_r][c] != row_text[c]) {
                if (first_diff == 0xFFU) {
                    first_diff = c;
                }
                last_diff = c;
            }
            screen_cache[vis_r][c] = row_text[c];
        }
        prev_row_len[vis_r] = col_len;
        screen_row_len[vis_r] = col_len;

        if (first_diff != 0xFFU) {
            if (!color_set) {
                print_str("\x1b[?25l");
                tui_set_color(TUI_COLOR_BLACK, TUI_COLOR_WHITE);
                color_set = true;
            }
            tui_set_cursor((uint8_t)(TEXT_ROW + vis_r), (uint8_t)(TEXT_COL + first_diff));
            for (uint8_t c = first_diff; c <= last_diff; ++c) {
                bios_putchar(row_text[c]);
            }
        }
    }

    force_full_redraw = false;

    if (status_dirty) {
        draw_status_bar();
    } else if (color_set) {
        print_str("\x1b[?25h");
    }

    uint8_t screen_cur_r = (uint8_t)(TEXT_ROW + (cur_r - scroll_row));
    uint8_t screen_cur_c = (uint8_t)(TEXT_COL + cur_c);
    tui_set_cursor(screen_cur_r, screen_cur_c);
}

static void insert_char(char c) {
    if (cur_len >= (PAGE_CAP - 1U)) {
        if (!split_current_page()) {
            return;
        }
    }


    if (cursor_idx < cur_len) {
        memmove(&page_buf[cursor_idx + 1U], &page_buf[cursor_idx], (size_t)(cur_len - cursor_idx));
    }
    page_buf[cursor_idx] = c;
    ++cur_len;
    ++cursor_idx;
    page_lens[cur_page] = cur_len;
    page_dirty = true;
    if (!doc_modified) {
        doc_modified = true;
        status_dirty = true;
    }
}

static void delete_char_before(void) {
    if (cursor_idx > 0U) {
        if (cursor_idx < cur_len) {
            memmove(&page_buf[cursor_idx - 1U], &page_buf[cursor_idx], (size_t)(cur_len - cursor_idx));
        }
        --cur_len;
        --cursor_idx;
        page_lens[cur_page] = cur_len;
        page_dirty = true;
        if (!doc_modified) {
            doc_modified = true;
            status_dirty = true;
        }
    } else if (cur_page > 0U) {
        if (switch_to_page((uint8_t)(cur_page - 1U))) {
            cursor_idx = cur_len;
        }
    }
}

static void delete_char_at(void) {
    if (cursor_idx < cur_len) {
        if ((cursor_idx + 1U) < cur_len) {
            memmove(&page_buf[cursor_idx], &page_buf[cursor_idx + 1U], (size_t)(cur_len - cursor_idx - 1U));
        }
        --cur_len;
        page_lens[cur_page] = cur_len;
        page_dirty = true;
        if (!doc_modified) {
            doc_modified = true;
            status_dirty = true;
        }
    }
}

static void move_cursor_vertical(int8_t dir) {
    uint16_t cur_r = 0U;
    uint16_t cur_c = 0U;
    compute_cursor_row_col(&cur_r, &cur_c);

    if (dir < 0) {
        if (cur_r == 0U) {
            if (cur_page > 0U && switch_to_page((uint8_t)(cur_page - 1U))) {
                cursor_idx = cur_len;
            } else {
                cursor_idx = 0U;
            }
            return;
        }
        uint16_t target_r = (uint16_t)(cur_r - 1U);
        uint16_t r = 0U;
        uint16_t c = 0U;
        uint16_t idx = 0U;
        while (idx < cur_len && r < target_r) {
            if (page_buf[idx] == '\n') {
                ++r;
                c = 0U;
            } else {
                ++c;
                if (c >= TEXT_COLS) {
                    ++r;
                    c = 0U;
                }
            }
            ++idx;
        }
        while (idx < cur_len && c < cur_c && page_buf[idx] != '\n') {
            ++idx;
            ++c;
        }
        cursor_idx = idx;
    } else {
        uint16_t target_r = (uint16_t)(cur_r + 1U);
        uint16_t r = 0U;
        uint16_t c = 0U;
        uint16_t idx = 0U;
        while (idx < cur_len && r < target_r) {
            if (page_buf[idx] == '\n') {
                ++r;
                c = 0U;
            } else {
                ++c;
                if (c >= TEXT_COLS) {
                    ++r;
                    c = 0U;
                }
            }
            ++idx;
        }
        if (r < target_r) {
            if ((uint8_t)(cur_page + 1U) < total_pages && switch_to_page((uint8_t)(cur_page + 1U))) {
                cursor_idx = 0U;
            } else {
                cursor_idx = cur_len;
            }
            return;
        }
        while (idx < cur_len && c < cur_c && page_buf[idx] != '\n') {
            ++idx;
            ++c;
        }
        cursor_idx = idx;
    }
}

static bool prompt_filename_dialog(const char* title, char* out_buf) {
    const uint8_t d_row = 7U;
    const uint8_t d_col = 18U;
    const uint8_t d_width = 44U;
    const uint8_t d_height = 10U;

    char input[FILENAME_MAX_LEN];
    strncpy(input, out_buf, FILENAME_MAX_LEN - 1U);
    input[FILENAME_MAX_LEN - 1U] = '\0';
    uint8_t len = (uint8_t)strlen(input);

    print_str("\x1b[?25l");
    tui_draw_window(d_row, d_col, d_width, d_height, title);
    tui_set_color(TUI_COLOR_BLACK, TUI_COLOR_WHITE);
    tui_set_cursor(d_row + 2U, d_col + 3U);
    print_str("File path:");

    tui_set_cursor(d_row + 6U, d_col);
    print_str("\x1b[30;47m\x1b(0t");
    for (uint8_t i = 0U; i < d_width - 2U; ++i) {
        bios_putchar('q');
    }
    bios_putchar('u');
    print_str("\x1b(B");

    uint8_t focus = 0U;
    uint8_t prev_focus = 0xFFU;
    uint8_t prev_len = 0xFFU;

    while (true) {
        if (focus != prev_focus || len != prev_len) {
            tui_set_cursor(d_row + 4U, d_col + 3U);
            print_str(focus == 0U ? "\x1b[37;44;1m" : "\x1b[30;46m");
            for (uint8_t i = 0U; i < 36U; ++i) {
                if (i < len) {
                    bios_putchar(input[i]);
                } else if (i == len && focus == 0U) {
                    bios_putchar('_');
                } else {
                    bios_putchar(' ');
                }
            }
            print_str("\x1b[0m");
            prev_len = len;
        }

        if (focus != prev_focus) {
            tui_draw_button(d_row + 7U, d_col + 8U, "OK", focus == 1U);
            tui_draw_button(d_row + 7U, d_col + 22U, "Cancel", focus == 2U);
            prev_focus = focus;
        }
        tui_set_cursor(d_row + 4U, (uint8_t)(d_col + 3U + len));

        uint16_t key = tui_get_key();
        if (key == TUI_KEY_ESC) {
            draw_chrome();
            return false;
        }
        if (key == TUI_KEY_TAB || key == TUI_KEY_DOWN || (focus > 0U && key == TUI_KEY_RIGHT)) {
            focus = (uint8_t)((focus + 1U) % 3U);
        } else if (key == TUI_KEY_SHIFT_TAB || key == TUI_KEY_UP || (focus > 0U && key == TUI_KEY_LEFT)) {
            focus = (uint8_t)((focus + 2U) % 3U);
        } else if (key == TUI_KEY_ENTER) {
            if (focus == 2U) {
                draw_chrome();
                return false;
            }
            if (len > 0U) {
                strcpy(out_buf, input);
                draw_chrome();
                return true;
            }
        } else if (focus == 0U) {
            if (key == KEY_BACKSPACE || key == KEY_DEL_ASCII || key == TUI_KEY_DELETE) {
                if (len > 0U) {
                    --len;
                    input[len] = '\0';
                }
            } else if (key >= 32U && key <= 126U && len < (FILENAME_MAX_LEN - 1U)) {
                input[len] = (char)key;
                ++len;
                input[len] = '\0';
            }
        }
    }
}

static bool show_main_menu_dialog(void) {
    const uint8_t w_row = 5U;
    const uint8_t w_col = 14U;
    const uint8_t w_width = 52U;
    const uint8_t w_height = 14U;

    print_str("\x1b[?25l");
    tui_draw_window(w_row, w_col, w_width, w_height, "MicroWord Menu");
    tui_set_color(TUI_COLOR_BLACK, TUI_COLOR_WHITE);
    tui_set_cursor(w_row + 2U, w_col + 3U);
    print_str("Document: ");
    print_str(current_file);

    tui_set_cursor(w_row + 4U, w_col + 3U);
    print_str("Select an action:");

    tui_set_cursor(w_row + 7U, w_col);
    print_str("\x1b[30;47m\x1b(0t");
    for (uint8_t i = 0U; i < w_width - 2U; ++i) {
        bios_putchar('q');
    }
    bios_putchar('u');
    print_str("\x1b(B");

    uint8_t selected = 0U;
    uint8_t prev_selected = 0xFFU;
    while (true) {
        if (selected != prev_selected) {
            tui_draw_button(w_row + 9U, w_col + 6U, "Resume", selected == 0U);
            tui_draw_button(w_row + 9U, w_col + 22U, "Save", selected == 1U);
            tui_draw_button(w_row + 9U, w_col + 36U, "Open", selected == 2U);
            tui_draw_button(w_row + 11U, w_col + 13U, "New", selected == 3U);
            tui_draw_button(w_row + 11U, w_col + 31U, "Exit", selected == 4U);
            prev_selected = selected;
        }
        tui_set_cursor(w_row + 2U, w_col + 2U);

        uint16_t key = tui_get_key();
        switch (key) {
            case TUI_KEY_RIGHT:
            case TUI_KEY_TAB:
                selected = (uint8_t)((selected + 1U) % 5U);
                break;
            case TUI_KEY_LEFT:
            case TUI_KEY_SHIFT_TAB:
                selected = (uint8_t)((selected + 4U) % 5U);
                break;
            case TUI_KEY_DOWN:
                if (selected < 3U) {
                    selected = (selected == 2U) ? 4U : (uint8_t)(selected + 3U);
                }
                break;
            case TUI_KEY_UP:
                if (selected >= 3U) {
                    selected = (uint8_t)(selected - 3U);
                }
                break;
            case TUI_KEY_ESC:
                draw_chrome();
                return true;
            case TUI_KEY_ENTER:
            case ' ': {
                if (selected == 0U) {
                    draw_chrome();
                    return true;
                }
                if (selected == 1U) {
                    char fname[FILENAME_MAX_LEN];
                    strcpy(fname, current_file);
                    if (prompt_filename_dialog("Save File", fname)) {
                        save_file_streaming(fname);
                        draw_chrome();
                    }
                    return true;
                }
                if (selected == 2U) {
                    char fname[FILENAME_MAX_LEN];
                    strcpy(fname, current_file);
                    if (prompt_filename_dialog("Open File (Streaming)", fname)) {
                        load_file_streaming(fname);
                        draw_chrome();
                    }
                    return true;
                }
                if (selected == 3U) {
                    strcpy(current_file, "UNTITLED.TXT");
                    reset_document();
                    set_status("New document");
                    draw_chrome();
                    return true;
                }
                return false;
            }
            default:
                break;
        }
    }
}

int main(void) {
    sd_mkdir("/SYSTEM");
    sd_mkdir("/SYSTEM/SWAP");

    reset_document();
    if (arg_count >= 2U && _args_ptr[1] != NULL && _args_ptr[1][0] != '\0') {
        load_file_streaming(_args_ptr[1]);
    }

    tui_init();
    draw_chrome();

    bool running = true;
    while (running) {
        render_editor();

        uint16_t key = tui_get_key();
        switch (key) {
            case TUI_KEY_ESC:
                running = show_main_menu_dialog();
                break;
            case CTRL_Q:
                running = false;
                break;
            case CTRL_S: {
                char fname[FILENAME_MAX_LEN];
                strcpy(fname, current_file);
                if (prompt_filename_dialog("Save File", fname)) {
                    save_file_streaming(fname);
                    draw_chrome();
                }
                break;
            }
            case CTRL_O: {
                char fname[FILENAME_MAX_LEN];
                strcpy(fname, current_file);
                if (prompt_filename_dialog("Open File (Streaming)", fname)) {
                    load_file_streaming(fname);
                    draw_chrome();
                }
                break;
            }
            case CTRL_N:
                if ((uint8_t)(cur_page + 1U) < total_pages) {
                    if (switch_to_page((uint8_t)(cur_page + 1U))) {
                        cursor_idx = 0U;
                    }
                } else {
                    set_status("Already at last chunk");
                }
                break;
            case CTRL_P:
                if (cur_page > 0U) {
                    if (switch_to_page((uint8_t)(cur_page - 1U))) {
                        cursor_idx = 0U;
                    }
                } else {
                    set_status("Already at first chunk");
                }
                break;
            case TUI_KEY_LEFT:
                if (cursor_idx > 0U) {
                    --cursor_idx;
                } else if (cur_page > 0U && switch_to_page((uint8_t)(cur_page - 1U))) {
                    cursor_idx = cur_len;
                }
                break;
            case TUI_KEY_RIGHT:
                if (cursor_idx < cur_len) {
                    ++cursor_idx;
                } else if ((uint8_t)(cur_page + 1U) < total_pages && switch_to_page((uint8_t)(cur_page + 1U))) {
                    cursor_idx = 0U;
                }
                break;
            case TUI_KEY_UP:
                move_cursor_vertical(-1);
                break;
            case TUI_KEY_DOWN:
                move_cursor_vertical(1);
                break;
            case TUI_KEY_HOME:
                while (cursor_idx > 0U && page_buf[cursor_idx - 1U] != '\n') {
                    --cursor_idx;
                }
                break;
            case TUI_KEY_END:
                while (cursor_idx < cur_len && page_buf[cursor_idx] != '\n') {
                    ++cursor_idx;
                }
                break;
            case KEY_BACKSPACE:
            case KEY_DEL_ASCII:
                delete_char_before();
                break;
            case TUI_KEY_DELETE:
                delete_char_at();
                break;
            case TUI_KEY_ENTER:
                insert_char('\n');
                break;
            case TUI_KEY_TAB:
                insert_char(' ');
                insert_char(' ');
                break;
            default:
                if (key >= 32U && key <= 126U) {
                    insert_char((char)key);
                }
                break;
        }
    }

    cleanup_swap_files();
    tui_shutdown();
    return 0;
}
