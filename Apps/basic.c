#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "Libs/app-bios.h"

#define PROG_BUF_SIZE 3072
#define MAX_ARRAYS    8
#define GOSUB_DEPTH   16
#define FOR_DEPTH     16
#define STR_LEN       32

typedef enum {
    VAL_NUM,
    VAL_STR
} ValType;

typedef struct {
    ValType type;
    float num;
    char str[STR_LEN];
} Value;

typedef struct {
    char base;
    char ext;
    bool is_str;
} VarRef;

struct Array {
    char name[4];
    bool is_str;
    uint8_t dim;
    float* num_data;
    char (*str_data)[STR_LEN];
};

struct GosubFrame {
    uint16_t line_num;
    const char* next_stmt;
};

struct ForFrame {
    VarRef var;
    float end_val;
    float step_val;
    uint16_t line_num;
    const char* loop_stmt;
};

static uint8_t prog_buf[PROG_BUF_SIZE];

static float vars[26];
static float vars_ext[26][10];
static char str_vars[26][STR_LEN];
static char* str_vars_ext[26][10];
static struct Array arrays[MAX_ARRAYS];

static struct GosubFrame gosub_stack[GOSUB_DEPTH];
static uint8_t gosub_sp;

static struct ForFrame for_stack[FOR_DEPTH];
static uint8_t for_sp;

static uint8_t* data_line_ptr;
static const char* data_item_ptr;

static bool program_running;
static bool interrupted;
static bool runtime_error;
static bool line_jumped;
static bool is_cont;

static uint16_t current_line_num;
static const char* current_stmt_ptr;
static uint8_t* current_line_ptr;

static uint16_t break_line_num;
static const char* break_stmt_ptr;
static uint8_t* break_line_ptr;

static uint8_t current_col;
static float last_rnd = 0.5f;

static void clear_vars(void);
static void execute_statement(const char** p);
static Value eval_expr(const char** p);
static void run_program_from(uint8_t* start_line);

static void out_char(char c) {
    if (c == '\r' || c == '\n') {
        current_col = 0;
    } else if (c == '\b') {
        if (current_col > 0) {
            current_col--;
        }
    } else {
        current_col++;
    }
    bios_putchar(c);
}

static void out_str(const char* s) {
    while (*s) {
        out_char(*s++);
    }
}

static void out_float(float f) {
    if (isnan(f)) {
        out_str("NAN");
        return;
    }
    if (isinf(f)) {
        if (f < 0.0f) {
            out_char('-');
        }
        out_str("INF");
        return;
    }
    if (f < 0.0f) {
        out_char('-');
        f = -f;
    }
    long int_part = (long)f;
    float frac = f - (float)int_part;
    print_num((unsigned int)int_part);
    if (frac > 0.00001f) {
        out_char('.');
        for (int i = 0; i < 5; i++) {
            frac *= 10.0f;
            int d = (int)frac;
            out_char((char)('0' + d));
            frac -= (float)d;
            if (frac < 0.00001f) {
                break;
            }
        }
    }
}

static void print_basic_number(float f) {
    if (f >= 0.0f) {
        out_char(' ');
    }
    out_float(f);
    out_char(' ');
}

static void float_to_str(float f, char* buf, size_t max) {
    if (isnan(f)) {
        strncpy(buf, "NAN", max);
        return;
    }
    if (isinf(f)) {
        strncpy(buf, (f < 0.0f) ? "-INF" : " INF", max);
        return;
    }
    long n = (long)f;
    if ((float)n == f) {
        if (f >= 0.0f) {
            snprintf(buf, max, " %ld", n);
        } else {
            snprintf(buf, max, "%ld", n);
        }
    } else {
        if (f >= 0.0f) {
            snprintf(buf, max, " %g", (double)f);
        } else {
            snprintf(buf, max, "%g", (double)f);
        }
    }
}

static void skip_spaces(const char** p) {
    while (**p == ' ' || **p == '\t') {
        (*p)++;
    }
}

static bool match_kw(const char** p, const char* kw) {
    skip_spaces(p);
    const char* s = *p;
    while (*kw) {
        if (toupper((unsigned char)*s) != toupper((unsigned char)*kw)) {
            return false;
        }
        s++;
        kw++;
    }
    char last = *(kw - 1);
    if (isalnum((unsigned char)last) && isalnum((unsigned char)*s)) {
        return false;
    }
    *p = s;
    return true;
}

static uint8_t* prog_end(void) {
    uint8_t* p = prog_buf;
    while (p < prog_buf + PROG_BUF_SIZE - 2) {
        uint16_t cur = (uint16_t)(p[0] | (p[1] << 8));
        if (cur == 0xFFFF) {
            return p;
        }
        p += 4 + p[2];
    }
    return p;
}

static uint8_t* find_line(uint16_t line_num) {
    uint8_t* p = prog_buf;
    while (p < prog_buf + PROG_BUF_SIZE - 2) {
        uint16_t cur = (uint16_t)(p[0] | (p[1] << 8));
        if (cur == 0xFFFF || cur >= line_num) {
            return p;
        }
        p += 4 + p[2];
    }
    return p;
}

static bool delete_line(uint16_t line_num) {
    uint8_t* p = find_line(line_num);
    if (!p) {
        return false;
    }
    uint16_t cur = (uint16_t)(p[0] | (p[1] << 8));
    if (cur != line_num) {
        return false;
    }
    uint8_t len = p[2];
    uint16_t entry_sz = (uint16_t)(4 + len);
    uint8_t* end = prog_end();
    memmove(p, p + entry_sz, (size_t)(end + 2 - (p + entry_sz)));
    return true;
}

static bool insert_line(uint16_t line_num, const char* text) {
    while (*text == ' ' || *text == '\t') {
        text++;
    }
    delete_line(line_num);
    if (*text == '\0') {
        return true;
    }

    size_t text_len = strlen(text);
    if (text_len > 250) {
        text_len = 250;
    }
    uint16_t new_sz = (uint16_t)(4 + text_len);
    uint8_t* end = prog_end();
    size_t cur_used = (size_t)(end + 2 - prog_buf);

    if (cur_used + new_sz > PROG_BUF_SIZE) {
        println("?OUT OF MEMORY");
        return false;
    }

    uint8_t* p = find_line(line_num);
    memmove(p + new_sz, p, (size_t)(end + 2 - p));
    p[0] = (uint8_t)(line_num & 0xFF);
    p[1] = (uint8_t)(line_num >> 8);
    p[2] = (uint8_t)text_len;
    memcpy(p + 3, text, text_len);
    p[3 + text_len] = '\0';
    return true;
}

static void clear_program(void) {
    prog_buf[0] = 0xFF;
    prog_buf[1] = 0xFF;
    clear_vars();
    break_line_num = 0;
    break_stmt_ptr = NULL;
    break_line_ptr = NULL;
}

