#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "../Libs/app-bios.h"

MICRODOS_NO_DESKTOP();

static void format_u32_right(char* buf, uint32_t val, uint8_t width) {
    buf[width] = '\0';
    for (int i = (int)width - 1; i >= 0; i--) {
        buf[i] = (val > 0 || i == (int)width - 1) ? (char)((val % 10) + '0') : ' ';
        val /= 10;
    }
}

static void print_column(uint32_t val) {
    char buf[9];
    format_u32_right(buf, val, 8);
    print_str(buf);
}

static void print_counts(uint32_t l, uint32_t w, uint32_t b, bool s_l, bool s_w, bool s_b, const char* name) {
    if (s_l) print_column(l);
    if (s_w) print_column(w);
    if (s_b) print_column(b);
    if (name) {
        bios_putchar(' ');
        print_str(name);
    }
    println("");
}

int main(void) {
    char** args = _args_ptr;
    bool show_lines = false;
    bool show_words = false;
    bool show_bytes = false;
    uint8_t file_indices[16];
    uint8_t num_files = 0;

    for (uint8_t i = 1; i < arg_count; i++) {
        const char* a = args[i];
        if (a[0] == '-' && a[1] != '\0') {
            for (uint8_t j = 1; a[j] != '\0'; j++) {
                if (a[j] == 'l') show_lines = true;
                else if (a[j] == 'w') show_words = true;
                else if (a[j] == 'c') show_bytes = true;
            }
        } else {
            if (num_files < 16) {
                file_indices[num_files++] = i;
            }
        }
    }

    if (!show_lines && !show_words && !show_bytes) {
        show_lines = true;
        show_words = true;
        show_bytes = true;
    }

    if (num_files == 0) {
        println("Usage: wc [-l] [-w] [-c] <file | ->");
        return 1;
    }

    uint32_t tot_lines = 0, tot_words = 0, tot_bytes = 0;
    uint8_t has_error = 0;

    for (uint8_t f = 0; f < num_files; f++) {
        const char* fname = args[file_indices[f]];
        uint32_t lines = 0, words = 0, bytes = 0;
        bool in_word = false;

        if (strcmp(fname, "-") == 0) {
            while (1) {
                int c = bios_getchar();
                if (c == 0 || c == 4 || c == 26 || c == -1 || c == 3 || c == 27) break;
                bytes++;
                if (c == '\n') lines++;
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') in_word = false;
                else if (!in_word) { in_word = true; words++; }
            }
            print_counts(lines, words, bytes, show_lines, show_words, show_bytes, nullptr);
        } else {
            SD_FILE fp;
            if (!sd_open(&fp, fname, SD_READ)) {
                print_str("wc: ");
                print_str(fname);
                println(": No such file");
                has_error = 1;
                continue;
            }
            int16_t c;
            while ((c = sd_getc(&fp)) >= 0) {
                bytes++;
                if (c == '\n') lines++;
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') in_word = false;
                else if (!in_word) { in_word = true; words++; }
            }
            sd_close(&fp);
            print_counts(lines, words, bytes, show_lines, show_words, show_bytes, fname);
        }

        tot_lines += lines;
        tot_words += words;
        tot_bytes += bytes;
    }

    if (num_files > 1) {
        print_counts(tot_lines, tot_words, tot_bytes, show_lines, show_words, show_bytes, "total");
    }

    return has_error ? 1 : 0;
}
