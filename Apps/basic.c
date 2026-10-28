#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "Libs/app-bios.h"
#include "Libs/LCD.h"

#define BASIC_PI      3.14159265f
#define PROG_BUF_SIZE 3072
#define MAX_ARRAYS    8
#define GOSUB_DEPTH   16
#define FOR_DEPTH     16
#define STR_LEN       32

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

static char str_tmp_buf[2][STR_LEN];
static uint8_t str_tmp_idx;

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
    long n = (long)x;
    if (x < (float)n) n--;
    return (float)n;
}

static float basic_sqrt(float x) {
    if (x <= 0.0f) return 0.0f;
    float g = (x > 1.0f) ? (x * 0.5f) : 1.0f;
    for (int i = 0; i < 8; i++) g = 0.5f * (g + x / g);
    return g;
}

static float basic_sin(float x) {
    while (x > BASIC_PI) x -= 2.0f * BASIC_PI;
    while (x < -BASIC_PI) x += 2.0f * BASIC_PI;
    float x2 = x * x;
    return x * (1.0f - x2 * (0.16666667f - x2 * (0.00833333f - x2 * 0.00019841f)));
}

static inline float basic_cos(float x) { return basic_sin(x + (BASIC_PI * 0.5f)); }

static inline float basic_tan(float x) {
    float c = basic_cos(x);
    return (c == 0.0f) ? 999999.0f : (basic_sin(x) / c);
}

static float basic_atan(float x) {
    if (x > 1.0f) return (BASIC_PI * 0.5f) - basic_atan(1.0f / x);
    if (x < -1.0f) return -(BASIC_PI * 0.5f) - basic_atan(1.0f / x);
    float x2 = x * x;
    return x * (1.0f - x2 * (0.33333333f - x2 * (0.2f - x2 * 0.14285714f)));
}

static float basic_exp(float x) {
    if (x < -20.0f) return 0.0f;
    if (x > 20.0f) return 1e9f;
    float term = 1.0f, sum = 1.0f;
    for (int i = 1; i <= 10; i++) {
        term *= x / (float)i;
        sum += term;
    }
    return sum;
}

static float basic_log(float x) {
    if (x <= 0.0f) return -999999.0f;
    int k = 0;
    while (x > 2.0f) { x *= 0.5f; k++; }
    while (x < 1.0f) { x *= 2.0f; k--; }
    float z = (x - 1.0f) / (x + 1.0f);
    float z2 = z * z;
    float sum = z * (2.0f + 0.66666667f * z2 + 0.4f * z2 * z2);
    return sum + (float)k * 0.69314718f;
}

static float basic_pow(float x, float y) {
    if (y == 0.0f) return 1.0f;
    if (x == 0.0f) return 0.0f;
    long iy = (long)y;
    if ((float)iy == y && iy > 0 && iy < 16) {
        float r = 1.0f;
        for (long i = 0; i < iy; i++) r *= x;
        return r;
    }
    if (x < 0.0f) return 0.0f;
    return basic_exp(y * basic_log(x));
}

static float basic_strtof(const char* s, const char** endptr) {
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
        int exp = 0;
        while (*s >= '0' && *s <= '9') {
            exp = exp * 10 + (*s - '0');
            s++;
        }
        while (exp-- > 0) val = exp_neg ? (val * 0.1f) : (val * 10.0f);
    }
    if (endptr) *endptr = s;
    return neg ? -val : val;
}

static inline float basic_atof(const char* s) {
    return basic_strtof(s, nullptr);
}