static void clear_vars(void) {
    memset(vars, 0, sizeof(vars));
    memset(vars_ext, 0, sizeof(vars_ext));
    memset(str_vars, 0, sizeof(str_vars));
    for (int i = 0; i < 26; i++) {
        for (int j = 0; j < 10; j++) {
            if (str_vars_ext[i][j]) {
                os_free(str_vars_ext[i][j]);
                str_vars_ext[i][j] = NULL;
            }
        }
    }
    for (int i = 0; i < MAX_ARRAYS; i++) {
        if (arrays[i].name[0]) {
            if (arrays[i].num_data) {
                os_free(arrays[i].num_data);
                arrays[i].num_data = NULL;
            }
            if (arrays[i].str_data) {
                os_free(arrays[i].str_data);
                arrays[i].str_data = NULL;
            }
            arrays[i].name[0] = '\0';
            arrays[i].dim = 0;
            arrays[i].is_str = false;
        }
    }
    gosub_sp = 0;
    for_sp = 0;
    data_line_ptr = NULL;
    data_item_ptr = NULL;
}

static bool parse_var_name(const char** p, VarRef* ref) {
    skip_spaces(p);
    char c = (char)toupper((unsigned char)**p);
    if (c < 'A' || c > 'Z') {
        return false;
    }
    (*p)++;
    ref->base = c;
    ref->ext = '\0';
    ref->is_str = false;

    if (**p >= '0' && **p <= '9') {
        ref->ext = **p;
        (*p)++;
    }
    if (**p == '$') {
        ref->is_str = true;
        (*p)++;
    }
    return true;
}

static struct Array* find_or_create_array(const VarRef* ref, uint8_t dim) {
    char name[4];
    name[0] = ref->base;
    int idx = 1;
    if (ref->ext) {
        name[idx++] = ref->ext;
    }
    if (ref->is_str) {
        name[idx++] = '$';
    }
    name[idx] = '\0';

    for (int i = 0; i < MAX_ARRAYS; i++) {
        if (strcmp(arrays[i].name, name) == 0) {
            return &arrays[i];
        }
    }
    for (int i = 0; i < MAX_ARRAYS; i++) {
        if (arrays[i].name[0] == '\0') {
            strncpy(arrays[i].name, name, 3);
            arrays[i].name[3] = '\0';
            arrays[i].is_str = ref->is_str;
            arrays[i].dim = dim;
            if (ref->is_str) {
                arrays[i].str_data = (char (*)[STR_LEN])os_alloc((uint16_t)((dim + 1) * STR_LEN));
                if (arrays[i].str_data) {
                    memset(arrays[i].str_data, 0, (size_t)((dim + 1) * STR_LEN));
                }
            } else {
                arrays[i].num_data = (float*)os_alloc((uint16_t)((dim + 1) * sizeof(float)));
                if (arrays[i].num_data) {
                    memset(arrays[i].num_data, 0, (size_t)((dim + 1) * sizeof(float)));
                }
            }
            return &arrays[i];
        }
    }
    return NULL;
}

static float get_num_var(const VarRef* ref) {
    int b = ref->base - 'A';
    if (ref->ext) {
        return vars_ext[b][ref->ext - '0'];
    }
    return vars[b];
}

static void set_num_var(const VarRef* ref, float val) {
    int b = ref->base - 'A';
    if (ref->ext) {
        vars_ext[b][ref->ext - '0'] = val;
    } else {
        vars[b] = val;
    }
}

static const char* get_str_var(const VarRef* ref) {
    int b = ref->base - 'A';
    if (ref->ext) {
        int d = ref->ext - '0';
        if (str_vars_ext[b][d]) {
            return str_vars_ext[b][d];
        }
        return "";
    }
    return str_vars[b];
}

static void set_str_var(const VarRef* ref, const char* str) {
    int b = ref->base - 'A';
    if (ref->ext) {
        int d = ref->ext - '0';
        if (!str_vars_ext[b][d]) {
            str_vars_ext[b][d] = (char*)os_alloc(STR_LEN);
        }
        if (str_vars_ext[b][d]) {
            strncpy(str_vars_ext[b][d], str, STR_LEN - 1);
            str_vars_ext[b][d][STR_LEN - 1] = '\0';
        }
    } else {
        strncpy(str_vars[b], str, STR_LEN - 1);
        str_vars[b][STR_LEN - 1] = '\0';
    }
}

static void goto_line(uint16_t target) {
    uint8_t* p = find_line(target);
    if (!p || (uint16_t)(p[0] | (p[1] << 8)) != target) {
        println("?UNDEF'D STATEMENT");
        runtime_error = true;
        return;
    }
    current_line_ptr = p;
    current_line_num = target;
    current_stmt_ptr = (const char*)(p + 3);
    line_jumped = true;
}

static void lcd_delay(void) {
    for (volatile uint8_t i = 0; i < 50; i++) {}
}

static void lcd_nibble_cmd(uint8_t nibble) {
    nibble &= 0x0F;
    VIA_ORB = nibble | 0x40;
    lcd_delay();
    VIA_ORB = nibble;
    lcd_delay();
}

static void lcd_nibble_data(uint8_t nibble) {
    nibble &= 0x0F;
    VIA_ORB = nibble | 0x10 | 0x40;
    lcd_delay();
    VIA_ORB = nibble | 0x10;
    lcd_delay();
}

static void lcd_instruction(uint8_t cmd) {
    lcd_nibble_cmd((uint8_t)(cmd >> 4));
    lcd_nibble_cmd((uint8_t)(cmd & 0x0F));
}

static void lcd_print_char(char c) {
    lcd_nibble_data((uint8_t)(((uint8_t)c) >> 4));
    lcd_nibble_data((uint8_t)(((uint8_t)c) & 0x0F));
}

static void lcd_init_hw(void) {
    VIA_DDRB = 0xFF;
    lcd_nibble_cmd(0x03);
    lcd_nibble_cmd(0x03);
    lcd_nibble_cmd(0x03);
    lcd_nibble_cmd(0x02);
    lcd_instruction(0x28);
    lcd_instruction(0x0E);
    lcd_instruction(0x06);
    lcd_instruction(0x01);
}

static Value eval_factor(const char** p);
static Value eval_unary(const char** p);
static Value eval_power(const char** p);
static Value eval_mul_div(const char** p);
static Value eval_add_sub(const char** p);
static Value eval_rel(const char** p);
static Value eval_not(const char** p);
static Value eval_and(const char** p);
static Value eval_or(const char** p);

