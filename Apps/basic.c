#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "Libs/app-bios.h"

#pragma clang section text=".overlay_4"
#include "Libs/LCD.h"
#pragma clang section text=""

#define BASIC_PI      3.14159265f
#define PROG_BUF_SIZE 3072
#define MAX_ARRAYS    8
#define GOSUB_DEPTH   16
#define FOR_DEPTH     16
#define STR_LEN       32

#define OV1       __attribute__((section(".overlay_1"), noinline))
#define OV2       __attribute__((section(".overlay_2"), noinline))
#define OV3       __attribute__((section(".overlay_3"), noinline))
#define OV4       __attribute__((section(".overlay_4"), noinline))

#define OV1_ENTRY __attribute__((section(".overlay_1"), used, noinline))
#define OV2_ENTRY __attribute__((section(".overlay_2"), used, noinline))
#define OV3_ENTRY __attribute__((section(".overlay_3"), used, noinline))
#define OV4_ENTRY __attribute__((section(".overlay_4"), used, noinline))

// Resident stub: jsr overlay_dispatcher + page + real address (same layout as OVERLAY_TRAMPOLINE in App-Runtime.s).
#define OVSTUB(page, name) typeof(name##_ov) name; \
    __asm__(".pushsection .text." #name ",\"ax\"\n.globl " #name "\n" #name ":\n" \
            "\tjsr overlay_dispatcher\n\t.byte " #page "\n\t.byte <" #name "_ov\n\t.byte >" #name "_ov\n.popsection")

typedef enum : uint8_t {
    VAL_NUM = 0,
    VAL_STR = 1
} ValType;

typedef struct {
    uint8_t type;
    float num;
    const char* str;
    uint8_t len;
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
static bool exit_requested;

static uint16_t current_line_num;
static const char* current_stmt_ptr;
static uint8_t* current_line_ptr;

static uint16_t break_line_num;
static const char* break_stmt_ptr;
static uint8_t* break_line_ptr;

static uint8_t current_col;
static float last_rnd = 0.5f;
static uint16_t rnd_state = 0xACE1u;

static char str_tmp_buf[2][STR_LEN];
static uint8_t str_tmp_idx;

static inline bool is_digit(char c) {
    return (uint8_t)(c - '0') < 10u;
}

static inline char to_upper(char c) {
    return (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
}

static inline bool is_alpha(char c) {
    return (uint8_t)((c | 32) - 'a') < 26u;
}

static inline bool is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

static inline bool basic_isnan(float f) {
    return __builtin_isnan(f);
}

static inline bool basic_isinf(float f) {
    uint32_t u;
    memcpy(&u, &f, 4);
    return (u & 0x7FFFFFFFu) == 0x7F800000u;
}

static inline float basic_fabs(float x) { return (x < 0.0f) ? -x : x; }

static inline float basic_floor(float x) {
    int32_t n = (int32_t)x;
    if (x < (float)n) n--;
    return (float)n;
}

static void out_char(char c) {
    if (c == '\r' || c == '\n') current_col = 0;
    else if (c == '\b') { if (current_col > 0) current_col--; }
    else current_col++;
    bios_putchar(c);
}

static void out_str(const char* s) {
    while (*s) out_char(*s++);
}

static void err_fail(const char* msg) {
    out_char('?');
    println(msg);
    runtime_error = true;
}

static void err_syntax(void) {
    err_fail("SYNTAX ERROR");
}

static void err_oom(void) {
    err_fail("OUT OF MEMORY");
}

static Value make_num(float n) {
    return (Value){ .type = VAL_NUM, .num = n, .str = nullptr, .len = 0 };
}

static Value make_str_ref(const char* s, uint8_t len) {
    return (Value){ .type = VAL_STR, .num = 0.0f, .str = s, .len = len };
}

static Value make_str_tmp(const char* s, uint8_t len) {
    char* buf = str_tmp_buf[str_tmp_idx & 1];
    str_tmp_idx++;
    if (len >= STR_LEN) len = STR_LEN - 1;
    memcpy(buf, s, len);
    buf[len] = '\0';
    return (Value){ .type = VAL_STR, .num = 0.0f, .str = buf, .len = len };
}

static void skip_spaces(const char** p) {
    while (**p == ' ' || **p == '\t') (*p)++;
}

static bool skip_paren(const char** p, char expected) {
    skip_spaces(p);
    if (**p == expected) {
        (*p)++;
        return true;
    }
    return false;
}

static bool match_kw(const char** p, const char* kw) {
    skip_spaces(p);
    const char* s = *p;
    while (*kw) {
        if (to_upper(*s) != *kw) {
            return false;
        }
        s++;
        kw++;
    }
    char last = *(kw - 1);
    if (is_alnum(last) && is_alnum(*s)) {
        return false;
    }
    *p = s;
    return true;
}

static uint16_t parse_u16(const char** p) {
    uint16_t v = 0;
    while (is_digit(**p)) {
        v = (uint16_t)(v * 10u + (uint8_t)(*(*p)++ - '0'));
    }
    return v;
}

static void clear_program(void) {
    prog_buf[0] = 0xFF;
    prog_buf[1] = 0xFF;
}

/* =========================================================================
 * OVERLAY 2: Math, Number Formatting, Program Lines, Variables, DATA/DIM/HW
 * ========================================================================= */

OV2 static float basic_sqrt_ov(float x) {
    if (x <= 0.0f) return 0.0f;
    float g = (x > 1.0f) ? (x * 0.5f) : 1.0f;
    for (uint8_t i = 0; i < 8; i++) g = 0.5f * (g + x / g);
    return g;
}

OV2 static float basic_sin_ov(float x) {
    while (x > BASIC_PI) x -= 2.0f * BASIC_PI;
    while (x < -BASIC_PI) x += 2.0f * BASIC_PI;
    float x2 = x * x;
    return x * (1.0f - x2 * (0.16666667f - x2 * (0.00833333f - x2 * 0.00019841f)));
}

OV2 static float basic_atan_ov(float x) {
    if (x > 1.0f) return (BASIC_PI * 0.5f) - basic_atan_ov(1.0f / x);
    if (x < -1.0f) return -(BASIC_PI * 0.5f) - basic_atan_ov(1.0f / x);
    float x2 = x * x;
    return x * (1.0f - x2 * (0.33333333f - x2 * (0.2f - x2 * 0.14285714f)));
}

OV2 static float basic_exp_ov(float x) {
    if (x < -20.0f) return 0.0f;
    if (x > 20.0f) return 1e9f;
    float term = 1.0f, sum = 1.0f;
    for (uint8_t i = 1; i <= 10; i++) {
        term *= x / (float)i;
        sum += term;
    }
    return sum;
}

OV2 static float basic_log_ov(float x) {
    if (x <= 0.0f) return -999999.0f;
    int8_t k = 0;
    while (x > 2.0f) { x *= 0.5f; k++; }
    while (x < 1.0f) { x *= 2.0f; k--; }
    float z = (x - 1.0f) / (x + 1.0f);
    float z2 = z * z;
    float sum = z * (2.0f + 0.66666667f * z2 + 0.4f * z2 * z2);
    return sum + (float)k * 0.69314718f;
}

OV2_ENTRY static float basic_pow_ov(float x, float y) {
    if (y == 0.0f) return 1.0f;
    if (x == 0.0f) return 0.0f;
    int16_t iy = (int16_t)y;
    if ((float)iy == y && iy > 0 && iy < 16) {
        float r = 1.0f;
        for (int16_t i = 0; i < iy; i++) r *= x;
        return r;
    }
    if (x < 0.0f) return 0.0f;
    return basic_exp_ov(y * basic_log_ov(x));
}
OVSTUB(1, basic_pow);

static float eval_paren_num_ov(const char** p);
OVSTUB(0, eval_paren_num);

OV2_ENTRY static bool check_math_fn_ov(const char** p, Value* out) {
    static const char names[] = "ABS\0INT\0SGN\0SQR\0SIN\0COS\0TAN\0ATN\0EXP\0LOG";
    for (uint8_t i = 0; i < 10; i++) {
        if (match_kw(p, &names[i * 4])) {
            float x = eval_paren_num(p);
            float r = 0.0f;
            switch (i) {
                case 0: r = basic_fabs(x); break;
                case 1: r = basic_floor(x); break;
                case 2: r = (x > 0.0f) ? 1.0f : ((x < 0.0f) ? -1.0f : 0.0f); break;
                case 3: r = basic_sqrt_ov(x); break;
                case 4: r = basic_sin_ov(x); break;
                case 5: r = basic_sin_ov(x + (BASIC_PI * 0.5f)); break;
                case 6: {
                    float c = basic_sin_ov(x + (BASIC_PI * 0.5f));
                    r = (c == 0.0f) ? 999999.0f : (basic_sin_ov(x) / c);
                    break;
                }
                case 7: r = basic_atan_ov(x); break;
                case 8: r = basic_exp_ov(x); break;
                case 9: r = basic_log_ov(x); break;
            }
            *out = make_num(r);
            return true;
        }
    }
    return false;
}
OVSTUB(1, check_math_fn);

OV2 static uint8_t u32_div10(uint32_t* val) {
    uint32_t q = 0, r = 0, x = *val;
    for (int8_t i = 31; i >= 0; i--) {
        r = (r << 1) | ((x >> i) & 1u);
        if (r >= 10u) {
            r -= 10u;
            q |= (1ul << i);
        }
    }
    *val = q;
    return (uint8_t)r;
}

OV2_ENTRY static uint8_t format_float_ov(float f, char* buf, uint8_t max_len) {
    if (max_len < 2) { if (max_len > 0) buf[0] = '\0'; return 0; }
    if (basic_isnan(f)) { memcpy(buf, "NAN", 4); return 3; }
    if (basic_isinf(f)) {
        memcpy(buf, (f < 0.0f) ? "-INF" : " INF", 5);
        return 4;
    }
    if (f == 0.0f) { memcpy(buf, " 0", 3); return 2; }

    uint8_t idx = 0;
    if (f < 0.0f) {
        buf[idx++] = '-';
        f = -f;
    } else {
        buf[idx++] = ' ';
    }

    uint32_t int_part = (uint32_t)(int32_t)f;
    float frac = f - (float)int_part;

    char tmp[10];
    uint8_t ti = 0;
    if (int_part == 0) {
        tmp[ti++] = '0';
    } else {
        uint32_t n = int_part;
        while (n > 0 && ti < 9) {
            tmp[ti++] = (char)('0' + u32_div10(&n));
        }
    }
    while (ti > 0 && idx < max_len - 1) buf[idx++] = tmp[--ti];

    if (frac > 0.0001f && idx < max_len - 2) {
        buf[idx++] = '.';
        for (uint8_t i = 0; i < 5 && idx < max_len - 1; i++) {
            frac *= 10.0f;
            int8_t d = (int8_t)frac;
            buf[idx++] = (char)('0' + d);
            frac -= (float)d;
            if (frac < 0.0001f) break;
        }
    }
    buf[idx] = '\0';
    return idx;
}
OVSTUB(1, format_float);

OV2_ENTRY static void uint_to_str_ov(uint16_t n, char* buf, uint8_t max_len) {
    char tmp[6];
    uint8_t i = 0;
    if (n == 0) tmp[i++] = '0';
    else {
        uint32_t v = n;
        while (v > 0 && i < 5) {
            tmp[i++] = (char)('0' + u32_div10(&v));
        }
    }
    uint8_t j = 0;
    while (i > 0 && j < max_len - 1) buf[j++] = tmp[--i];
    buf[j] = '\0';
}
OVSTUB(1, uint_to_str);

OV2_ENTRY static uint8_t* find_line_ov(uint16_t target) {
    uint8_t* p = prog_buf;
    while (p < prog_buf + PROG_BUF_SIZE - 2) {
        uint16_t num = (uint16_t)(p[0] | (p[1] << 8));
        if (num == 0xFFFF || num >= target) return p;
        p += 4 + p[2];
    }
    return nullptr;
}
OVSTUB(1, find_line);

OV2 static void delete_line_ov(uint16_t num) {
    uint8_t* p = find_line_ov(num);
    if (!p) return;
    uint16_t cur = (uint16_t)(p[0] | (p[1] << 8));
    if (cur != num) return;
    uint8_t len = p[2];
    uint8_t* next = p + 4 + len;
    uint8_t* end = prog_buf;
    while (end < prog_buf + PROG_BUF_SIZE - 2) {
        uint16_t n = (uint16_t)(end[0] | (end[1] << 8));
        if (n == 0xFFFF) break;
        end += 4 + end[2];
    }
    size_t move_len = (size_t)(end - next) + 2;
    for (size_t i = 0; i < move_len; i++) p[i] = next[i];
}

OV2_ENTRY static bool insert_line_ov(uint16_t num, const char* text) {
    delete_line_ov(num);
    skip_spaces(&text);
    if (*text == '\0') return true;
    size_t tlen = strlen(text);
    if (tlen > 120) tlen = 120;
    uint8_t* p = find_line_ov(num);
    if (!p) return false;
    uint8_t* end = prog_buf;
    while (end < prog_buf + PROG_BUF_SIZE - 2) {
        uint16_t n = (uint16_t)(end[0] | (end[1] << 8));
        if (n == 0xFFFF) break;
        end += 4 + end[2];
    }
    size_t needed = 4 + tlen;
    if ((size_t)(end - prog_buf) + needed >= PROG_BUF_SIZE - 2) {
        err_oom();
        return false;
    }
    size_t move_len = (size_t)(end - p) + 2;
    for (size_t i = move_len; i > 0; i--) {
        p[needed + i - 1] = p[i - 1];
    }
    p[0] = (uint8_t)(num & 0xFF);
    p[1] = (uint8_t)(num >> 8);
    p[2] = (uint8_t)tlen;
    memcpy(p + 3, text, tlen);
    p[3 + tlen] = '\0';
    return true;
}
OVSTUB(1, insert_line);

OV2_ENTRY static void clear_vars_ov(void) {
    memset(vars, 0, sizeof(vars));
    memset(vars_ext, 0, sizeof(vars_ext));
    memset(str_vars, 0, sizeof(str_vars));
    for (uint8_t i = 0; i < 26; i++) {
        for (uint8_t j = 0; j < 10; j++) {
            if (str_vars_ext[i][j]) {
                os_free(str_vars_ext[i][j]);
                str_vars_ext[i][j] = nullptr;
            }
        }
    }
    for (uint8_t i = 0; i < MAX_ARRAYS; i++) {
        if (arrays[i].name[0]) {
            if (!arrays[i].is_str && arrays[i].num_data) os_free(arrays[i].num_data);
            if (arrays[i].is_str && arrays[i].str_data) os_free(arrays[i].str_data);
            arrays[i].name[0] = '\0';
        }
    }
    gosub_sp = 0;
    for_sp = 0;
    data_line_ptr = nullptr;
    data_item_ptr = nullptr;
}
OVSTUB(1, clear_vars);

OV2_ENTRY static bool parse_var_name_ov(const char** p, VarRef* ref) {
    skip_spaces(p);
    char c = **p;
    if (!is_alpha(c)) return false;
    ref->base = to_upper(c);
    ref->ext = '\0';
    ref->is_str = false;
    (*p)++;
    if (is_alnum(**p)) {
        ref->ext = **p;
        (*p)++;
    }
    if (**p == '$') {
        ref->is_str = true;
        (*p)++;
    }
    return true;
}
OVSTUB(1, parse_var_name);

OV2_ENTRY static struct Array* find_array_ov(const char* name) {
    for (uint8_t i = 0; i < MAX_ARRAYS; i++) {
        const char* a = arrays[i].name;
        if (a[0] == name[0] && a[1] == name[1] && a[2] == name[2] && a[3] == name[3]) {
            return &arrays[i];
        }
    }
    return nullptr;
}
OVSTUB(1, find_array);

OV2_ENTRY static float* num_var_ptr_ov(const VarRef* ref) {
    uint8_t b = (uint8_t)(ref->base - 'A');
    if (b >= 26) return &vars[0];
    if (ref->ext >= '0' && ref->ext <= '9') return &vars_ext[b][ref->ext - '0'];
    return &vars[b];
}
OVSTUB(1, num_var_ptr);

OV2_ENTRY static const char* get_str_var_ov(const VarRef* ref) {
    uint8_t b = (uint8_t)(ref->base - 'A');
    if (b >= 26) return "";
    if (ref->ext >= '0' && ref->ext <= '9') {
        uint8_t d = (uint8_t)(ref->ext - '0');
        return str_vars_ext[b][d] ? str_vars_ext[b][d] : "";
    }
    return str_vars[b];
}
OVSTUB(1, get_str_var);

OV2_ENTRY static void set_str_var_ov(const VarRef* ref, const char* val, uint8_t len) {
    uint8_t b = (uint8_t)(ref->base - 'A');
    if (b >= 26) return;
    if (len >= STR_LEN) len = STR_LEN - 1;
    char* dst;
    if (ref->ext >= '0' && ref->ext <= '9') {
        uint8_t d = (uint8_t)(ref->ext - '0');
        if (!str_vars_ext[b][d]) {
            str_vars_ext[b][d] = (char*)os_alloc(STR_LEN);
            if (!str_vars_ext[b][d]) return;
        }
        dst = str_vars_ext[b][d];
    } else {
        dst = str_vars[b];
    }
    memcpy(dst, val, len);
    dst[len] = '\0';
}
OVSTUB(1, set_str_var);

static float basic_atof_ov(const char* s);
OVSTUB(0, basic_atof);
static Value eval_expr_ov(const char** p);
OVSTUB(0, eval_expr);

OV2 static void find_next_data_ov(void) {
    uint8_t* p = data_line_ptr ? data_line_ptr : prog_buf;
    if (data_line_ptr && data_item_ptr) p += 4 + p[2];
    while (p < prog_buf + PROG_BUF_SIZE - 2) {
        uint16_t lnum = (uint16_t)(p[0] | (p[1] << 8));
        if (lnum == 0xFFFF) break;
        const char* s = (const char*)(p + 3);
        bool in_q = false;
        while (*s) {
            if (*s == '"') in_q = !in_q;
            else if (!in_q && match_kw(&s, "DATA")) {
                data_line_ptr = p;
                data_item_ptr = s;
                return;
            }
            s++;
        }
        p += 4 + p[2];
    }
    data_line_ptr = nullptr;
    data_item_ptr = nullptr;
}

OV2_ENTRY static void exec_read_ov(const char** p) {
    while (**p && **p != ':') {
        VarRef ref;
        if (!parse_var_name_ov(p, &ref)) break;
        if (!data_item_ptr) find_next_data_ov();
        if (!data_item_ptr) {
            err_fail("OUT OF DATA");
            return;
        }
        skip_spaces(&data_item_ptr);
        if (ref.is_str) {
            char val_str[STR_LEN];
            uint8_t idx = 0;
            if (*data_item_ptr == '"') {
                data_item_ptr++;
                while (*data_item_ptr && *data_item_ptr != '"' && idx < STR_LEN - 1) val_str[idx++] = *data_item_ptr++;
                if (*data_item_ptr == '"') data_item_ptr++;
            } else {
                while (*data_item_ptr && *data_item_ptr != ',' && *data_item_ptr != ':' && idx < STR_LEN - 1) val_str[idx++] = *data_item_ptr++;
            }
            val_str[idx] = '\0';
            set_str_var_ov(&ref, val_str, idx);
        } else {
            *num_var_ptr_ov(&ref) = basic_atof(data_item_ptr);
            while (*data_item_ptr && *data_item_ptr != ',' && *data_item_ptr != ':') data_item_ptr++;
        }
        skip_spaces(&data_item_ptr);
        if (*data_item_ptr == ',') data_item_ptr++;
        else data_item_ptr = nullptr;
        skip_spaces(p);
        if (**p == ',') (*p)++;
        else break;
    }
}
OVSTUB(1, exec_read);

OV2_ENTRY static void exec_dim_ov(const char** p) {
    VarRef ref;
    if (!parse_var_name_ov(p, &ref)) {
        err_syntax();
        return;
    }
    float d = eval_paren_num(p);
    uint8_t dim = (uint8_t)d;
    char aname[4] = { ref.base, ref.ext ? ref.ext : '\0', ref.is_str ? '$' : '\0', '\0' };
    if (find_array_ov(aname)) {
        err_fail("REDIM'D ARRAY");
        return;
    }
    for (uint8_t i = 0; i < MAX_ARRAYS; i++) {
        if (!arrays[i].name[0]) {
            memcpy(arrays[i].name, aname, 4);
            arrays[i].is_str = ref.is_str;
            arrays[i].dim = dim;
            if (ref.is_str) {
                arrays[i].str_data = (char (*)[STR_LEN])os_alloc((uint16_t)((dim + 1) * STR_LEN));
                if (arrays[i].str_data) memset(arrays[i].str_data, 0, (dim + 1) * STR_LEN);
            } else {
                arrays[i].num_data = (float*)os_alloc((uint16_t)((dim + 1) * sizeof(float)));
                if (arrays[i].num_data) memset(arrays[i].num_data, 0, (dim + 1) * sizeof(float));
            }
            return;
        }
    }
    err_oom();
}
OVSTUB(1, exec_dim);

OV2_ENTRY static void exec_poke_ov(const char** p) {
    Value a = eval_expr(p);
    skip_paren(p, ',');
    Value v = eval_expr(p);
    *(volatile uint8_t*)(uint16_t)a.num = (uint8_t)v.num;
}
OVSTUB(1, exec_poke);

OV2_ENTRY static void exec_wait_ov(const char** p) {
    Value a = eval_expr(p);
    skip_paren(p, ',');
    Value m = eval_expr(p);
    uint8_t xor_v = 0;
    skip_spaces(p);
    if (**p == ',') {
        (*p)++;
        xor_v = (uint8_t)eval_expr(p).num;
    }
    uint16_t addr = (uint16_t)a.num;
    uint8_t mask = (uint8_t)m.num;
    while (((*(volatile uint8_t*)addr ^ xor_v) & mask) == 0) {
        if (bios_getchar_nb() == 3) {
            interrupted = true;
            program_running = false;
            return;
        }
    }
}
OVSTUB(1, exec_wait);

/* =========================================================================
 * OVERLAY 1: Expression Parser & Evaluator
 * ========================================================================= */

OV1 static float basic_strtof_ov(const char* s, const char** endptr) {
    while (*s == ' ' || *s == '\t') s++;
    bool neg = false;
    if (*s == '-') { neg = true; s++; }
    else if (*s == '+') { s++; }
    float val = 0.0f;
    while (*s >= '0' && *s <= '9') {
        val = val * 10.0f + (float)(*s - '0');
        s++;
    }
    if (*s == '.') {
        s++;
        float div = 10.0f;
        while (*s >= '0' && *s <= '9') {
            val += (float)(*s - '0') / div;
            div *= 10.0f;
            s++;
        }
    }
    if (*s == 'e' || *s == 'E') {
        s++;
        bool exp_neg = false;
        if (*s == '-') { exp_neg = true; s++; }
        else if (*s == '+') { s++; }
        int16_t exp = 0;
        while (*s >= '0' && *s <= '9') {
            exp = exp * 10 + (*s - '0');
            s++;
        }
        while (exp-- > 0) val = exp_neg ? (val * 0.1f) : (val * 10.0f);
    }
    if (endptr) *endptr = s;
    return neg ? -val : val;
}

OV1_ENTRY static float basic_atof_ov(const char* s) {
    return basic_strtof_ov(s, nullptr);
}

OV1 static Value eval_paren_expr_ov(const char** p) {
    skip_paren(p, '(');
    Value v = eval_expr_ov(p);
    skip_paren(p, ')');
    return v;
}

OV1_ENTRY static float eval_paren_num_ov(const char** p) {
    return eval_paren_expr_ov(p).num;
}

OV1 static Value eval_factor_ov(const char** p) {
    skip_spaces(p);
    if (**p == '"') {
        (*p)++;
        const char* start = *p;
        while (**p && **p != '"') (*p)++;
        uint8_t slen = (uint8_t)(*p - start);
        if (**p == '"') (*p)++;
        return make_str_ref(start, slen);
    }

    if (**p == '(') {
        return eval_paren_expr_ov(p);
    }

    if (is_digit(**p) || **p == '.') {
        const char* end_ptr;
        float num = basic_strtof_ov(*p, &end_ptr);
        *p = end_ptr;
        return make_num(num);
    }

    if (match_kw(p, "PEEK")) {
        uint16_t addr = (uint16_t)eval_paren_num_ov(p);
        return make_num((float)(*(volatile uint8_t*)addr));
    }

    Value math_val;
    if (check_math_fn(p, &math_val)) {
        return math_val;
    }

    if (match_kw(p, "RND")) {
        float arg = eval_paren_num_ov(p);
        if (arg < 0.0f) {
            rnd_state = (uint16_t)(-(int16_t)arg) | 1u;
        }
        if (arg != 0.0f) {
            uint16_t x = rnd_state;
            x ^= (uint16_t)(x << 7);
            x ^= (uint16_t)(x >> 9);
            x ^= (uint16_t)(x << 8);
            rnd_state = x;
            last_rnd = (float)(x & 0x7FFFu) * (1.0f / 32768.0f);
        }
        return make_num(last_rnd);
    }

    if (match_kw(p, "LEN")) {
        Value arg = eval_paren_expr_ov(p);
        return make_num((float)arg.len);
    }

    if (match_kw(p, "STR$")) {
        float n = eval_paren_num_ov(p);
        char buf[20];
        uint8_t len = format_float(n, buf, sizeof(buf));
        return make_str_tmp(buf, len);
    }

    if (match_kw(p, "VAL")) {
        Value arg = eval_paren_expr_ov(p);
        char buf[32];
        uint8_t l = arg.len < 31 ? arg.len : 31;
        memcpy(buf, arg.str, l);
        buf[l] = '\0';
        return make_num(basic_strtof_ov(buf, nullptr));
    }

    if (match_kw(p, "CHR$")) {
        float n = eval_paren_num_ov(p);
        char c = (char)(int16_t)n;
        return make_str_tmp(&c, 1);
    }

    if (match_kw(p, "ASC")) {
        Value arg = eval_paren_expr_ov(p);
        return make_num((arg.len > 0) ? (float)(unsigned char)arg.str[0] : 0.0f);
    }

    if (match_kw(p, "LEFT$") || match_kw(p, "RIGHT$")) {
        bool is_left = (*(*p - 6) == 'L' || *(*p - 6) == 'l');
        skip_paren(p, '(');
        Value s = eval_expr_ov(p);
        skip_paren(p, ',');
        int16_t len = (int16_t)eval_expr_ov(p).num;
        skip_paren(p, ')');
        if (len < 0) len = 0;
        if (len > s.len) len = s.len;
        const char* start = is_left ? s.str : (s.str + s.len - len);
        return make_str_tmp(start, (uint8_t)len);
    }

    if (match_kw(p, "MID$")) {
        skip_paren(p, '(');
        Value s = eval_expr_ov(p);
        skip_paren(p, ',');
        int16_t start = (int16_t)eval_expr_ov(p).num - 1;
        int16_t len = s.len;
        skip_spaces(p);
        if (**p == ',') {
            (*p)++;
            len = (int16_t)eval_expr_ov(p).num;
        }
        skip_paren(p, ')');
        if (start < 0) start = 0;
        if (start >= s.len || len <= 0) return make_str_ref("", 0);
        if (start + len > s.len) len = s.len - start;
        return make_str_tmp(s.str + start, (uint8_t)len);
    }

    if (match_kw(p, "FRE")) {
        eval_paren_expr_ov(p);
        return make_num(3000.0f);
    }

    if (match_kw(p, "POS")) {
        eval_paren_expr_ov(p);
        return make_num((float)current_col);
    }

    VarRef ref;
    const char* save_p = *p;
    if (parse_var_name(p, &ref)) {
        skip_spaces(p);
        if (**p == '(') {
            (*p)++;
            float idx = eval_expr_ov(p).num;
            skip_paren(p, ')');
            char aname[4] = { ref.base, ref.ext ? ref.ext : '\0', ref.is_str ? '$' : '\0', '\0' };
            struct Array* arr = find_array(aname);
            if (!arr || (uint8_t)idx > arr->dim) {
                err_fail("BAD SUBSCRIPT");
                return make_num(0.0f);
            }
            if (arr->is_str) {
                const char* s = arr->str_data[(uint8_t)idx];
                return make_str_ref(s, (uint8_t)strlen(s));
            }
            return make_num(arr->num_data[(uint8_t)idx]);
        }
        if (ref.is_str) {
            const char* s = get_str_var(&ref);
            return make_str_ref(s, (uint8_t)strlen(s));
        }
        return make_num(*num_var_ptr(&ref));
    }

    *p = save_p;
    err_syntax();
    return make_num(0.0f);
}

OV1 static Value eval_unary_ov(const char** p) {
    skip_spaces(p);
    if (**p == '+') { (*p)++; return eval_unary_ov(p); }
    if (**p == '-') { (*p)++; Value v = eval_unary_ov(p); v.num = -v.num; return v; }
    return eval_factor_ov(p);
}

OV1 static Value eval_power_ov(const char** p) {
    Value a = eval_unary_ov(p);
    skip_spaces(p);
    while (**p == '^') {
        (*p)++;
        Value b = eval_unary_ov(p);
        a.num = basic_pow(a.num, b.num);
        skip_spaces(p);
    }
    return a;
}

OV1 static Value eval_mul_div_ov(const char** p) {
    Value a = eval_power_ov(p);
    skip_spaces(p);
    while (**p == '*' || **p == '/') {
        char op = **p;
        (*p)++;
        Value b = eval_power_ov(p);
        if (op == '*') a.num *= b.num;
        else {
            if (b.num == 0.0f) {
                err_fail("DIVISION BY ZERO");
                return make_num(0.0f);
            }
            a.num /= b.num;
        }
        skip_spaces(p);
    }
    return a;
}

OV1 static Value eval_add_sub_ov(const char** p) {
    Value a = eval_mul_div_ov(p);
    skip_spaces(p);
    while (**p == '+' || **p == '-') {
        char op = **p;
        (*p)++;
        Value b = eval_mul_div_ov(p);
        if (op == '+') {
            if (a.type == VAL_STR || b.type == VAL_STR) {
                char buf[STR_LEN];
                uint8_t l1 = a.len < STR_LEN - 1 ? a.len : STR_LEN - 1;
                memcpy(buf, a.str, l1);
                uint8_t rem = STR_LEN - 1 - l1;
                uint8_t l2 = b.len < rem ? b.len : rem;
                memcpy(buf + l1, b.str, l2);
                buf[l1 + l2] = '\0';
                a = make_str_tmp(buf, l1 + l2);
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

OV1 static Value eval_rel_ov(const char** p) {
    Value a = eval_add_sub_ov(p);
    skip_spaces(p);
    while (1) {
        uint8_t op = 0;
        if ((*p)[0] == '<' && (*p)[1] == '=') { op = 1; *p += 2; }
        else if ((*p)[0] == '>' && (*p)[1] == '=') { op = 2; *p += 2; }
        else if ((*p)[0] == '<' && (*p)[1] == '>') { op = 3; *p += 2; }
        else if ((*p)[0] == '<') { op = 4; (*p)++; }
        else if ((*p)[0] == '>') { op = 5; (*p)++; }
        else if ((*p)[0] == '=') { op = 6; (*p)++; }
        else break;

        Value b = eval_add_sub_ov(p);
        bool res = false;
        if (a.type == VAL_STR && b.type == VAL_STR) {
            uint8_t min_l = a.len < b.len ? a.len : b.len;
            int8_t c = 0;
            for (uint8_t i = 0; i < min_l; i++) {
                if (a.str[i] != b.str[i]) {
                    c = ((unsigned char)a.str[i] < (unsigned char)b.str[i]) ? -1 : 1;
                    break;
                }
            }
            if (c == 0 && a.len != b.len) c = (a.len < b.len) ? -1 : 1;
            if (op == 1) res = (c <= 0);
            else if (op == 2) res = (c >= 0);
            else if (op == 3) res = (c != 0);
            else if (op == 4) res = (c < 0);
            else if (op == 5) res = (c > 0);
            else if (op == 6) res = (c == 0);
        } else {
            if (op == 1) res = (a.num <= b.num);
            else if (op == 2) res = (a.num >= b.num);
            else if (op == 3) res = (a.num != b.num);
            else if (op == 4) res = (a.num < b.num);
            else if (op == 5) res = (a.num > b.num);
            else if (op == 6) res = (a.num == b.num);
        }
        a = make_num(res ? -1.0f : 0.0f);
        skip_spaces(p);
    }
    return a;
}

OV1 static Value eval_not_ov(const char** p) {
    skip_spaces(p);
    if (match_kw(p, "NOT")) {
        Value a = eval_not_ov(p);
        float res = (a.num == 0.0f) ? -1.0f : 0.0f;
        if (a.num != 0.0f && a.num != -1.0f) res = (float)(~(int16_t)a.num);
        return make_num(res);
    }
    return eval_rel_ov(p);
}

OV1 static Value eval_and_ov(const char** p) {
    Value a = eval_not_ov(p);
    while (match_kw(p, "AND")) {
        Value b = eval_not_ov(p);
        bool ta = (a.num != 0.0f), tb = (b.num != 0.0f);
        if ((a.num == -1.0f || a.num == 0.0f) && (b.num == -1.0f || b.num == 0.0f)) {
            a.num = (ta && tb) ? -1.0f : 0.0f;
        } else {
            a.num = (float)((int16_t)a.num & (int16_t)b.num);
        }
    }
    return a;
}

OV1 static Value eval_or_ov(const char** p) {
    Value a = eval_and_ov(p);
    while (match_kw(p, "OR")) {
        Value b = eval_and_ov(p);
        bool ta = (a.num != 0.0f), tb = (b.num != 0.0f);
        if ((a.num == -1.0f || a.num == 0.0f) && (b.num == -1.0f || b.num == 0.0f)) {
            a.num = (ta || tb) ? -1.0f : 0.0f;
        } else {
            a.num = (float)((int16_t)a.num | (int16_t)b.num);
        }
    }
    return a;
}

OV1_ENTRY static Value eval_expr_ov(const char** p) {
    return eval_or_ov(p);
}

/* =========================================================================
 * OVERLAY 3: Core Statements (PRINT, LET, FOR, NEXT, INPUT) & REPL Loop
 * ========================================================================= */

static void goto_line_ov(uint16_t target);
OVSTUB(3, goto_line);
static void execute_statement_ov(const char** p);
OVSTUB(3, execute_statement);

OV3 static void read_line_raw(char* buf, size_t max) {
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

OV3_ENTRY static void exec_print_ov(const char** p) {
    skip_spaces(p);
    if (**p == '\0' || **p == ':') {
        println("");
        return;
    }
    bool trailing = false;
    while (**p && **p != ':') {
        skip_spaces(p);
        if (**p == ';') { (*p)++; trailing = true; continue; }
        if (**p == ',') {
            (*p)++;
            uint8_t next_tab = 14;
            while (next_tab <= current_col) next_tab += 14;
            while (current_col < next_tab) out_char(' ');
            trailing = true;
            continue;
        }
        if (match_kw(p, "TAB")) {
            uint8_t t = (uint8_t)eval_paren_num(p);
            while (current_col < t) out_char(' ');
            trailing = true;
            continue;
        }
        if (match_kw(p, "SPC")) {
            uint8_t s = (uint8_t)eval_paren_num(p);
            for (uint8_t i = 0; i < s; i++) out_char(' ');
            trailing = true;
            continue;
        }
        Value v = eval_expr(p);
        if (runtime_error) return;
        if (v.type == VAL_STR) {
            for (uint8_t i = 0; i < v.len; i++) out_char(v.str[i]);
        } else {
            char buf[20];
            format_float(v.num, buf, sizeof(buf));
            out_str(buf);
        }
        trailing = false;
    }
    if (!trailing) println("");
}
OVSTUB(2, exec_print);

OV3_ENTRY static void exec_let_ov(const char** p) {
    VarRef ref;
    if (!parse_var_name(p, &ref)) {
        err_syntax();
        return;
    }
    skip_spaces(p);
    if (**p == '(') {
        (*p)++;
        float idx = eval_expr(p).num;
        skip_paren(p, ')');
        skip_spaces(p);
        if (**p == '=') (*p)++;
        Value v = eval_expr(p);
        char aname[4] = { ref.base, ref.ext ? ref.ext : '\0', ref.is_str ? '$' : '\0', '\0' };
        struct Array* arr = find_array(aname);
        if (!arr || (uint8_t)idx > arr->dim) {
            err_fail("BAD SUBSCRIPT");
            return;
        }
        if (arr->is_str) {
            uint8_t l = v.len < STR_LEN - 1 ? v.len : STR_LEN - 1;
            memcpy(arr->str_data[(uint8_t)idx], v.str, l);
            arr->str_data[(uint8_t)idx][l] = '\0';
        } else {
            arr->num_data[(uint8_t)idx] = v.num;
        }
        return;
    }
    if (**p == '=') (*p)++;
    Value v = eval_expr(p);
    if (ref.is_str) set_str_var(&ref, v.str, v.len);
    else *num_var_ptr(&ref) = v.num;
}
OVSTUB(2, exec_let);

OV3_ENTRY static void exec_for_ov(const char** p) {
    VarRef ref;
    if (!parse_var_name(p, &ref) || ref.is_str) {
        err_syntax();
        return;
    }
    skip_spaces(p);
    if (**p == '=') (*p)++;
    float start_val = eval_expr(p).num;
    if (!match_kw(p, "TO")) {
        err_syntax();
        return;
    }
    float end_val = eval_expr(p).num;
    float step_val = 1.0f;
    if (match_kw(p, "STEP")) {
        step_val = eval_expr(p).num;
    }
    *num_var_ptr(&ref) = start_val;
    if (for_sp >= FOR_DEPTH) {
        err_oom();
        return;
    }
    for_stack[for_sp].var = ref;
    for_stack[for_sp].end_val = end_val;
    for_stack[for_sp].step_val = step_val;
    for_stack[for_sp].line_num = current_line_num;
    for_stack[for_sp].loop_stmt = *p;
    for_sp++;
}
OVSTUB(2, exec_for);

OV3_ENTRY static void exec_next_ov(const char** p) {
    VarRef ref;
    bool has_var = parse_var_name(p, &ref);
    if (for_sp == 0) {
        err_fail("NEXT WITHOUT FOR");
        return;
    }
    int8_t slot = (int8_t)(for_sp - 1);
    if (has_var) {
        while (slot >= 0) {
            if (for_stack[slot].var.base == ref.base && for_stack[slot].var.ext == ref.ext) break;
            slot--;
        }
        if (slot < 0) {
            err_fail("NEXT WITHOUT FOR");
            return;
        }
    }
    float* vp = num_var_ptr(&for_stack[slot].var);
    float cur = *vp + for_stack[slot].step_val;
    *vp = cur;
    bool done = (for_stack[slot].step_val >= 0.0f) ? (cur > for_stack[slot].end_val) : (cur < for_stack[slot].end_val);
    if (!done) {
        for_sp = (uint8_t)(slot + 1);
        goto_line(for_stack[slot].line_num);
        current_stmt_ptr = for_stack[slot].loop_stmt;
        *p = for_stack[slot].loop_stmt;
    } else {
        for_sp = (uint8_t)slot;
    }
}
OVSTUB(2, exec_next);

OV3_ENTRY static void exec_input_ov(const char** p) {
    skip_spaces(p);
    if (**p == '"') {
        (*p)++;
        while (**p && **p != '"') out_char(*(*p)++);
        if (**p == '"') (*p)++;
        skip_spaces(p);
        if (**p == ';' || **p == ',') (*p)++;
    } else {
        out_str("? ");
    }
    char in_buf[64];
    read_line_raw(in_buf, sizeof(in_buf));
    const char* in_p = in_buf;
    while (**p && **p != ':') {
        VarRef ref;
        if (!parse_var_name(p, &ref)) break;
        skip_spaces(&in_p);
        if (ref.is_str) {
            char val_str[STR_LEN];
            uint8_t idx = 0;
            if (*in_p == '"') {
                in_p++;
                while (*in_p && *in_p != '"' && idx < STR_LEN - 1) val_str[idx++] = *in_p++;
                if (*in_p == '"') in_p++;
            } else {
                while (*in_p && *in_p != ',' && idx < STR_LEN - 1) val_str[idx++] = *in_p++;
            }
            val_str[idx] = '\0';
            set_str_var(&ref, val_str, idx);
        } else {
            *num_var_ptr(&ref) = basic_atof(in_p);
            while (*in_p && *in_p != ',') in_p++;
        }
        skip_spaces(&in_p);
        if (*in_p == ',') in_p++;
        skip_spaces(p);
        if (**p == ',') (*p)++;
        else break;
    }
}
OVSTUB(2, exec_input);

OV3_ENTRY static void repl_loop_ov(void) {
    println("microDOS MSBasic 1.0");
    println("READY.");
    clear_program();
    exit_requested = false;

    char line_buf[128];
    while (!exit_requested) {
        read_line_raw(line_buf, sizeof(line_buf));
        if (exit_requested) break;
        const char* p = line_buf;
        skip_spaces(&p);
        if (*p == '\0') continue;

        if (is_digit(*p)) {
            uint16_t lnum = parse_u16(&p);
            while (*p == ' ') p++;
            insert_line(lnum, p);
        } else {
            runtime_error = false;
            interrupted = false;
            current_line_num = 0;
            current_line_ptr = nullptr;

            while (*p && !exit_requested) {
                skip_spaces(&p);
                if (*p == '\0') break;
                if (*p == ':') {
                    p++;
                    continue;
                }

                execute_statement(&p);
                if (runtime_error || interrupted || exit_requested) break;
            }
            if (!runtime_error && !interrupted && !exit_requested) {
                println("READY.");
            }
        }
    }
}
OVSTUB(2, repl_loop);

/* =========================================================================
 * OVERLAY 4: Statement Dispatcher, Flow Control, File I/O, Sound, LCD
 * ========================================================================= */

OV4_ENTRY static void goto_line_ov(uint16_t target) {
    uint8_t* p = find_line(target);
    if (!p || (uint16_t)(p[0] | (p[1] << 8)) != target) {
        err_fail("UNDEF'D STATEMENT");
        return;
    }
    current_line_ptr = p;
    current_line_num = target;
    current_stmt_ptr = (const char*)(p + 3);
    line_jumped = true;
}

OV4 static void exec_if_ov(const char** p) {
    Value cond = eval_expr(p);
    skip_spaces(p);
    if (!match_kw(p, "THEN") && !match_kw(p, "GOTO")) {
        err_syntax();
        return;
    }
    bool is_true = (cond.type == VAL_NUM) ? (cond.num != 0.0f) : (cond.len > 0);
    if (is_true) {
        skip_spaces(p);
        if (is_digit(**p)) {
            goto_line_ov(parse_u16(p));
        } else {
            execute_statement_ov(p);
        }
    } else {
        while (**p && **p != ':') {
            if (match_kw(p, "ELSE")) {
                execute_statement_ov(p);
                return;
            }
            (*p)++;
        }
    }
}

OV4 static void exec_on_ov(const char** p) {
    Value v = eval_expr(p);
    int16_t idx = (int16_t)v.num;
    bool is_gosub = match_kw(p, "GOSUB");
    if (!is_gosub && !match_kw(p, "GOTO")) {
        err_syntax();
        return;
    }
    int16_t cur_idx = 1;
    while (**p && **p != ':') {
        Value l = eval_expr(p);
        if (cur_idx == idx) {
            if (is_gosub) {
                if (gosub_sp >= GOSUB_DEPTH) {
                    err_oom();
                    return;
                }
                gosub_stack[gosub_sp].line_num = current_line_num;
                gosub_stack[gosub_sp].next_stmt = *p;
                gosub_sp++;
            }
            goto_line_ov((uint16_t)l.num);
            return;
        }
        cur_idx++;
        skip_spaces(p);
        if (**p == ',') (*p)++;
        else break;
    }
}

OV4 static void exec_list_ov(const char** p) {
    skip_spaces(p);
    uint16_t from = 0, to = 0xFFFF;
    if (is_digit(**p)) {
        from = parse_u16(p);
        to = from;
        skip_spaces(p);
        if (**p == '-') {
            (*p)++;
            to = is_digit(**p) ? parse_u16(p) : 0xFFFF;
        }
    }
    uint8_t* cur = prog_buf;
    while (cur < prog_buf + PROG_BUF_SIZE - 2) {
        uint16_t lnum = (uint16_t)(cur[0] | (cur[1] << 8));
        if (lnum == 0xFFFF) break;
        if (lnum >= from && lnum <= to) {
            print_num(lnum);
            out_char(' ');
            out_str((const char*)(cur + 3));
            println("");
        }
        cur += 4 + cur[2];
    }
}

OV4 static void get_filename_ov(const char** p, char* buf, size_t max) {
    skip_spaces(p);
    size_t i = 0;
    if (**p == '"') {
        (*p)++;
        while (**p && **p != '"' && i < max - 1) buf[i++] = *(*p)++;
        if (**p == '"') (*p)++;
    } else {
        while (**p && **p != ' ' && **p != ':' && i < max - 1) buf[i++] = *(*p)++;
    }
    buf[i] = '\0';
}

OV4 static void exec_save_ov(const char** p) {
    char filename[32];
    get_filename_ov(p, filename, sizeof(filename));
    if (!filename[0]) {
        err_fail("MISSING FILENAME");
        return;
    }
    SD_FILE fp;
    if (sd_open(&fp, filename, SD_WRITE | SD_CREATE_ALWAYS) != 0) {
        err_fail("FILE ERROR");
        return;
    }
    uint8_t* cur = prog_buf;
    char num_buf[16];
    while (cur < prog_buf + PROG_BUF_SIZE - 2) {
        uint16_t lnum = (uint16_t)(cur[0] | (cur[1] << 8));
        if (lnum == 0xFFFF) break;
        uint8_t len = cur[2];
        uint_to_str(lnum, num_buf, sizeof(num_buf));
        sd_write(&fp, num_buf, (uint16_t)strlen(num_buf));
        sd_write(&fp, " ", 1);
        sd_write(&fp, cur + 3, len);
        sd_write(&fp, "\r\n", 2);
        cur += 4 + len;
    }
    sd_close(&fp);
    println("OK");
}

OV4 static void exec_load_ov(const char** p) {
    char filename[32];
    get_filename_ov(p, filename, sizeof(filename));
    if (!filename[0]) {
        err_fail("MISSING FILENAME");
        return;
    }
    SD_FILE fp;
    if (sd_open(&fp, filename, SD_READ) != 0) {
        err_fail("FILE NOT FOUND");
        return;
    }
    clear_program();
    clear_vars();
    char line[128];
    uint8_t lidx = 0;
    while (1) {
        int16_t c = sd_getc(&fp);
        if (c < 0) break;
        if (c == '\r') continue;
        if (c == '\n') {
            line[lidx] = '\0';
            const char* lp = line;
            skip_spaces(&lp);
            if (is_digit(*lp)) {
                uint16_t lnum = parse_u16(&lp);
                insert_line(lnum, lp);
            }
            lidx = 0;
            continue;
        }
        if (lidx < sizeof(line) - 1) line[lidx++] = (char)c;
    }
    sd_close(&fp);
    println("OK");
}

OV4 static void exec_beep_ov(const char** p) {
    uint16_t freq = 0x1CD5;
    uint8_t dur = 20;
    skip_spaces(p);
    if (**p != '\0' && **p != ':') {
        Value vf = eval_expr(p);
        freq = (uint16_t)vf.num;
        skip_spaces(p);
        if (**p == ',') {
            (*p)++;
            Value vd = eval_expr(p);
            dur = (uint8_t)vd.num;
        }
    }
    sid_write(0x18, 0x0F);
    sid_write(0x05, 0x00);
    sid_write(0x06, 0xF0);
    sid_write(0x02, 0x00);
    sid_write(0x03, 0x08);
    sid_write(0x00, (uint8_t)(freq & 0xFF));
    sid_write(0x01, (uint8_t)(freq >> 8));
    sid_write(0x04, 0x41);
    for (uint8_t i = 0; i < dur; i++) {
        for (volatile uint16_t d = 0; d < 500; d++);
    }
    sid_write(0x04, 0x40);
}

OV4 static void exec_sound_ov(const char** p) {
    Value vr = eval_expr(p);
    skip_spaces(p);
    if (**p == ',') (*p)++;
    Value vv = eval_expr(p);
    sid_write((uint8_t)vr.num, (uint8_t)vv.num);
}

OV4 static void exec_exit_ov(const char** p) {
    (void)p;
    sid_reset();
    println("");
    exit_requested = true;
    program_running = false;
}

OV4 static void run_program_from(uint8_t* start_line) {
    program_running = true;
    interrupted = false;
    runtime_error = false;
    current_line_ptr = start_line;

    while (program_running && current_line_ptr && !exit_requested) {
        current_line_num = (uint16_t)(current_line_ptr[0] | (current_line_ptr[1] << 8));
        if (current_line_num == 0xFFFF) break;

        if (is_cont) {
            current_stmt_ptr = break_stmt_ptr;
            is_cont = false;
        } else {
            current_stmt_ptr = (const char*)(current_line_ptr + 3);
        }

        while (*current_stmt_ptr && program_running && !exit_requested) {
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

            execute_statement_ov(&current_stmt_ptr);
            if (runtime_error) {
                print_str(" IN ");
                print_num(current_line_num);
                println("");
                program_running = false;
                return;
            }
            if (line_jumped) break;
        }

        if (!line_jumped && program_running) {
            current_line_ptr += 4 + current_line_ptr[2];
        }
    }
    program_running = false;
}

OV4_ENTRY static void execute_statement_ov(const char** p) {
    skip_spaces(p);
    if (**p == '\0') return;

    if (**p == '?' || match_kw(p, "PRINT")) {
        if (**p == '?') (*p)++;
        exec_print(p);
        return;
    }
    if (match_kw(p, "REM") || match_kw(p, "ELSE") || match_kw(p, "DATA")) {
        *p += strlen(*p);
        return;
    }
    if (match_kw(p, "IF")) { exec_if_ov(p); return; }
    if (match_kw(p, "GOTO")) { goto_line_ov((uint16_t)eval_expr(p).num); return; }
    if (match_kw(p, "GOSUB")) {
        Value v = eval_expr(p);
        if (gosub_sp >= GOSUB_DEPTH) {
            err_oom();
            return;
        }
        gosub_stack[gosub_sp].line_num = current_line_num;
        gosub_stack[gosub_sp].next_stmt = *p;
        gosub_sp++;
        goto_line_ov((uint16_t)v.num);
        return;
    }
    if (match_kw(p, "RETURN")) {
        if (gosub_sp == 0) {
            err_fail("RETURN WITHOUT GOSUB");
            return;
        }
        gosub_sp--;
        uint16_t ret_line = gosub_stack[gosub_sp].line_num;
        const char* ret_stmt = gosub_stack[gosub_sp].next_stmt;
        uint8_t* pl = find_line(ret_line);
        if (!pl || (uint16_t)(pl[0] | (pl[1] << 8)) != ret_line) {
            err_fail("UNDEF'D STATEMENT");
            return;
        }
        current_line_ptr = pl;
        current_line_num = ret_line;
        current_stmt_ptr = ret_stmt;
        *p = ret_stmt;
        line_jumped = true;
        return;
    }
    if (match_kw(p, "FOR")) { exec_for(p); return; }
    if (match_kw(p, "NEXT")) { exec_next(p); return; }
    if (match_kw(p, "INPUT")) { exec_input(p); return; }
    if (match_kw(p, "READ")) { exec_read(p); return; }
    if (match_kw(p, "RESTORE")) { data_line_ptr = nullptr; data_item_ptr = nullptr; return; }
    if (match_kw(p, "DIM")) { exec_dim(p); return; }
    if (match_kw(p, "POKE")) { exec_poke(p); return; }
    if (match_kw(p, "WAIT")) { exec_wait(p); return; }
    if (match_kw(p, "ON")) { exec_on_ov(p); return; }
    if (match_kw(p, "END")) { program_running = false; return; }
    if (match_kw(p, "STOP")) {
        break_line_num = current_line_num;
        break_stmt_ptr = *p;
        break_line_ptr = current_line_ptr;
        print_str("BREAK IN ");
        print_num(current_line_num);
        println("");
        program_running = false;
        interrupted = true;
        return;
    }
    if (match_kw(p, "CONT")) {
        if (break_line_num == 0) {
            err_fail("CAN'T CONTINUE");
            return;
        }
        is_cont = true;
        run_program_from(break_line_ptr);
        return;
    }
    if (match_kw(p, "NEW")) { clear_program(); clear_vars(); return; }
    if (match_kw(p, "CLEAR")) { clear_vars(); return; }
    if (match_kw(p, "LIST")) { exec_list_ov(p); return; }
    if (match_kw(p, "RUN")) {
        clear_vars();
        skip_spaces(p);
        uint16_t start = is_digit(**p) ? parse_u16(p) : 0;
        uint8_t* pl = find_line(start);
        if (pl) run_program_from(pl);
        return;
    }
    if (match_kw(p, "LCDINIT")) { lcd_init(); return; }
    if (match_kw(p, "LCDCMD")) {
        Value v = eval_expr(p);
        lcd_instruction((uint8_t)v.num);
        return;
    }
    if (match_kw(p, "LCDPRINT")) {
        Value v = eval_expr(p);
        if (v.type == VAL_STR) {
            for (uint8_t i = 0; i < v.len; i++) lcd_print_char(v.str[i]);
        } else {
            char num_buf[16];
            format_float(v.num, num_buf, sizeof(num_buf));
            const char* s = num_buf;
            while (*s == ' ') s++;
            while (*s) lcd_print_char(*s++);
        }
        return;
    }
    if (match_kw(p, "BEEP")) { exec_beep_ov(p); return; }
    if (match_kw(p, "SOUND")) { exec_sound_ov(p); return; }
    if (match_kw(p, "SAVE")) { exec_save_ov(p); return; }
    if (match_kw(p, "LOAD")) { exec_load_ov(p); return; }
    if (match_kw(p, "EXIT")) { exec_exit_ov(p); return; }

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

    err_syntax();
}

int main(void) {
    repl_loop();
    return 0;
}