static uint8_t format_float(float f, char* buf, uint8_t max_len) {
    if (max_len < 2) { if (max_len > 0) buf[0] = '\0'; return 0; }
    if (basic_isnan(f)) { strncpy(buf, "NAN", max_len); return 3; }
    if (basic_isinf(f)) {
        const char* inf_s = (f < 0.0f) ? "-INF" : " INF";
        strncpy(buf, inf_s, max_len);
        return (uint8_t)strlen(buf);
    }
    if (f == 0.0f) { strncpy(buf, " 0", max_len); return 2; }

    uint8_t idx = 0;
    if (f < 0.0f) {
        buf[idx++] = '-';
        f = -f;
    } else {
        buf[idx++] = ' ';
    }

    long int_part = (long)f;
    float frac = f - (float)int_part;

    char tmp[10];
    int ti = 0;
    if (int_part == 0) {
        tmp[ti++] = '0';
    } else {
        long n = int_part;
        while (n > 0 && ti < 9) {
            tmp[ti++] = (char)('0' + (n % 10));
            n /= 10;
        }
    }
    while (ti > 0 && idx < max_len - 1) buf[idx++] = tmp[--ti];

    if (frac > 0.0001f && idx < max_len - 2) {
        buf[idx++] = '.';
        for (int i = 0; i < 5 && idx < max_len - 1; i++) {
            frac *= 10.0f;
            int d = (int)frac;
            buf[idx++] = (char)('0' + d);
            frac -= (float)d;
            if (frac < 0.0001f) break;
        }
    }
    buf[idx] = '\0';
    return idx;
}