static Value eval_factor(const char** p) {
    skip_spaces(p);
    Value v = { .type = VAL_NUM, .num = 0.0f, .str = {0} };

    if (**p == '"') {
        (*p)++;
        v.type = VAL_STR;
        int idx = 0;
        while (**p && **p != '"' && idx < STR_LEN - 1) {
            v.str[idx++] = *(*p)++;
        }
        if (**p == '"') {
            (*p)++;
        }
        v.str[idx] = '\0';
        return v;
    }

    if (**p == '(') {
        (*p)++;
        v = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') {
            (*p)++;
        }
        return v;
    }

    if (isdigit((unsigned char)**p) || **p == '.') {
        char* end_ptr;
        v.num = strtof(*p, &end_ptr);
        *p = end_ptr;
        return v;
    }

    if (match_kw(p, "PEEK")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        uint16_t addr = (uint16_t)arg.num;
        v.num = (float)(*(volatile uint8_t*)addr);
        return v;
    }

    if (match_kw(p, "ABS")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.num = fabsf(arg.num);
        return v;
    }

    if (match_kw(p, "INT")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.num = floorf(arg.num);
        return v;
    }

    if (match_kw(p, "SGN")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.num = (arg.num > 0.0f) ? 1.0f : ((arg.num < 0.0f) ? -1.0f : 0.0f);
        return v;
    }

    if (match_kw(p, "SQR")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.num = sqrtf(arg.num);
        return v;
    }

    if (match_kw(p, "RND")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        if (arg.num < 0.0f) {
            srand((unsigned int)(-(long)arg.num));
            last_rnd = (float)rand() / (float)RAND_MAX;
        } else if (arg.num > 0.0f) {
            last_rnd = (float)rand() / (float)RAND_MAX;
        }
        v.num = last_rnd;
        return v;
    }

    if (match_kw(p, "SIN")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.num = sinf(arg.num);
        return v;
    }

    if (match_kw(p, "COS")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.num = cosf(arg.num);
        return v;
    }

    if (match_kw(p, "TAN")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.num = tanf(arg.num);
        return v;
    }

    if (match_kw(p, "ATN")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.num = atanf(arg.num);
        return v;
    }

    if (match_kw(p, "EXP")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.num = expf(arg.num);
        return v;
    }

    if (match_kw(p, "LOG")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.num = logf(arg.num);
        return v;
    }

    if (match_kw(p, "LEN")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.num = (float)strlen(arg.str);
        return v;
    }

    if (match_kw(p, "STR$")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.type = VAL_STR;
        float_to_str(arg.num, v.str, sizeof(v.str));
        return v;
    }

    if (match_kw(p, "VAL")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        const char* s = arg.str;
        while (*s == ' ') s++;
        v.num = (float)atof(s);
        return v;
    }

    if (match_kw(p, "CHR$")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.type = VAL_STR;
        v.str[0] = (char)(int)arg.num;
        v.str[1] = '\0';
        return v;
    }

    if (match_kw(p, "ASC")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value arg = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.num = (arg.str[0] != '\0') ? (float)(uint8_t)arg.str[0] : 0.0f;
        return v;
    }

    if (match_kw(p, "LEFT$")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value s = eval_expr(p);
        skip_spaces(p);
        if (**p == ',') (*p)++;
        Value n = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.type = VAL_STR;
        int len = (int)n.num;
        if (len < 0) len = 0;
        int orig_len = (int)strlen(s.str);
        if (len > orig_len) len = orig_len;
        if (len >= STR_LEN) len = STR_LEN - 1;
        strncpy(v.str, s.str, (size_t)len);
        v.str[len] = '\0';
        return v;
    }

    if (match_kw(p, "RIGHT$")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value s = eval_expr(p);
        skip_spaces(p);
        if (**p == ',') (*p)++;
        Value n = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.type = VAL_STR;
        int len = (int)n.num;
        if (len < 0) len = 0;
        int orig_len = (int)strlen(s.str);
        if (len > orig_len) len = orig_len;
        if (len >= STR_LEN) len = STR_LEN - 1;
        strncpy(v.str, s.str + orig_len - len, (size_t)len);
        v.str[len] = '\0';
        return v;
    }

    if (match_kw(p, "MID$")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        Value s = eval_expr(p);
        skip_spaces(p);
        if (**p == ',') (*p)++;
        Value pos = eval_expr(p);
        Value count = { .type = VAL_NUM, .num = 255.0f, .str = {0} };
        skip_spaces(p);
        if (**p == ',') {
            (*p)++;
            count = eval_expr(p);
        }
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.type = VAL_STR;
        int orig_len = (int)strlen(s.str);
        int start = (int)pos.num - 1;
        if (start < 0) start = 0;
        int len = (int)count.num;
        if (len < 0) len = 0;
        if (start >= orig_len) {
            v.str[0] = '\0';
        } else {
            if (start + len > orig_len) len = orig_len - start;
            if (len >= STR_LEN) len = STR_LEN - 1;
            strncpy(v.str, s.str + start, (size_t)len);
            v.str[len] = '\0';
        }
        return v;
    }

    if (match_kw(p, "FRE")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        uint8_t* end = prog_end();
        v.num = (float)(PROG_BUF_SIZE - (size_t)(end + 2 - prog_buf));
        return v;
    }

    if (match_kw(p, "POS")) {
        skip_spaces(p);
        if (**p == '(') (*p)++;
        eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        v.num = (float)current_col;
        return v;
    }

    VarRef ref;
    if (parse_var_name(p, &ref)) {
        skip_spaces(p);
        if (**p == '(') {
            (*p)++;
            Value idx_val = eval_expr(p);
            skip_spaces(p);
            if (**p == ')') (*p)++;
            int idx = (int)idx_val.num;
            struct Array* arr = find_or_create_array(&ref, 10);
            if (!arr || idx < 0 || idx > arr->dim) {
                println("?BAD SUBSCRIPT");
                runtime_error = true;
                return v;
            }
            if (ref.is_str) {
                v.type = VAL_STR;
                if (arr->str_data) {
                    strncpy(v.str, arr->str_data[idx], STR_LEN - 1);
                    v.str[STR_LEN - 1] = '\0';
                }
            } else {
                v.num = arr->num_data ? arr->num_data[idx] : 0.0f;
            }
            return v;
        }

        if (ref.is_str) {
            v.type = VAL_STR;
            strncpy(v.str, get_str_var(&ref), STR_LEN - 1);
            v.str[STR_LEN - 1] = '\0';
        } else {
            v.num = get_num_var(&ref);
        }
        return v;
    }

    return v;
}

static Value eval_unary(const char** p) {
    skip_spaces(p);
    if (**p == '+') {
        (*p)++;
        return eval_unary(p);
    }
    if (**p == '-') {
        (*p)++;
        Value a = eval_unary(p);
        a.num = -a.num;
        return a;
    }
    return eval_factor(p);
}

static Value eval_power(const char** p) {
    Value a = eval_unary(p);
    skip_spaces(p);
    while (**p == '^') {
        (*p)++;
        Value b = eval_unary(p);
        a.num = powf(a.num, b.num);
        skip_spaces(p);
    }
    return a;
}

static Value eval_mul_div(const char** p) {
    Value a = eval_power(p);
    skip_spaces(p);
    while (**p == '*' || **p == '/') {
        char op = **p;
        (*p)++;
        Value b = eval_power(p);
        if (op == '*') {
            a.num *= b.num;
        } else {
            if (b.num == 0.0f) {
                println("?DIVISION BY ZERO");
                runtime_error = true;
                return a;
            }
            a.num /= b.num;
        }
        skip_spaces(p);
    }
    return a;
}

static Value eval_add_sub(const char** p) {
    Value a = eval_mul_div(p);
    skip_spaces(p);
    while (**p == '+' || **p == '-') {
        char op = **p;
        (*p)++;
        Value b = eval_mul_div(p);
        if (op == '+') {
            if (a.type == VAL_STR || b.type == VAL_STR) {
                char buf[STR_LEN];
                buf[0] = '\0';
                strncat(buf, a.str, STR_LEN - 1);
                size_t len = strlen(buf);
                if (len < STR_LEN - 1) {
                    strncat(buf, b.str, STR_LEN - 1 - len);
                }
                a.type = VAL_STR;
                strncpy(a.str, buf, STR_LEN - 1);
                a.str[STR_LEN - 1] = '\0';
            } else {
                a.num += b.num;
            }
        } else {
            a.num -= b.num;
        }
        skip_spaces(p);
    }
    return a;
}

static Value eval_rel(const char** p) {
    Value a = eval_add_sub(p);
    skip_spaces(p);
    while (1) {
        int op = 0;
        if ((*p)[0] == '<' && (*p)[1] == '=') {
            op = 1; *p += 2;
        } else if ((*p)[0] == '>' && (*p)[1] == '=') {
            op = 2; *p += 2;
        } else if ((*p)[0] == '<' && (*p)[1] == '>') {
            op = 3; *p += 2;
        } else if ((*p)[0] == '<') {
            op = 4; (*p)++;
        } else if ((*p)[0] == '>') {
            op = 5; (*p)++;
        } else if ((*p)[0] == '=') {
            op = 6; (*p)++;
        } else {
            break;
        }

        Value b = eval_add_sub(p);
        bool res = false;
        if (a.type == VAL_STR && b.type == VAL_STR) {
            int c = strcmp(a.str, b.str);
            if (op == 1) res = (c <= 0);
            else if (op == 2) res = (c >= 0);
            else if (op == 3) res = (c != 0);
            else if (op == 4) res = (c < 0);
            else if (op == 5) res = (c > 0);
            else if (op == 6) res = (c == 0);
        } else {
            float v1 = a.num;
            float v2 = b.num;
            if (op == 1) res = (v1 <= v2);
            else if (op == 2) res = (v1 >= v2);
            else if (op == 3) res = (v1 != v2);
            else if (op == 4) res = (v1 < v2);
            else if (op == 5) res = (v1 > v2);
            else if (op == 6) res = (v1 == v2);
        }
        a.type = VAL_NUM;
        a.num = res ? -1.0f : 0.0f;
        skip_spaces(p);
    }
    return a;
}

static Value eval_not(const char** p) {
    skip_spaces(p);
    if (match_kw(p, "NOT")) {
        Value a = eval_not(p);
        float res = (a.num == 0.0f) ? -1.0f : 0.0f;
        if (a.num != 0.0f && a.num != -1.0f) {
            res = (float)(~(int)a.num);
        }
        return (Value){ .type = VAL_NUM, .num = res, .str = {0} };
    }
    return eval_rel(p);
}

static Value eval_and(const char** p) {
    Value a = eval_not(p);
    while (match_kw(p, "AND")) {
        Value b = eval_not(p);
        bool ta = (a.num != 0.0f);
        bool tb = (b.num != 0.0f);
        if ((a.num == -1.0f || a.num == 0.0f) && (b.num == -1.0f || b.num == 0.0f)) {
            a.num = (ta && tb) ? -1.0f : 0.0f;
        } else {
            a.num = (float)((int)a.num & (int)b.num);
        }
    }
    return a;
}

static Value eval_or(const char** p) {
    Value a = eval_and(p);
    while (match_kw(p, "OR")) {
        Value b = eval_and(p);
        bool ta = (a.num != 0.0f);
        bool tb = (b.num != 0.0f);
        if ((a.num == -1.0f || a.num == 0.0f) && (b.num == -1.0f || b.num == 0.0f)) {
            a.num = (ta || tb) ? -1.0f : 0.0f;
        } else {
            a.num = (float)((int)a.num | (int)b.num);
        }
    }
    return a;
}

static Value eval_expr(const char** p) {
    return eval_or(p);
}

static void read_line_raw(char* buf, size_t max) {
    size_t len = 0;
    while (1) {
        char c = bios_getchar();
        if (c == '\r' || c == '\n') {
            bios_putchar('\r');
            bios_putchar('\n');
            buf[len] = '\0';
            return;
        }
        if (c == '\b' || c == 127) {
            if (len > 0) {
                len--;
                bios_putchar('\b');
                bios_putchar(' ');
                bios_putchar('\b');
            }
            continue;
        }
        if (c == 3) {
            buf[0] = '\0';
            println("^C");
            interrupted = true;
            return;
        }
        if (c >= 32 && c <= 126) {
            if (len < max - 1) {
                buf[len++] = c;
                bios_putchar(c);
            }
        }
    }
}

static void exec_print(const char** p) {
    skip_spaces(p);
    if (**p == '\0' || **p == ':') {
        println("");
        return;
    }
    bool trailing_sep = false;
    while (**p && **p != ':') {
        skip_spaces(p);
        if (**p == ';') {
            (*p)++;
            trailing_sep = true;
            continue;
        }
        if (**p == ',') {
            (*p)++;
            uint8_t next_tab = (uint8_t)(((current_col / 14) + 1) * 14);
            while (current_col < next_tab) {
                out_char(' ');
            }
            trailing_sep = true;
            continue;
        }
        if (match_kw(p, "TAB")) {
            skip_spaces(p);
            if (**p == '(') (*p)++;
            Value v = eval_expr(p);
            skip_spaces(p);
            if (**p == ')') (*p)++;
            int target_col = (int)v.num;
            while (current_col < target_col) {
                out_char(' ');
            }
            trailing_sep = false;
            continue;
        }
        if (match_kw(p, "SPC")) {
            skip_spaces(p);
            if (**p == '(') (*p)++;
            Value v = eval_expr(p);
            skip_spaces(p);
            if (**p == ')') (*p)++;
            int count = (int)v.num;
            for (int i = 0; i < count; i++) {
                out_char(' ');
            }
            trailing_sep = false;
            continue;
        }

        Value v = eval_expr(p);
        if (runtime_error) {
            return;
        }
        if (v.type == VAL_STR) {
            out_str(v.str);
        } else {
            print_basic_number(v.num);
        }
        trailing_sep = false;
    }
    if (!trailing_sep) {
        println("");
    }
}