static void uint_to_str(uint16_t n, char* buf, uint8_t max_len) {
    char tmp[6];
    int i = 0;
    if (n == 0) tmp[i++] = '0';
    else {
        while (n > 0 && i < 5) {
            tmp[i++] = (char)('0' + (n % 10));
            n /= 10;
        }
    }
    uint8_t j = 0;
    while (i > 0 && j < max_len - 1) buf[j++] = tmp[--i];
    buf[j] = '\0';
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

static void out_char(char c) {
    if (c == '\r' || c == '\n') current_col = 0;
    else if (c == '\b') { if (current_col > 0) current_col--; }
    else current_col++;
    bios_putchar(c);
}

static void out_str(const char* s) {
    while (*s) out_char(*s++);
}

static void out_float(float f) {
    char buf[20];
    format_float(f, buf, sizeof(buf));
    out_str(buf);
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

static void clear_program(void) {
    prog_buf[0] = 0xFF;
    prog_buf[1] = 0xFF;
}

static uint8_t* find_line(uint16_t target) {
    uint8_t* p = prog_buf;
    while (p < prog_buf + PROG_BUF_SIZE - 2) {
        uint16_t num = (uint16_t)(p[0] | (p[1] << 8));
        if (num == 0xFFFF || num >= target) return p;
        p += 4 + p[2];
    }
    return nullptr;
}

static void delete_line(uint16_t num) {
    uint8_t* p = find_line(num);
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
    memmove(p, next, move_len);
}

static bool insert_line(uint16_t num, const char* text) {
    delete_line(num);
    skip_spaces(&text);
    if (*text == '\0') return true;
    size_t tlen = strlen(text);
    if (tlen > 120) tlen = 120;
    uint8_t* p = find_line(num);
    if (!p) return false;
    uint8_t* end = prog_buf;
    while (end < prog_buf + PROG_BUF_SIZE - 2) {
        uint16_t n = (uint16_t)(end[0] | (end[1] << 8));
        if (n == 0xFFFF) break;
        end += 4 + end[2];
    }
    size_t needed = 4 + tlen;
    if ((size_t)(end - prog_buf) + needed >= PROG_BUF_SIZE - 2) {
        println("?OUT OF MEMORY");
        return false;
    }
    size_t move_len = (size_t)(end - p) + 2;
    memmove(p + needed, p, move_len);
    p[0] = (uint8_t)(num & 0xFF);
    p[1] = (uint8_t)(num >> 8);
    p[2] = (uint8_t)tlen;
    memcpy(p + 3, text, tlen);
    p[3 + tlen] = '\0';
    return true;
}

static void clear_vars(void) {
    memset(vars, 0, sizeof(vars));
    memset(vars_ext, 0, sizeof(vars_ext));
    memset(str_vars, 0, sizeof(str_vars));
    for (int i = 0; i < 26; i++) {
        for (int j = 0; j < 10; j++) {
            if (str_vars_ext[i][j]) {
                os_free(str_vars_ext[i][j]);
                str_vars_ext[i][j] = nullptr;
            }
        }
    }
    for (int i = 0; i < MAX_ARRAYS; i++) {
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

static bool parse_var_name(const char** p, VarRef* ref) {
    skip_spaces(p);
    char c = **p;
    if (!isalpha((unsigned char)c)) return false;
    ref->base = (char)toupper((unsigned char)c);
    ref->ext = '\0';
    ref->is_str = false;
    (*p)++;
    if (isalnum((unsigned char)**p)) {
        ref->ext = **p;
        (*p)++;
    }
    if (**p == '$') {
        ref->is_str = true;
        (*p)++;
    }
    return true;
}

static Value eval_expr(const char** p);

static Value eval_paren_expr(const char** p) {
    skip_paren(p, '(');
    Value v = eval_expr(p);
    skip_paren(p, ')');
    return v;
}

static float eval_paren_num(const char** p) {
    return eval_paren_expr(p).num;
}

static struct Array* find_array(const char* name) {
    for (int i = 0; i < MAX_ARRAYS; i++) {
        if (strcmp(arrays[i].name, name) == 0) return &arrays[i];
    }
    return nullptr;
}

static float get_num_var(const VarRef* ref) {
    int b = ref->base - 'A';
    if (b < 0 || b >= 26) return 0.0f;
    if (ref->ext >= '0' && ref->ext <= '9') return vars_ext[b][ref->ext - '0'];
    return vars[b];
}

static void set_num_var(const VarRef* ref, float val) {
    int b = ref->base - 'A';
    if (b < 0 || b >= 26) return;
    if (ref->ext >= '0' && ref->ext <= '9') vars_ext[b][ref->ext - '0'] = val;
    else vars[b] = val;
}

static const char* get_str_var(const VarRef* ref) {
    int b = ref->base - 'A';
    if (b < 0 || b >= 26) return "";
    if (ref->ext >= '0' && ref->ext <= '9') {
        int d = ref->ext - '0';
        return str_vars_ext[b][d] ? str_vars_ext[b][d] : "";
    }
    return str_vars[b];
}

static void set_str_var(const VarRef* ref, const char* val, uint8_t len) {
    int b = ref->base - 'A';
    if (b < 0 || b >= 26) return;
    if (len >= STR_LEN) len = STR_LEN - 1;
    if (ref->ext >= '0' && ref->ext <= '9') {
        int d = ref->ext - '0';
        if (!str_vars_ext[b][d]) {
            str_vars_ext[b][d] = (char*)os_alloc(STR_LEN);
            if (!str_vars_ext[b][d]) return;
        }
        memcpy(str_vars_ext[b][d], val, len);
        str_vars_ext[b][d][len] = '\0';
    } else {
        memcpy(str_vars[b], val, len);
        str_vars[b][len] = '\0';
    }
}

static bool check_math_fn(const char** p, Value* out) {
    static const char* const names[] = {
        "ABS", "INT", "SGN", "SQR", "SIN", "COS", "TAN", "ATN", "EXP", "LOG"
    };
    for (uint8_t i = 0; i < 10; i++) {
        if (match_kw(p, names[i])) {
            float x = eval_paren_num(p);
            float r = 0.0f;
            switch (i) {
                case 0: r = basic_fabs(x); break;
                case 1: r = basic_floor(x); break;
                case 2: r = (x > 0.0f) ? 1.0f : ((x < 0.0f) ? -1.0f : 0.0f); break;
                case 3: r = basic_sqrt(x); break;
                case 4: r = basic_sin(x); break;
                case 5: r = basic_cos(x); break;
                case 6: r = basic_tan(x); break;
                case 7: r = basic_atan(x); break;
                case 8: r = basic_exp(x); break;
                case 9: r = basic_log(x); break;
            }
            *out = make_num(r);
            return true;
        }
    }
    return false;
}

static Value eval_factor(const char** p) {
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
        return eval_paren_expr(p);
    }

    if (isdigit((unsigned char)**p) || **p == '.') {
        const char* end_ptr;
        float num = basic_strtof(*p, &end_ptr);
        *p = end_ptr;
        return make_num(num);
    }

    if (match_kw(p, "PEEK")) {
        uint16_t addr = (uint16_t)eval_paren_num(p);
        return make_num((float)(*(volatile uint8_t*)addr));
    }

    Value math_val;
    if (check_math_fn(p, &math_val)) {
        return math_val;
    }

    if (match_kw(p, "RND")) {
        float arg = eval_paren_num(p);
        if (arg < 0.0f) {
            srand((unsigned int)(-(long)arg));
            last_rnd = (float)rand() / (float)RAND_MAX;
        } else if (arg > 0.0f) {
            last_rnd = (float)rand() / (float)RAND_MAX;
        }
        return make_num(last_rnd);
    }

    if (match_kw(p, "LEN")) {
        Value arg = eval_paren_expr(p);
        return make_num((float)arg.len);
    }

    if (match_kw(p, "STR$")) {
        float n = eval_paren_num(p);
        char buf[20];
        uint8_t len = format_float(n, buf, sizeof(buf));
        return make_str_tmp(buf, len);
    }

    if (match_kw(p, "VAL")) {
        Value arg = eval_paren_expr(p);
        char buf[32];
        uint8_t l = arg.len < 31 ? arg.len : 31;
        memcpy(buf, arg.str, l);
        buf[l] = '\0';
        return make_num(basic_atof(buf));
    }

    if (match_kw(p, "CHR$")) {
        float n = eval_paren_num(p);
        char c = (char)(int)n;
        return make_str_tmp(&c, 1);
    }

    if (match_kw(p, "ASC")) {
        Value arg = eval_paren_expr(p);
        return make_num((arg.len > 0) ? (float)(unsigned char)arg.str[0] : 0.0f);
    }

    if (match_kw(p, "LEFT$") || match_kw(p, "RIGHT$")) {
        bool is_left = (*(*p - 6) == 'L' || *(*p - 6) == 'l');
        skip_paren(p, '(');
        Value s = eval_expr(p);
        skip_paren(p, ',');
        float n = eval_expr(p).num;
        skip_paren(p, ')');
        int len = (int)n;
        if (len < 0) len = 0;
        if (len > s.len) len = s.len;
        const char* start = is_left ? s.str : (s.str + s.len - len);
        return make_str_tmp(start, (uint8_t)len);
    }

    if (match_kw(p, "MID$")) {
        skip_paren(p, '(');
        Value s = eval_expr(p);
        skip_paren(p, ',');
        int start = (int)eval_expr(p).num - 1;
        int len = s.len;
        skip_spaces(p);
        if (**p == ',') {
            (*p)++;
            len = (int)eval_expr(p).num;
        }
        skip_paren(p, ')');
        if (start < 0) start = 0;
        if (start >= s.len || len <= 0) return make_str_ref("", 0);
        if (start + len > s.len) len = s.len - start;
        return make_str_tmp(s.str + start, (uint8_t)len);
    }

    if (match_kw(p, "FRE")) {
        eval_paren_expr(p);
        return make_num(3000.0f);
    }

    if (match_kw(p, "POS")) {
        eval_paren_expr(p);
        return make_num((float)current_col);
    }

    VarRef ref;
    const char* save_p = *p;
    if (parse_var_name(p, &ref)) {
        skip_spaces(p);
        if (**p == '(') {
            (*p)++;
            float idx = eval_expr(p).num;
            skip_paren(p, ')');
            char aname[4] = { ref.base, ref.ext ? ref.ext : '\0', ref.is_str ? '$' : '\0', '\0' };
            struct Array* arr = find_array(aname);
            if (!arr || (uint8_t)idx > arr->dim) {
                println("?BAD SUBSCRIPT");
                runtime_error = true;
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
        return make_num(get_num_var(&ref));
    }

    *p = save_p;
    println("?SYNTAX ERROR");
    runtime_error = true;
    return make_num(0.0f);
}

static Value eval_unary(const char** p) {
    skip_spaces(p);
    if (**p == '+') { (*p)++; return eval_unary(p); }
    if (**p == '-') { (*p)++; Value v = eval_unary(p); v.num = -v.num; return v; }
    return eval_factor(p);
}

static Value eval_power(const char** p) {
    Value a = eval_unary(p);
    skip_spaces(p);
    while (**p == '^') {
        (*p)++;
        Value b = eval_unary(p);
        a.num = basic_pow(a.num, b.num);
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
        if (op == '*') a.num *= b.num;
        else {
            if (b.num == 0.0f) {
                println("?DIVISION BY ZERO");
                runtime_error = true;
                return make_num(0.0f);
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

static Value eval_rel(const char** p) {
    Value a = eval_add_sub(p);
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

        Value b = eval_add_sub(p);
        bool res = false;
        if (a.type == VAL_STR && b.type == VAL_STR) {
            int c = strncmp(a.str, b.str, a.len < b.len ? a.len : b.len);
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

static Value eval_not(const char** p) {
    skip_spaces(p);
    if (match_kw(p, "NOT")) {
        Value a = eval_not(p);
        float res = (a.num == 0.0f) ? -1.0f : 0.0f;
        if (a.num != 0.0f && a.num != -1.0f) res = (float)(~(int)a.num);
        return make_num(res);
    }
    return eval_rel(p);
}

static Value eval_and(const char** p) {
    Value a = eval_not(p);
    while (match_kw(p, "AND")) {
        Value b = eval_not(p);
        bool ta = (a.num != 0.0f), tb = (b.num != 0.0f);
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
        bool ta = (a.num != 0.0f), tb = (b.num != 0.0f);
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
    bool trailing = false;
    while (**p && **p != ':') {
        skip_spaces(p);
        if (**p == ';') { (*p)++; trailing = true; continue; }
        if (**p == ',') {
            (*p)++;
            uint8_t next_tab = (uint8_t)(((current_col / 14) + 1) * 14);
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
            out_float(v.num);
        }
        trailing = false;
    }
    if (!trailing) println("");
}

static void exec_let(const char** p) {
    VarRef ref;
    if (!parse_var_name(p, &ref)) {
        println("?SYNTAX ERROR");
        runtime_error = true;
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
            println("?BAD SUBSCRIPT");
            runtime_error = true;
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
    else set_num_var(&ref, v.num);
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

static void execute_statement(const char** p);

static void exec_if(const char** p) {
    Value cond = eval_expr(p);
    skip_spaces(p);
    if (!match_kw(p, "THEN") && !match_kw(p, "GOTO")) {
        println("?SYNTAX ERROR");
        runtime_error = true;
        return;
    }
    bool is_true = (cond.type == VAL_NUM) ? (cond.num != 0.0f) : (cond.len > 0);
    if (is_true) {
        skip_spaces(p);
        if (isdigit((unsigned char)**p)) {
            uint16_t lnum = (uint16_t)strtoul(*p, (char**)p, 10);
            goto_line(lnum);
        } else {
            execute_statement(p);
        }
    } else {
        while (**p && **p != ':') {
            if (match_kw(p, "ELSE")) {
                execute_statement(p);
                return;
            }
            (*p)++;
        }
    }
}

static void exec_for(const char** p) {
    VarRef ref;
    if (!parse_var_name(p, &ref) || ref.is_str) {
        println("?SYNTAX ERROR");
        runtime_error = true;
        return;
    }
    skip_spaces(p);
    if (**p == '=') (*p)++;
    float start_val = eval_expr(p).num;
    if (!match_kw(p, "TO")) {
        println("?SYNTAX ERROR");
        runtime_error = true;
        return;
    }
    float end_val = eval_expr(p).num;
    float step_val = 1.0f;
    if (match_kw(p, "STEP")) {
        step_val = eval_expr(p).num;
    }
    set_num_var(&ref, start_val);
    if (for_sp >= FOR_DEPTH) {
        println("?OUT OF MEMORY");
        runtime_error = true;
        return;
    }
    for_stack[for_sp].var = ref;
    for_stack[for_sp].end_val = end_val;
    for_stack[for_sp].step_val = step_val;
    for_stack[for_sp].line_num = current_line_num;
    for_stack[for_sp].loop_stmt = *p;
    for_sp++;
}

static void exec_next(const char** p) {
    VarRef ref;
    bool has_var = parse_var_name(p, &ref);
    if (for_sp == 0) {
        println("?NEXT WITHOUT FOR");
        runtime_error = true;
        return;
    }
    int slot = for_sp - 1;
    if (has_var) {
        while (slot >= 0) {
            if (for_stack[slot].var.base == ref.base && for_stack[slot].var.ext == ref.ext) break;
            slot--;
        }
        if (slot < 0) {
            println("?NEXT WITHOUT FOR");
            runtime_error = true;
            return;
        }
    }
    float cur = get_num_var(&for_stack[slot].var);
    cur += for_stack[slot].step_val;
    set_num_var(&for_stack[slot].var, cur);
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

static void exec_input(const char** p) {
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
            float f = basic_atof(in_p);
            set_num_var(&ref, f);
            while (*in_p && *in_p != ',') in_p++;
        }
        skip_spaces(&in_p);
        if (*in_p == ',') in_p++;
        skip_spaces(p);
        if (**p == ',') (*p)++;
        else break;
    }
}

static void find_next_data(void) {
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

static void exec_read(const char** p) {
    while (**p && **p != ':') {
        VarRef ref;
        if (!parse_var_name(p, &ref)) break;
        if (!data_item_ptr) find_next_data();
        if (!data_item_ptr) {
            println("?OUT OF DATA");
            runtime_error = true;
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
            set_str_var(&ref, val_str, idx);
        } else {
            float f = basic_atof(data_item_ptr);
            set_num_var(&ref, f);
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

static void exec_dim(const char** p) {
    VarRef ref;
    if (!parse_var_name(p, &ref)) {
        println("?SYNTAX ERROR");
        runtime_error = true;
        return;
    }
    float d = eval_paren_num(p);
    uint8_t dim = (uint8_t)d;
    char aname[4] = { ref.base, ref.ext ? ref.ext : '\0', ref.is_str ? '$' : '\0', '\0' };
    if (find_array(aname)) {
        println("?REDIM'D ARRAY");
        runtime_error = true;
        return;
    }
    for (int i = 0; i < MAX_ARRAYS; i++) {
        if (!arrays[i].name[0]) {
            strncpy(arrays[i].name, aname, 3);
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
    println("?OUT OF MEMORY");
    runtime_error = true;
}

static void exec_poke(const char** p) {
    Value a = eval_expr(p);
    skip_paren(p, ',');
    Value v = eval_expr(p);
    *(volatile uint8_t*)(uint16_t)a.num = (uint8_t)v.num;
}

static void exec_wait(const char** p) {
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

static void run_program_from(uint8_t* start_line);

static void exec_on(const char** p) {
    Value v = eval_expr(p);
    int idx = (int)v.num;
    bool is_gosub = match_kw(p, "GOSUB");
    if (!is_gosub && !match_kw(p, "GOTO")) {
        println("?SYNTAX ERROR");
        runtime_error = true;
        return;
    }
    int cur_idx = 1;
    while (**p && **p != ':') {
        Value l = eval_expr(p);
        if (cur_idx == idx) {
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
            goto_line((uint16_t)l.num);
            return;
        }
        cur_idx++;
        skip_spaces(p);
        if (**p == ',') (*p)++;
        else break;
    }
}

static void exec_list(const char** p) {
    skip_spaces(p);
    uint16_t from = 0, to = 0xFFFF;
    if (isdigit((unsigned char)**p)) {
        from = (uint16_t)strtoul(*p, (char**)p, 10);
        to = from;
        skip_spaces(p);
        if (**p == '-') {
            (*p)++;
            to = isdigit((unsigned char)**p) ? (uint16_t)strtoul(*p, (char**)p, 10) : 0xFFFF;
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

static void get_filename(const char** p, char* buf, size_t max) {
    skip_spaces(p);
    if (**p == '"') {
        (*p)++;
        size_t i = 0;
        while (**p && **p != '"' && i < max - 1) buf[i++] = *(*p)++;
        if (**p == '"') (*p)++;
        buf[i] = '\0';
    } else {
        size_t i = 0;
        while (**p && **p != ' ' && **p != ':' && i < max - 1) buf[i++] = *(*p)++;
        buf[i] = '\0';
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
            if (isdigit((unsigned char)*lp)) {
                uint16_t lnum = (uint16_t)strtoul(lp, (char**)&lp, 10);
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

static void exec_beep(const char** p) {
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

static void exec_sound(const char** p) {
    Value vr = eval_expr(p);
    skip_spaces(p);
    if (**p == ',') (*p)++;
    Value vv = eval_expr(p);
    sid_write((uint8_t)vr.num, (uint8_t)vv.num);
}

static void exec_exit(const char** p) {
    (void)p;
    sid_reset();
    println("");
    exit_requested = true;
    program_running = false;
}

static void execute_statement(const char** p) {
    skip_spaces(p);
    if (**p == '\0') return;

    if (**p == '?' || match_kw(p, "PRINT")) {
        if (**p == '?') (*p)++;
        exec_print(p);
        return;
    }
    if (match_kw(p, "REM")) { *p += strlen(*p); return; }
    if (match_kw(p, "IF")) { exec_if(p); return; }
    if (match_kw(p, "ELSE")) { *p += strlen(*p); return; }
    if (match_kw(p, "GOTO")) { goto_line((uint16_t)eval_expr(p).num); return; }
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
    if (match_kw(p, "FOR")) { exec_for(p); return; }
    if (match_kw(p, "NEXT")) { exec_next(p); return; }
    if (match_kw(p, "INPUT")) { exec_input(p); return; }
    if (match_kw(p, "DATA")) { *p += strlen(*p); return; }
    if (match_kw(p, "READ")) { exec_read(p); return; }
    if (match_kw(p, "RESTORE")) { data_line_ptr = nullptr; data_item_ptr = nullptr; return; }
    if (match_kw(p, "DIM")) { exec_dim(p); return; }
    if (match_kw(p, "POKE")) { exec_poke(p); return; }
    if (match_kw(p, "WAIT")) { exec_wait(p); return; }
    if (match_kw(p, "ON")) { exec_on(p); return; }
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
            println("?CAN'T CONTINUE");
            runtime_error = true;
            return;
        }
        is_cont = true;
        run_program_from(break_line_ptr);
        return;
    }
    if (match_kw(p, "NEW")) { clear_program(); clear_vars(); return; }
    if (match_kw(p, "CLEAR")) { clear_vars(); return; }
    if (match_kw(p, "LIST")) { exec_list(p); return; }
    if (match_kw(p, "RUN")) {
        clear_vars();
        skip_spaces(p);
        uint16_t start = isdigit((unsigned char)**p) ? (uint16_t)strtoul(*p, (char**)p, 10) : 0;
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
    if (match_kw(p, "BEEP")) { exec_beep(p); return; }
    if (match_kw(p, "SOUND")) { exec_sound(p); return; }
    if (match_kw(p, "SAVE")) { exec_save(p); return; }
    if (match_kw(p, "LOAD")) { exec_load(p); return; }
    if (match_kw(p, "EXIT")) { exec_exit(p); return; }

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

            execute_statement(&current_stmt_ptr);
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

int main(void) {
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

        if (isdigit((unsigned char)*p)) {
            uint16_t lnum = (uint16_t)strtoul(p, (char**)&p, 10);
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
    return 0;
}