static void exec_let(const char** p) {
    match_kw(p, "LET");
    VarRef ref;
    if (!parse_var_name(p, &ref)) {
        println("?SYNTAX ERROR");
        runtime_error = true;
        return;
    }
    skip_spaces(p);
    if (**p == '(') {
        (*p)++;
        Value idx_val = eval_expr(p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        skip_spaces(p);
        if (**p == '=') (*p)++;
        Value val = eval_expr(p);
        struct Array* arr = find_or_create_array(&ref, 10);
        int idx = (int)idx_val.num;
        if (!arr || idx < 0 || idx > arr->dim) {
            println("?BAD SUBSCRIPT");
            runtime_error = true;
            return;
        }
        if (ref.is_str) {
            if (arr->str_data) {
                strncpy(arr->str_data[idx], val.str, STR_LEN - 1);
                arr->str_data[idx][STR_LEN - 1] = '\0';
            }
        } else {
            if (arr->num_data) {
                arr->num_data[idx] = val.num;
            }
        }
    } else {
        if (**p == '=') (*p)++;
        Value val = eval_expr(p);
        if (ref.is_str) {
            set_str_var(&ref, val.str);
        } else {
            set_num_var(&ref, val.num);
        }
    }
}

static void exec_if(const char** p) {
    Value cond = eval_expr(p);
    if (!match_kw(p, "THEN")) {
        println("?SYNTAX ERROR");
        runtime_error = true;
        return;
    }
    skip_spaces(p);
    if (cond.num != 0.0f) {
        if (isdigit((unsigned char)**p)) {
            uint16_t target = (uint16_t)strtoul(*p, (char**)p, 10);
            goto_line(target);
        } else {
            execute_statement(p);
        }
    } else {
        const char* s = *p;
        bool in_q = false;
        const char* else_pos = NULL;
        while (*s) {
            if (*s == '"') {
                in_q = !in_q;
            } else if (!in_q && (toupper((unsigned char)s[0]) == 'E' &&
                               toupper((unsigned char)s[1]) == 'L' &&
                               toupper((unsigned char)s[2]) == 'S' &&
                               toupper((unsigned char)s[3]) == 'E' &&
                               !isalnum((unsigned char)s[4]))) {
                else_pos = s + 4;
                break;
            }
            s++;
        }
        if (else_pos) {
            *p = else_pos;
            skip_spaces(p);
            if (isdigit((unsigned char)**p)) {
                uint16_t target = (uint16_t)strtoul(*p, (char**)p, 10);
                goto_line(target);
            } else {
                execute_statement(p);
            }
        } else {
            *p += strlen(*p);
        }
    }
}

static void exec_for(const char** p) {
    VarRef ref;
    if (!parse_var_name(p, &ref)) {
        println("?SYNTAX ERROR");
        runtime_error = true;
        return;
    }
    skip_spaces(p);
    if (**p == '=') (*p)++;
    Value start_val = eval_expr(p);
    set_num_var(&ref, start_val.num);
    if (!match_kw(p, "TO")) {
        println("?SYNTAX ERROR");
        runtime_error = true;
        return;
    }
    Value end_val = eval_expr(p);
    float step_val = 1.0f;
    if (match_kw(p, "STEP")) {
        Value st = eval_expr(p);
        step_val = st.num;
    }
    if (for_sp >= FOR_DEPTH) {
        println("?OUT OF MEMORY");
        runtime_error = true;
        return;
    }
    for_stack[for_sp].var = ref;
    for_stack[for_sp].end_val = end_val.num;
    for_stack[for_sp].step_val = step_val;
    for_stack[for_sp].line_num = current_line_num;
    for_stack[for_sp].loop_stmt = *p;
    for_sp++;
}

static void exec_next(const char** p) {
    VarRef ref;
    bool has_var = parse_var_name(p, &ref);
    int target_sp = -1;
    if (has_var) {
        for (int i = (int)for_sp - 1; i >= 0; i--) {
            if (for_stack[i].var.base == ref.base && for_stack[i].var.ext == ref.ext) {
                target_sp = i;
                break;
            }
        }
    } else {
        if (for_sp > 0) {
            target_sp = (int)for_sp - 1;
        }
    }
    if (target_sp < 0) {
        println("?NEXT WITHOUT FOR");
        runtime_error = true;
        return;
    }

    struct ForFrame* f = &for_stack[target_sp];
    float v = get_num_var(&f->var) + f->step_val;
    set_num_var(&f->var, v);
    bool loop_again = (f->step_val >= 0.0f) ? (v <= f->end_val) : (v >= f->end_val);

    if (loop_again) {
        current_line_num = f->line_num;
        uint8_t* pl = find_line(f->line_num);
        if (pl) {
            current_line_ptr = pl;
        }
        current_stmt_ptr = f->loop_stmt;
        *p = f->loop_stmt;
        line_jumped = true;
        for_sp = (uint8_t)(target_sp + 1);
    } else {
        for_sp = (uint8_t)target_sp;
    }
}

static void exec_input(const char** p) {
    skip_spaces(p);
    if (**p == '"') {
        (*p)++;
        while (**p && **p != '"') {
            out_char(*(*p)++);
        }
        if (**p == '"') (*p)++;
        skip_spaces(p);
        if (**p == ';' || **p == ',') (*p)++;
    } else {
        out_str("? ");
    }

    char in_buf[80];
    read_line_raw(in_buf, sizeof(in_buf));
    const char* in_p = in_buf;

    while (1) {
        VarRef ref;
        if (!parse_var_name(p, &ref)) break;
        skip_spaces(&in_p);
        if (ref.is_str) {
            char val_str[STR_LEN];
            int idx = 0;
            if (*in_p == '"') {
                in_p++;
                while (*in_p && *in_p != '"' && idx < STR_LEN - 1) {
                    val_str[idx++] = *in_p++;
                }
                if (*in_p == '"') in_p++;
            } else {
                while (*in_p && *in_p != ',' && idx < STR_LEN - 1) {
                    val_str[idx++] = *in_p++;
                }
            }
            val_str[idx] = '\0';
            set_str_var(&ref, val_str);
        } else {
            float f = (float)atof(in_p);
            set_num_var(&ref, f);
            while (*in_p && *in_p != ',') in_p++;
        }
        skip_spaces(&in_p);
        if (*in_p == ',') in_p++;
        skip_spaces(p);
        if (**p == ',') {
            (*p)++;
        } else {
            break;
        }
    }
}

static void find_next_data(void) {
    uint8_t* p = data_line_ptr ? data_line_ptr : prog_buf;
    if (data_line_ptr && data_item_ptr) {
        p += 4 + p[2];
    }
    while (p < prog_buf + PROG_BUF_SIZE - 2) {
        uint16_t lnum = (uint16_t)(p[0] | (p[1] << 8));
        if (lnum == 0xFFFF) break;
        const char* s = (const char*)(p + 3);
        bool in_q = false;
        while (*s) {
            if (*s == '"') {
                in_q = !in_q;
            } else if (!in_q && toupper((unsigned char)s[0]) == 'D' &&
                               toupper((unsigned char)s[1]) == 'A' &&
                               toupper((unsigned char)s[2]) == 'T' &&
                               toupper((unsigned char)s[3]) == 'A' &&
                               !isalnum((unsigned char)s[4])) {
                data_line_ptr = p;
                data_item_ptr = s + 4;
                return;
            }
            s++;
        }
        p += 4 + p[2];
    }
    data_line_ptr = NULL;
    data_item_ptr = NULL;
}

static void exec_read(const char** p) {
    while (1) {
        VarRef ref;
        if (!parse_var_name(p, &ref)) {
            break;
        }
        if (!data_item_ptr || *data_item_ptr == '\0' || *data_item_ptr == ':') {
            find_next_data();
        }
        if (!data_item_ptr) {
            println("?OUT OF DATA");
            runtime_error = true;
            return;
        }
        skip_spaces(&data_item_ptr);
        if (ref.is_str) {
            char val_str[STR_LEN];
            int idx = 0;
            if (*data_item_ptr == '"') {
                data_item_ptr++;
                while (*data_item_ptr && *data_item_ptr != '"' && idx < STR_LEN - 1) {
                    val_str[idx++] = *data_item_ptr++;
                }
                if (*data_item_ptr == '"') data_item_ptr++;
            } else {
                while (*data_item_ptr && *data_item_ptr != ',' && *data_item_ptr != ':' && idx < STR_LEN - 1) {
                    val_str[idx++] = *data_item_ptr++;
                }
            }
            val_str[idx] = '\0';
            set_str_var(&ref, val_str);
        } else {
            float f = (float)atof(data_item_ptr);
            set_num_var(&ref, f);
            while (*data_item_ptr && *data_item_ptr != ',' && *data_item_ptr != ':') {
                data_item_ptr++;
            }
        }
        skip_spaces(&data_item_ptr);
        if (*data_item_ptr == ',') {
            data_item_ptr++;
        }
        skip_spaces(p);
        if (**p == ',') {
            (*p)++;
        } else {
            break;
        }
    }
}

static void exec_restore(const char** p) {
    skip_spaces(p);
    if (isdigit((unsigned char)**p)) {
        uint16_t target = (uint16_t)strtoul(*p, (char**)p, 10);
        data_line_ptr = find_line(target);
    } else {
        data_line_ptr = prog_buf;
    }
    data_item_ptr = NULL;
}

static void exec_wait(const char** p) {
    Value a = eval_expr(p);
    skip_spaces(p);
    if (**p == ',') (*p)++;
    Value m = eval_expr(p);
    uint8_t xor_val = 0;
    skip_spaces(p);
    if (**p == ',') {
        (*p)++;
        Value x = eval_expr(p);
        xor_val = (uint8_t)x.num;
    }
    uint16_t addr = (uint16_t)a.num;
    uint8_t mask = (uint8_t)m.num;
    while (((*(volatile uint8_t*)addr ^ xor_val) & mask) == 0) {
        if (bios_getchar_nb() == 3) {
            interrupted = true;
            break;
        }
    }
}

static void exec_on(const char** p) {
    Value v = eval_expr(p);
    int idx = (int)v.num;
    bool is_gosub = false;
    if (match_kw(p, "GOTO")) {
        is_gosub = false;
    } else if (match_kw(p, "GOSUB")) {
        is_gosub = true;
    } else {
        println("?SYNTAX ERROR");
        runtime_error = true;
        return;
    }
    int count = 0;
    uint16_t target = 0;
    while (1) {
        skip_spaces(p);
        if (!isdigit((unsigned char)**p)) break;
        uint16_t lnum = (uint16_t)strtoul(*p, (char**)p, 10);
        count++;
        if (count == idx) {
            target = lnum;
        }
        skip_spaces(p);
        if (**p == ',') {
            (*p)++;
        } else {
            break;
        }
    }
    if (target != 0) {
        if (is_gosub) {
            if (gosub_sp >= GOSUB_DEPTH) {
                println("?OUT OF MEMORY");
                runtime_error = true;
                return;
            }
            gosub_stack[gosub_sp].line_num = current_line_num;
            gosub_stack[gosub_sp].next_stmt = *p;
            gosub_sp++;
        }
        goto_line(target);
    }
}

static void exec_dim(const char** p) {
    VarRef ref;
    if (!parse_var_name(p, &ref)) return;
    skip_spaces(p);
    if (**p == '(') (*p)++;
    Value d = eval_expr(p);
    skip_spaces(p);
    if (**p == ')') (*p)++;
    uint8_t dim = (uint8_t)d.num;

    char name[4];
    name[0] = ref.base;
    int ni = 1;
    if (ref.ext) name[ni++] = ref.ext;
    if (ref.is_str) name[ni++] = '$';
    name[ni] = '\0';

    for (int i = 0; i < MAX_ARRAYS; i++) {
        if (strcmp(arrays[i].name, name) == 0) {
            println("?REDIM'D ARRAY");
            runtime_error = true;
            return;
        }
    }
    if (!find_or_create_array(&ref, dim)) {
        println("?OUT OF MEMORY");
        runtime_error = true;
    }
}

static void exec_list(const char** p) {
    skip_spaces(p);
    uint16_t start = 0;
    uint16_t end = 65534;
    if (isdigit((unsigned char)**p)) {
        start = (uint16_t)strtoul(*p, (char**)p, 10);
        skip_spaces(p);
        if (**p == '-') {
            (*p)++;
            skip_spaces(p);
            if (isdigit((unsigned char)**p)) {
                end = (uint16_t)strtoul(*p, (char**)p, 10);
            } else {
                end = 65534;
            }
        } else {
            end = start;
        }
    } else if (**p == '-') {
        (*p)++;
        skip_spaces(p);
        if (isdigit((unsigned char)**p)) {
            end = (uint16_t)strtoul(*p, (char**)p, 10);
        }
    }

    uint8_t* cur = prog_buf;
    while (cur < prog_buf + PROG_BUF_SIZE - 2) {
        uint16_t lnum = (uint16_t)(cur[0] | (cur[1] << 8));
        if (lnum == 0xFFFF) break;
        if (lnum >= start && lnum <= end) {
            print_num(lnum);
            bios_putchar(' ');
            println((const char*)(cur + 3));
        }
        cur += 4 + cur[2];
    }
}

static void exec_beep(const char** p) {
    uint16_t freq = 0x1124;
    uint16_t dur = 200;
    skip_spaces(p);
    if (**p && **p != ':' && **p != '\n') {
        Value f = eval_expr(p);
        freq = (uint16_t)f.num;
        skip_spaces(p);
        if (**p == ',') {
            (*p)++;
            Value d = eval_expr(p);
            dur = (uint16_t)d.num;
        }
    }
    sid_write(MODE_VOL, 15);
    sid_write(AD_1, 0x09);
    sid_write(SR_1, 0xF0);
    sid_write(PW_LO_1, 0x00);
    sid_write(PW_HI_1, 0x08);
    sid_write(FREQ_LO_1, (uint8_t)(freq & 0xFF));
    sid_write(FREQ_HI_1, (uint8_t)((freq >> 8) & 0xFF));
    sid_write(CTRL_1, WAVE_PULSE | WAVE_GATE);
    for (uint16_t i = 0; i < dur; i++) {
        for (volatile uint16_t d = 0; d < 200; d++) {}
        if (bios_getchar_nb() == 3) {
            interrupted = true;
            break;
        }
    }
    sid_write(CTRL_1, WAVE_PULSE);
}

static void get_filename(const char** p, char* out, size_t max) {
    skip_spaces(p);
    if (**p == '"') {
        (*p)++;
        size_t idx = 0;
        while (**p && **p != '"' && idx < max - 1) {
            out[idx++] = *(*p)++;
        }
        if (**p == '"') (*p)++;
        out[idx] = '\0';
    } else {
        if (isalpha((unsigned char)**p)) {
            const char* saved = *p;
            VarRef ref;
            if (parse_var_name(p, &ref) && ref.is_str) {
                strncpy(out, get_str_var(&ref), max - 1);
                out[max - 1] = '\0';
                return;
            }
            *p = saved;
        }
        size_t idx = 0;
        while (**p && **p != ' ' && **p != ':' && **p != '\r' && **p != '\n' && idx < max - 1) {
            out[idx++] = *(*p)++;
        }
        out[idx] = '\0';
    }
}

static void exec_save(const char** p) {
    char filename[32];
    get_filename(p, filename, sizeof(filename));
    if (!filename[0]) {
        println("?MISSING FILENAME");
        return;
    }
    SD_FILE fp;
    if (sd_open(&fp, filename, SD_WRITE | SD_CREATE_ALWAYS) != 0) {
        println("?FILE ERROR");
        return;
    }
    uint8_t* cur = prog_buf;
    char num_buf[16];
    while (cur < prog_buf + PROG_BUF_SIZE - 2) {
        uint16_t lnum = (uint16_t)(cur[0] | (cur[1] << 8));
        if (lnum == 0xFFFF) break;
        uint8_t len = cur[2];
        snprintf(num_buf, sizeof(num_buf), "%u ", lnum);
        sd_write(&fp, num_buf, (uint16_t)strlen(num_buf));
        sd_write(&fp, cur + 3, len);
        sd_write(&fp, "\r\n", 2);
        cur += 4 + len;
    }
    sd_close(&fp);
    println("OK");
}

static void exec_load(const char** p) {
    char filename[32];
    get_filename(p, filename, sizeof(filename));
    if (!filename[0]) {
        println("?MISSING FILENAME");
        return;
    }
    SD_FILE fp;
    if (sd_open(&fp, filename, SD_READ) != 0) {
        println("?FILE NOT FOUND");
        return;
    }
    clear_program();
    char line_buf[128];
    int idx = 0;
    while (1) {
        int16_t ch = sd_getc(&fp);
        if (ch < 0) break;
        if (ch == '\r') continue;
        if (ch == '\n') {
            line_buf[idx] = '\0';
            if (idx > 0) {
                char* s = line_buf;
                while (*s == ' ') s++;
                if (isdigit((unsigned char)*s)) {
                    uint16_t lnum = (uint16_t)strtoul(s, &s, 10);
                    while (*s == ' ') s++;
                    insert_line(lnum, s);
                }
            }
            idx = 0;
        } else {
            if (idx < 126) line_buf[idx++] = (char)ch;
        }
    }
    if (idx > 0) {
        line_buf[idx] = '\0';
        char* s = line_buf;
        while (*s == ' ') s++;
        if (isdigit((unsigned char)*s)) {
            uint16_t lnum = (uint16_t)strtoul(s, &s, 10);
            while (*s == ' ') s++;
            insert_line(lnum, s);
        }
    }
    sd_close(&fp);
    println("OK");
}

static void exec_run(const char** p) {
    clear_vars();
    uint16_t start_line = 0;
    skip_spaces(p);
    if (isdigit((unsigned char)**p)) {
        start_line = (uint16_t)strtoul(*p, (char**)p, 10);
    }
    uint8_t* entry = (start_line > 0) ? find_line(start_line) : prog_buf;
    if (!entry || (uint16_t)(entry[0] | (entry[1] << 8)) == 0xFFFF) {
        return;
    }
    run_program_from(entry);
}

static void exec_cont(void) {
    if (!break_line_ptr || break_line_num == 0) {
        println("?CANNOT CONTINUE");
        return;
    }
    is_cont = true;
    run_program_from(break_line_ptr);
}

static void execute_statement(const char** p) {
    skip_spaces(p);
    if (**p == '\0') return;

    if (**p == '?' || match_kw(p, "PRINT")) {
        if (**p == '?') (*p)++;
        exec_print(p);
        return;
    }

    if (match_kw(p, "REM")) {
        *p += strlen(*p);
        return;
    }

    if (match_kw(p, "IF")) {
        exec_if(p);
        return;
    }

    if (match_kw(p, "ELSE")) {
        *p += strlen(*p);
        return;
    }

    if (match_kw(p, "GOTO")) {
        Value v = eval_expr(p);
        goto_line((uint16_t)v.num);
        return;
    }

    if (match_kw(p, "GOSUB")) {
        Value v = eval_expr(p);
        if (gosub_sp >= GOSUB_DEPTH) {
            println("?OUT OF MEMORY");
            runtime_error = true;
            return;
        }
        gosub_stack[gosub_sp].line_num = current_line_num;
        gosub_stack[gosub_sp].next_stmt = *p;
        gosub_sp++;
        goto_line((uint16_t)v.num);
        return;
    }

    if (match_kw(p, "RETURN")) {
        if (gosub_sp == 0) {
            println("?RETURN WITHOUT GOSUB");
            runtime_error = true;
            return;
        }
        gosub_sp--;
        uint16_t ret_line = gosub_stack[gosub_sp].line_num;
        const char* ret_stmt = gosub_stack[gosub_sp].next_stmt;
        uint8_t* pl = find_line(ret_line);
        if (!pl || (uint16_t)(pl[0] | (pl[1] << 8)) != ret_line) {
            println("?UNDEF'D STATEMENT");
            runtime_error = true;
            return;
        }
        current_line_ptr = pl;
        current_line_num = ret_line;
        current_stmt_ptr = ret_stmt;
        *p = ret_stmt;
        line_jumped = true;
        return;
    }

    if (match_kw(p, "FOR")) {
        exec_for(p);
        return;
    }

    if (match_kw(p, "NEXT")) {
        exec_next(p);
        return;
    }

    if (match_kw(p, "INPUT")) {
        exec_input(p);
        return;
    }

    if (match_kw(p, "DATA")) {
        *p += strlen(*p);
        return;
    }

    if (match_kw(p, "READ")) {
        exec_read(p);
        return;
    }

    if (match_kw(p, "RESTORE")) {
        exec_restore(p);
        return;
    }

    if (match_kw(p, "STOP")) {
        break_line_num = current_line_num;
        break_stmt_ptr = *p;
        break_line_ptr = current_line_ptr;
        print_str("BREAK IN ");
        print_num(current_line_num);
        println("");
        program_running = false;
        return;
    }

    if (match_kw(p, "CONT")) {
        exec_cont();
        return;
    }

    if (match_kw(p, "END")) {
        program_running = false;
        *p += strlen(*p);
        return;
    }

    if (match_kw(p, "POKE")) {
        Value a = eval_expr(p);
        skip_spaces(p);
        if (**p == ',') (*p)++;
        Value v = eval_expr(p);
        *(volatile uint8_t*)(uint16_t)a.num = (uint8_t)v.num;
        return;
    }

    if (match_kw(p, "WAIT")) {
        exec_wait(p);
        return;
    }

    if (match_kw(p, "ON")) {
        exec_on(p);
        return;
    }

    if (match_kw(p, "DIM")) {
        exec_dim(p);
        return;
    }

    if (match_kw(p, "LIST")) {
        exec_list(p);
        return;
    }

    if (match_kw(p, "NEW")) {
        clear_program();
        return;
    }

    if (match_kw(p, "CLEAR")) {
        clear_vars();
        return;
    }

    if (match_kw(p, "RUN")) {
        exec_run(p);
        return;
    }

    if (match_kw(p, "LCDINIT")) {
        lcd_init_hw();
        return;
    }

    if (match_kw(p, "LCDCMD")) {
        Value v = eval_expr(p);
        lcd_instruction((uint8_t)v.num);
        return;
    }

    if (match_kw(p, "LCDPRINT")) {
        Value v = eval_expr(p);
        if (v.type == VAL_STR) {
            for (const char* s = v.str; *s; s++) {
                lcd_print_char(*s);
            }
        } else {
            char num_buf[16];
            long n = (long)v.num;
            if ((float)n == v.num) {
                snprintf(num_buf, sizeof(num_buf), "%ld", n);
            } else {
                snprintf(num_buf, sizeof(num_buf), "%g", (double)v.num);
            }
            for (const char* s = num_buf; *s; s++) {
                lcd_print_char(*s);
            }
        }
        return;
    }

    if (match_kw(p, "BEEP")) {
        exec_beep(p);
        return;
    }

    if (match_kw(p, "SOUND")) {
        Value r = eval_expr(p);
        skip_spaces(p);
        if (**p == ',') (*p)++;
        Value v = eval_expr(p);
        sid_write((uint8_t)r.num, (uint8_t)v.num);
        return;
    }

    if (match_kw(p, "SAVE")) {
        exec_save(p);
        return;
    }

    if (match_kw(p, "LOAD")) {
        exec_load(p);
        return;
    }

    if (match_kw(p, "EXIT")) {
        sid_reset();
        println("");
        exit(0);
    }

    const char* probe = *p;
    VarRef ref;
    if (parse_var_name(&probe, &ref)) {
        skip_spaces(&probe);
        if (*probe == '=' || *probe == '(') {
            exec_let(p);
            return;
        }
    }

    if (match_kw(p, "LET")) {
        exec_let(p);
        return;
    }

    println("?SYNTAX ERROR");
    runtime_error = true;
}

static void run_program_from(uint8_t* start_line) {
    program_running = true;
    interrupted = false;
    runtime_error = false;
    current_line_ptr = start_line;

    while (program_running && current_line_ptr) {
        current_line_num = (uint16_t)(current_line_ptr[0] | (current_line_ptr[1] << 8));
        if (current_line_num == 0xFFFF) break;

        if (is_cont) {
            current_stmt_ptr = break_stmt_ptr;
            is_cont = false;
        } else {
            current_stmt_ptr = (const char*)(current_line_ptr + 3);
        }

        while (*current_stmt_ptr && program_running) {
            line_jumped = false;

            if (bios_getchar_nb() == 3) {
                break_line_num = current_line_num;
                break_stmt_ptr = current_stmt_ptr;
                break_line_ptr = current_line_ptr;
                print_str("\r\nBREAK IN ");
                print_num(current_line_num);
                println("");
                program_running = false;
                interrupted = true;
                return;
            }

            skip_spaces(&current_stmt_ptr);
            if (*current_stmt_ptr == '\0') break;
            if (*current_stmt_ptr == ':') {
                current_stmt_ptr++;
                continue;
            }

            execute_statement(&current_stmt_ptr);
            if (runtime_error) {
                print_str(" IN ");
                print_num(current_line_num);
                println("");
                program_running = false;
                return;
            }
            if (line_jumped) {
                break;
            }
        }

        if (!line_jumped && program_running) {
            current_line_ptr += 4 + current_line_ptr[2];
        }
    }
    program_running = false;
}

int main(void) {
    println("microDOS MSBasic 1.0");
    println("READY.");
    clear_program();

    char line_buf[128];
    while (1) {
        read_line_raw(line_buf, sizeof(line_buf));
        const char* p = line_buf;
        skip_spaces(&p);
        if (*p == '\0') {
            continue;
        }

        if (isdigit((unsigned char)*p)) {
            uint16_t lnum = (uint16_t)strtoul(p, (char**)&p, 10);
            while (*p == ' ') p++;
            insert_line(lnum, p);
        } else {
            runtime_error = false;
            interrupted = false;
            current_line_num = 0;
            current_line_ptr = NULL;

            while (*p) {
                skip_spaces(&p);
                if (*p == '\0') break;
                if (*p == ':') {
                    p++;
                    continue;
                }

                if (match_kw(&p, "EXIT")) {
                    sid_reset();
                    println("");
                    return 0;
                }

                execute_statement(&p);
                if (runtime_error || interrupted) break;
            }
            if (!runtime_error && !interrupted) {
                println("READY.");
            }
        }
    }
    return 0;
}
