#ifndef MICRODOS
#define MICRODOS 1
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "Libs/app-bios.h"

MICRODOS_APP_ICON({
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x15, 0x15, 0x15, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x15, 0x3F, 0x3F, 0x15, 0x3F, 0x3F, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x15, 0x3F, 0x00, 0x15, 0x00, 0x3F, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x15, 0x15, 0x3C, 0x3C, 0x3C, 0x15, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x15, 0x15, 0x3F, 0x3C, 0x3C, 0x3C, 0x3F, 0x15, 0x15, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x15, 0x15, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x15, 0x15, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x15, 0x15, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x15, 0x15, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x15, 0x15, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x15, 0x15, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x15, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x15, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x15, 0x15, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x15, 0x15, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x15, 0x15, 0x3F, 0x3F, 0x3F, 0x15, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3C, 0x3C, 0x3C, 0x15, 0x15, 0x15, 0x3C, 0x3C, 0x3C, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x3C, 0x3C, 0x3C, 0x3C, 0x00, 0x00, 0x00, 0x3C, 0x3C, 0x3C, 0x3C, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
});

#define OV1       __attribute__((section(".overlay_1"), noinline))
#define OV2       __attribute__((section(".overlay_2"), noinline))
#define OV3       __attribute__((section(".overlay_3"), noinline))
#define OV4       __attribute__((section(".overlay_4"), noinline))
#define OV5       __attribute__((section(".overlay_5"), noinline))
#define OV6       __attribute__((section(".overlay_6"), noinline))
#define OV7       __attribute__((section(".overlay_7"), noinline))
#define OV8       __attribute__((section(".overlay_8"), noinline))

#define OV1_ENTRY __attribute__((section(".overlay_1"), used, noinline))
#define OV2_ENTRY __attribute__((section(".overlay_2"), used, noinline))
#define OV3_ENTRY __attribute__((section(".overlay_3"), used, noinline))
#define OV4_ENTRY __attribute__((section(".overlay_4"), used, noinline))
#define OV5_ENTRY __attribute__((section(".overlay_5"), used, noinline))
#define OV6_ENTRY __attribute__((section(".overlay_6"), used, noinline))
#define OV7_ENTRY __attribute__((section(".overlay_7"), used, noinline))
#define OV8_ENTRY __attribute__((section(".overlay_8"), used, noinline))

#define OV1_S(s) ({ static const char _s[] __attribute__((section(".overlay_1.rodata"))) = (s); _s; })
#define OV2_S(s) ({ static const char _s[] __attribute__((section(".overlay_2.rodata"))) = (s); _s; })
#define OV3_S(s) ({ static const char _s[] __attribute__((section(".overlay_3.rodata"))) = (s); _s; })
#define OV4_S(s) ({ static const char _s[] __attribute__((section(".overlay_4.rodata"))) = (s); _s; })
#define OV5_S(s) ({ static const char _s[] __attribute__((section(".overlay_5.rodata"))) = (s); _s; })
#define OV6_S(s) ({ static const char _s[] __attribute__((section(".overlay_6.rodata"))) = (s); _s; })
#define OV7_S(s) ({ static const char _s[] __attribute__((section(".overlay_7.rodata"))) = (s); _s; })
#define OV8_S(s) ({ static const char _s[] __attribute__((section(".overlay_8.rodata"))) = (s); _s; })

#define OVSTUB(page, name) typeof(name##_ov) name; \
    __asm__(".pushsection .text." #name ",\"ax\"\n.globl " #name "\n" #name ":\n" \
            "\tjsr overlay_dispatcher\n\t.byte " #page "\n\t.word " #name "_ov\n.popsection")

#define ACIA_DATA_REG   (*(volatile uint8_t*)0x7FC0)
#define ACIA_STAT_REG   (*(volatile uint8_t*)0x7FC1)
#define ACIA_CMD_REG    (*(volatile uint8_t*)0x7FC2)
#define ACIA_CTRL_REG   (*(volatile uint8_t*)0x7FC3)
#define SD_SPI_CTRL     (*(volatile uint8_t*)0x7FC8)

enum {
    S_IFMT   = 0170000,
    S_IFSOCK = 0140000,
    S_IFLNK  = 0120000,
    S_IFREG  = 0100000,
    S_IFBLK  = 0060000,
    S_IFDIR  = 0040000,
    S_IFCHR  = 0020000,
    S_IFIFO  = 0010000,
    S_ISUID  = 0004000,
    S_ISGID  = 0002000,
    S_ISVTX  = 0001000,
    S_IRWXU  = 0000700,
    S_IRUSR  = 0000400,
    S_IWUSR  = 0000200,
    S_IXUSR  = 0000100,
    S_IRWXG  = 0000070,
    S_IRWXOG = 0000077
};

#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define MKDEV(ma, mi) ((uint16_t)(((uint16_t)(ma) << 8) | ((uint8_t)(mi))))
#define MAJOR(dev)    ((uint8_t)((dev) >> 8))
#define MINOR(dev)    ((uint8_t)((dev) & 0xFF))

enum {
    TASK_RUNNING         = 0,
    TASK_INTERRUPTIBLE   = 1,
    TASK_UNINTERRUPTIBLE = 2,
    __TASK_STOPPED       = 4,
    EXIT_ZOMBIE          = 32,
    PF_KTHREAD           = 0x20,
    PF_USED_ASYNC        = 0x40
};

enum {
    KERN_EMERG   = 0,
    KERN_ALERT   = 1,
    KERN_CRIT    = 2,
    KERN_ERR     = 3,
    KERN_WARNING = 4,
    KERN_NOTICE  = 5,
    KERN_INFO    = 6,
    KERN_DEBUG   = 7
};

enum {
    MAX_INODES      = 42,
    RAMFS_BLK_SIZE  = 64,
    RAMFS_NUM_BLKS  = 24,
    RAMFS_POOL_SIZE = 1536,
    LOG_BUF_LEN     = 512,
    MAX_TASKS       = 10,
    MAX_ENV_VARS    = 10,
    ENV_VAR_LEN     = 28,
    HIST_MAX        = 6,
    HIST_LINE_LEN   = 56,
    CMD_LINE_MAX    = 96,
    PIPE_BUF_CAP    = 256,
    MAX_ARGS_SH     = 10
};

struct new_utsname {
    char sysname[10];
    char nodename[18];
    char release[24];
    char version[20];
    char machine[10];
    char domainname[10];
};

struct sysinfo {
    uint32_t uptime;
    uint16_t totalram;
    uint16_t freeram;
    uint16_t sharedram;
    uint16_t bufferram;
    uint16_t totalswap;
    uint16_t freeswap;
    uint16_t procs;
};

struct task_struct {
    uint8_t  pid;
    uint8_t  ppid;
    uint8_t  state;
    uint8_t  flags;
    uint16_t uid;
    int8_t   nice;
    uint8_t  rss_pages;
    uint16_t ticks;
    char     comm[14];
};

struct inode {
    uint8_t  ino;
    uint8_t  parent_ino;
    uint16_t mode;
    uint16_t uid;
    uint16_t gid;
    uint16_t size;
    uint16_t data_off;
    uint16_t rdev;
    char     name[14];
};

enum cpio_state {
    CPIO_Start = 0,
    CPIO_Collect,
    CPIO_GotHeader,
    CPIO_SkipIt,
    CPIO_GotName,
    CPIO_CopyFile,
    CPIO_Reset
};

/* Global Kernel State in extra_ram ($5FC0-$7FBF) */
static struct new_utsname init_uts_ns;
static struct task_struct tasks[MAX_TASKS];
static struct inode       inode_table[MAX_INODES];
static uint8_t            ramfs_pool[RAMFS_POOL_SIZE];
static uint8_t            page_bitmap[4];
static char               log_buf[LOG_BUF_LEN];
static uint16_t           log_next_idx;
static uint8_t            console_loglevel;

static char     env_vars[MAX_ENV_VARS][ENV_VAR_LEN];
static char     cmd_hist[HIST_MAX][HIST_LINE_LEN];
static uint8_t  hist_count;
static char     cmd_line_buf[CMD_LINE_MAX];
static char     pipe_buf[PIPE_BUF_CAP];
static uint16_t pipe_len;
static char     pipe_in_buf[PIPE_BUF_CAP];
static uint16_t pipe_in_len;
static uint16_t pipe_in_pos;

static char     scratch_path[64];
static char     scratch_arg[64];
static char     scratch_buf[128];
static char*    sh_argv[MAX_ARGS_SH];
static uint8_t  sh_argc;

static uint32_t jiffies;
static uint16_t loops_per_jiffy;
static uint16_t bogomips_int;
static uint16_t bogomips_frac;
static uint8_t  prev_t1_hi;

static uint8_t  current_uid;
static uint8_t  current_gid;
static uint8_t  cwd_ino;
static uint8_t  active_pid;
static int16_t  last_exit_code;

static bool     kernel_running;
static bool     boot_quiet;
static bool     boot_single;
static bool     boot_panic_test;
static bool     vt100_bar_enabled;
static uint8_t  redir_mode; /* 0 = console, 1 = pipe_buf */
static bool     stdin_from_pipe;

static uint8_t  mnt_flags; /* bit0=proc, bit1=sys, bit2=dev, bit3=sd */
#define MNT_PROC 0x01U
#define MNT_SYS  0x02U
#define MNT_DEV  0x04U
#define MNT_SD   0x08U

static uint16_t irq_acia_cnt;
static uint16_t irq_via_cnt;
static uint16_t irq_sd_cnt;
static uint16_t prng_state;
static uint16_t unpacked_inodes_cnt;
static uint16_t unpacked_bytes_cnt;

static SD_FILE  lnx_sd_fp;
static SD_DIR   lnx_sd_dir;
static SD_INFO  lnx_sd_fno;

/* =========================================================================
 * FORWARD DECLARATIONS & OVERLAY TRAMPOLINES (OVSTUB 0..6)
 * ========================================================================= */

/* Overlay 1 (page 0): init/main.c, init/calibrate.c, arch/mos6502, kernel/panic.c */
static void start_kernel_ov(void);
OVSTUB(0, start_kernel);
static void panic_ov(const char* msg);
OVSTUB(0, panic);

/* Overlay 2 (page 1): init/initramfs.c (CPIO newc unpacker) & /init runner */
static void populate_rootfs_ov(void);
OVSTUB(1, populate_rootfs);
static void run_init_process_ov(void);
OVSTUB(1, run_init_process);

/* Overlay 3 (page 2): fs/namei.c, fs/ramfs/, drivers/char/mem.c, VFAT /mnt/sd */
static int8_t vfs_path_lookup_ov(const char* path, uint8_t* parent_out, char* leaf_out);
OVSTUB(2, vfs_path_lookup);
static int8_t vfs_create_node_ov(uint8_t parent, const char* name, uint16_t mode, uint16_t rdev);
OVSTUB(2, vfs_create_node);
static int16_t vfs_write_inode_ov(uint8_t ino, const void* data, uint16_t len, bool append);
OVSTUB(2, vfs_write_inode);
static int16_t vfs_read_inode_ov(uint8_t ino, void* buf, uint16_t max_len, uint16_t offset);
OVSTUB(2, vfs_read_inode);
static bool vfs_unlink_path_ov(const char* path, bool recursive);
OVSTUB(2, vfs_unlink_path);
static void vfs_build_cwd_ov(char* out, uint8_t max_len);
OVSTUB(2, vfs_build_cwd);
static int16_t vfs_read_by_path_ov(const char* path, char* out_buf, uint16_t max_len);
OVSTUB(2, vfs_read_by_path);
static bool vfs_write_by_path_ov(const char* path, const char* data, uint16_t len, bool append);
OVSTUB(2, vfs_write_by_path);

/* Overlay 4 (page 3): fs/proc/ & fs/sysfs/ generators */
static bool proc_sys_handle_ov(const char* path, bool is_write, const char* write_val);
OVSTUB(3, proc_sys_handle);
static bool proc_sys_listdir_ov(const char* path, bool long_fmt);
OVSTUB(3, proc_sys_listdir);

/* Overlay 5 (page 4): drivers/tty/n_tty.c & BusyBox /bin/sh shell engine */
static void vt100_draw_bar_ov(void);
OVSTUB(4, vt100_draw_bar);
static void shell_run_loop_ov(void);
OVSTUB(4, shell_run_loop);
static int16_t shell_exec_line_ov(char* line);
OVSTUB(4, shell_exec_line);
static bool shell_run_script_ov(const char* path, bool verbose_rc);
OVSTUB(4, shell_run_script);

/* Overlay 6 (page 5): BusyBox File, Text & Editor Applets */
static int16_t bb_file_applets_ov(void);
OVSTUB(5, bb_file_applets);

/* Overlay 7 (page 6): BusyBox System, Process & Admin Applets */
static int16_t bb_sys_applets_ov(void);
OVSTUB(6, bb_sys_applets);

/* Overlay 8 (page 7): BusyBox Text, Editor, Network & Hardware Applets */
static int16_t bb_ext_applets_ov(void);
OVSTUB(7, bb_ext_applets);

/* =========================================================================
 * RESIDENT CORE HELPERS (core_ram: $0200-$27FF)
 * ========================================================================= */

static void update_jiffies(void) {
    uint8_t hi = VIA_T1C_H;
    if (hi != prev_t1_hi) {
        uint8_t diff = (uint8_t)(prev_t1_hi - hi);
        jiffies += (uint32_t)(diff ? diff : 1U);
        prev_t1_hi = hi;
        irq_via_cnt++;
    } else {
        jiffies++;
    }
}

static uint8_t next_prng_byte(void) {
    uint16_t s = (uint16_t)(prng_state ^ ((uint16_t)VIA_T1C_L << 8) ^ VIA_T2C_L ^ (uint16_t)jiffies);
    if (s == 0U) s = 0xACE1U;
    s ^= (uint16_t)(s << 7);
    s ^= (uint16_t)(s >> 9);
    s ^= (uint16_t)(s << 8);
    prng_state = s;
    return (uint8_t)(s & 0xFFU);
}

static size_t kstrlen(const char* s) {
    size_t n = 0;
    if (!s) return 0;
    while (s[n]) n++;
    return n;
}

static int8_t kstrcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (int8_t)((uint8_t)*a - (uint8_t)*b);
}

static int8_t kstrncmp(const char* a, const char* b, size_t n) {
    while (n > 0 && *a && (*a == *b)) {
        a++;
        b++;
        n--;
    }
    if (n == 0) return 0;
    return (int8_t)((uint8_t)*a - (uint8_t)*b);
}

static void kstrcpy_lim(char* dst, const char* src, size_t max_len) {
    if (max_len == 0) return;
    size_t i = 0;
    while (src && src[i] && i < max_len - 1U) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static char* kpath_copy(const char* ov_str) {
    kstrcpy_lim(scratch_path, ov_str, sizeof(scratch_path));
    return scratch_path;
}

static char* karg_copy(const char* ov_str) {
    kstrcpy_lim(scratch_arg, ov_str, sizeof(scratch_arg));
    return scratch_arg;
}

static void log_store_char(char c) {
    log_buf[log_next_idx & (LOG_BUF_LEN - 1U)] = c;
    log_next_idx++;
}

static void kputc(char c) {
    if (redir_mode == 1U) {
        if (pipe_len < PIPE_BUF_CAP - 1U) {
            pipe_buf[pipe_len++] = c;
            pipe_buf[pipe_len] = '\0';
        }
        return;
    }
    if (c == '\n') {
        bios_putchar('\r');
        bios_putchar('\n');
    } else {
        bios_putchar(c);
    }
}

static void kputs(const char* s) {
    while (s && *s) {
        kputc(*s++);
    }
}

static void kprintln(const char* s) {
    kputs(s);
    kputc('\n');
}

static char kgetc_nb(void) {
    if (stdin_from_pipe) {
        if (pipe_in_pos < pipe_in_len) {
            return pipe_in_buf[pipe_in_pos++];
        }
        return 0;
    }
    char c = bios_getchar_nb();
    if (c != 0) irq_acia_cnt++;
    return c;
}

static uint8_t u32_div10_rem(uint32_t* val) {
    uint32_t q = 0, r = 0, x = *val;
    for (int8_t i = 31; i >= 0; i--) {
        r = (r << 1) | ((x >> i) & 1U);
        if (r >= 10U) {
            r -= 10U;
            q |= (1UL << i);
        }
    }
    *val = q;
    return (uint8_t)r;
}

static uint8_t fmt_u32_dec(uint32_t v, char* buf) {
    char tmp[11];
    uint8_t ti = 0;
    if (v == 0U) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1U;
    }
    while (v > 0U && ti < 10U) {
        tmp[ti++] = (char)('0' + u32_div10_rem(&v));
    }
    uint8_t i = 0;
    while (ti > 0U) {
        buf[i++] = tmp[--ti];
    }
    buf[i] = '\0';
    return i;
}

static void kprint_u32(uint32_t v) {
    char buf[12];
    fmt_u32_dec(v, buf);
    kputs(buf);
}

static void kprint_u16(uint16_t v) {
    kprint_u32((uint32_t)v);
}

static void kprint_hex8(uint8_t v) {
    static const char hc[] = "0123456789abcdef";
    kputc(hc[v >> 4]);
    kputc(hc[v & 0x0FU]);
}

static void kprint_hex16(uint16_t v) {
    kprint_hex8((uint8_t)(v >> 8));
    kprint_hex8((uint8_t)(v & 0xFFU));
}

static void vprintk_emit(uint8_t level, const char* msg) {
    update_jiffies();
    char tbuf[16];
    uint32_t sec = jiffies / 1000UL;
    uint32_t rem = jiffies % 1000UL;
    uint32_t usec = rem * 1000UL + (uint32_t)(0xFFU - VIA_T1C_L) * 3UL;
    if (usec >= 1000000UL) usec = 999999UL;

    log_store_char('[');
    char sbuf[12];
    uint8_t sl = fmt_u32_dec(sec, sbuf);
    for (uint8_t p = sl; p < 5U; p++) log_store_char(' ');
    for (uint8_t p = 0; p < sl; p++) log_store_char(sbuf[p]);
    log_store_char('.');
    uint8_t ul = fmt_u32_dec(usec, tbuf);
    for (uint8_t p = ul; p < 6U; p++) log_store_char('0');
    for (uint8_t p = 0; p < ul; p++) log_store_char(tbuf[p]);
    log_store_char(']');
    log_store_char(' ');
    const char* mp = msg;
    while (mp && *mp) {
        log_store_char(*mp++);
    }
    log_store_char('\n');

    if (boot_quiet && level > KERN_WARNING) {
        return;
    }

    kputs("\x1b[32m[");
    for (uint8_t p = sl; p < 5U; p++) kputc(' ');
    kputs(sbuf);
    kputc('.');
    for (uint8_t p = ul; p < 6U; p++) kputc('0');
    kputs(tbuf);
    kputs("]\x1b[0m ");
    if (level <= KERN_ERR) kputs("\x1b[31;1m");
    else if (level == KERN_WARNING) kputs("\x1b[33m");
    else if (level == KERN_NOTICE) kputs("\x1b[36m");
    kputs(msg);
    if (level <= KERN_NOTICE) kputs("\x1b[0m");
    kputc('\n');
}

static void printk(uint8_t level, const char* msg) {
    vprintk_emit(level, msg);
}

static bool page_test_bit(uint8_t blk) {
    return (page_bitmap[blk >> 3] & (uint8_t)(1U << (blk & 7U))) != 0U;
}

static void page_set_bit(uint8_t blk) {
    page_bitmap[blk >> 3] |= (uint8_t)(1U << (blk & 7U));
}

static void page_clear_bit(uint8_t blk) {
    page_bitmap[blk >> 3] &= (uint8_t)~(1U << (blk & 7U));
}

static uint8_t nr_free_pages(void) {
    uint8_t free_cnt = 0;
    for (uint8_t i = 0; i < RAMFS_NUM_BLKS; i++) {
        if (!page_test_bit(i)) free_cnt++;
    }
    return free_cnt;
}

static uint8_t nr_used_inodes(void) {
    uint8_t cnt = 0;
    for (uint8_t i = 0; i < MAX_INODES; i++) {
        if (inode_table[i].mode != 0U) cnt++;
    }
    return cnt;
}

static void si_meminfo(struct sysinfo* val) {
    update_jiffies();
    val->uptime = jiffies / 100UL;
    val->totalram = (uint16_t)(RAMFS_NUM_BLKS * RAMFS_BLK_SIZE);
    val->freeram = (uint16_t)(nr_free_pages() * RAMFS_BLK_SIZE);
    val->sharedram = 0U;
    val->bufferram = (uint16_t)(nr_used_inodes() * sizeof(struct inode));
    val->totalswap = 8192U;
    val->freeswap = 8192U;
    uint16_t p = 0;
    for (uint8_t i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].pid != 0U || i == 0U) p++;
    }
    val->procs = p;
}

static int16_t ramfs_alloc_contiguous(uint8_t num_blks) {
    if (num_blks == 0U) return 0;
    uint8_t run = 0;
    uint8_t start = 0;
    for (uint8_t i = 0; i < RAMFS_NUM_BLKS; i++) {
        if (!page_test_bit(i)) {
            if (run == 0U) start = i;
            run++;
            if (run == num_blks) {
                for (uint8_t k = start; k < start + num_blks; k++) {
                    page_set_bit(k);
                }
                uint16_t off = (uint16_t)start * RAMFS_BLK_SIZE;
                memset(&ramfs_pool[off], 0, (size_t)num_blks * RAMFS_BLK_SIZE);
                return (int16_t)off;
            }
        } else {
            run = 0U;
        }
    }
    return -1;
}

static void ramfs_free_range(uint16_t data_off, uint16_t size_bytes) {
    if (size_bytes == 0U || data_off >= sizeof(ramfs_pool)) return;
    uint8_t first = (uint8_t)(data_off / RAMFS_BLK_SIZE);
    uint8_t count = (uint8_t)((size_bytes + RAMFS_BLK_SIZE - 1U) / RAMFS_BLK_SIZE);
    for (uint8_t i = 0; i < count && (first + i) < RAMFS_NUM_BLKS; i++) {
        page_clear_bit((uint8_t)(first + i));
    }
}

static const char* env_get(const char* key) {
    size_t klen = kstrlen(key);
    for (uint8_t i = 0; i < MAX_ENV_VARS; i++) {
        if (env_vars[i][0] && kstrncmp(env_vars[i], key, klen) == 0 && env_vars[i][klen] == '=') {
            return &env_vars[i][klen + 1U];
        }
    }
    return nullptr;
}

static void env_set(const char* key, const char* val) {
    size_t klen = kstrlen(key);
    int8_t free_slot = -1;
    for (uint8_t i = 0; i < MAX_ENV_VARS; i++) {
        if (env_vars[i][0] == '\0') {
            if (free_slot < 0) free_slot = (int8_t)i;
        } else if (kstrncmp(env_vars[i], key, klen) == 0 && env_vars[i][klen] == '=') {
            free_slot = (int8_t)i;
            break;
        }
    }
    if (free_slot < 0) free_slot = (int8_t)(MAX_ENV_VARS - 1U);
    char* dst = env_vars[(uint8_t)free_slot];
    uint8_t pos = 0;
    while (key[pos] && pos < ENV_VAR_LEN - 3U) {
        dst[pos] = key[pos];
        pos++;
    }
    dst[pos++] = '=';
    uint8_t vi = 0;
    while (val && val[vi] && pos < ENV_VAR_LEN - 1U) {
        dst[pos++] = val[vi++];
    }
    dst[pos] = '\0';
}

/* =========================================================================
 * OVERLAY 1 (page 0): init/main.c, init/calibrate.c, arch/mos6502, panic.c
 * ========================================================================= */

OV1 static uint8_t arch_get_sp(void) {
    uint8_t sp;
    __asm__ volatile("tsx\n\tstx %0" : "=m"(sp) : : "x");
    return sp;
}

OV1 static uint8_t arch_get_flags(void) {
    uint8_t p;
    __asm__ volatile("php\n\tpla\n\tsta %0" : "=m"(p) : : "a");
    return p;
}

OV1 static void setup_arch_ov(void) {
    kstrcpy_lim(init_uts_ns.sysname, OV1_S("Linux"), sizeof(init_uts_ns.sysname));
    kstrcpy_lim(init_uts_ns.nodename, OV1_S("uclinux-65c02"), sizeof(init_uts_ns.nodename));
    kstrcpy_lim(init_uts_ns.release, OV1_S("6.6.0-mos65c02-nommu"), sizeof(init_uts_ns.release));
    kstrcpy_lim(init_uts_ns.version, OV1_S("#1 PREEMPT NOMMU"), sizeof(init_uts_ns.version));
    kstrcpy_lim(init_uts_ns.machine, OV1_S("mos65c02"), sizeof(init_uts_ns.machine));
    kstrcpy_lim(init_uts_ns.domainname, OV1_S("(none)"), sizeof(init_uts_ns.domainname));

    printk(KERN_NOTICE, OV1_S("Linux version 6.6.0-mos65c02-nommu (root@microdos) (llvm-mos-clang 19.0.0, C23) #1 PREEMPT NOMMU"));
    printk(KERN_INFO, OV1_S("Command line: console=ttyS0,19200n8 root=/dev/ram0 rw init=/init loglevel=7"));

    uint8_t sp = arch_get_sp();
    uint8_t fl = arch_get_flags();
    char cpubuf[64];
    kstrcpy_lim(cpubuf, OV1_S("CPU: WDC W65C02S CMOS 8-bit (SP=0x01"), sizeof(cpubuf));
    static const char hc[] __attribute__((section(".overlay_1.rodata"))) = "0123456789abcdef";
    size_t l = kstrlen(cpubuf);
    cpubuf[l++] = hc[sp >> 4];
    cpubuf[l++] = hc[sp & 0x0FU];
    kstrcpy_lim(&cpubuf[l], OV1_S(" P=0x"), sizeof(cpubuf) - l);
    l = kstrlen(cpubuf);
    cpubuf[l++] = hc[fl >> 4];
    cpubuf[l++] = hc[fl & 0x0FU];
    cpubuf[l++] = ')';
    cpubuf[l] = '\0';
    printk(KERN_INFO, cpubuf);

    printk(KERN_INFO, OV1_S("Machine model: 65c02-SIM Virtual Motherboard (NOMMU 64KB Address Space)"));
    printk(KERN_INFO, OV1_S("earlycon: acia6850 at MMIO 0x7fc0 (options '19200n8')"));
    printk(KERN_INFO, OV1_S("printk: console [ttyS0] enabled"));
    printk(KERN_INFO, OV1_S("e820: BIOS-provided physical RAM map:"));
    printk(KERN_INFO, OV1_S("BIOS-e820: [mem 0x0000-0x005f] Zero Page Virtual Registers (96B)"));
    printk(KERN_INFO, OV1_S("BIOS-e820: [mem 0x0200-0x27ff] Kernel Resident Core (9728B)"));
    printk(KERN_INFO, OV1_S("BIOS-e820: [mem 0x2800-0x47ff] Overlay MMU Window (7x8KB = 56KB Paged)"));
    printk(KERN_INFO, OV1_S("BIOS-e820: [mem 0x4800-0x53ff] Kernel & User Software Stack (3072B)"));
    printk(KERN_INFO, OV1_S("BIOS-e820: [mem 0x5fc0-0x7fbf] Kernel Slab & Initramfs Arena (8192B)"));
    printk(KERN_INFO, OV1_S("BIOS-e820: [mem 0x7fc0-0x7fff] System MMIO Peripherals (64B)"));
    printk(KERN_INFO, OV1_S("BIOS-e820: [mem 0x8000-0xffff] microDOS ROM & ABI Jump Table (32KB)"));
}

OV1 static void mm_core_init_ov(void) {
    memset(page_bitmap, 0, sizeof(page_bitmap));
    memset(inode_table, 0, sizeof(inode_table));
    memset(ramfs_pool, 0, sizeof(ramfs_pool));
    printk(KERN_INFO, OV1_S("Zone ranges: Normal [mem 0x0200-0x7fbf] NOMMU flat mapping"));
    printk(KERN_INFO, OV1_S("Memory: 11K/64K available (56K paged kernel code, 8K slab/ramfs, 32K ROM)"));
    printk(KERN_INFO, OV1_S("SLUB: HWalign=1, Order=0-0, MinObjects=48, CPUs=1, Nodes=1"));
}

OV1 static void spawn_kthread_ov(uint8_t slot, uint8_t pid, uint8_t ppid, uint8_t state, uint8_t flags, const char* comm) {
    tasks[slot].pid = pid;
    tasks[slot].ppid = ppid;
    tasks[slot].state = state;
    tasks[slot].flags = flags;
    tasks[slot].uid = 0U;
    tasks[slot].nice = 0;
    tasks[slot].rss_pages = (flags & PF_KTHREAD) ? 0U : 4U;
    tasks[slot].ticks = (uint16_t)(slot + 1U);
    kstrcpy_lim(tasks[slot].comm, comm, sizeof(tasks[slot].comm));
}

OV1 static void sched_init_ov(void) {
    memset(tasks, 0, sizeof(tasks));
    spawn_kthread_ov(0, 1, 0, TASK_INTERRUPTIBLE, 0, OV1_S("init"));
    spawn_kthread_ov(1, 2, 0, TASK_INTERRUPTIBLE, PF_KTHREAD, OV1_S("kthreadd"));
    spawn_kthread_ov(2, 3, 2, TASK_INTERRUPTIBLE, PF_KTHREAD, OV1_S("ksoftirqd/0"));
    spawn_kthread_ov(3, 4, 2, TASK_INTERRUPTIBLE, PF_KTHREAD, OV1_S("kworker/0:0"));
    spawn_kthread_ov(4, 5, 2, TASK_INTERRUPTIBLE, PF_KTHREAD, OV1_S("kswapd0"));
    spawn_kthread_ov(5, 6, 2, TASK_INTERRUPTIBLE, PF_KTHREAD, OV1_S("spi_mmc0"));
    active_pid = 1U;
    printk(KERN_INFO, OV1_S("rcu: Preemptible hierarchical RCU implementation."));
    printk(KERN_INFO, OV1_S("NR_IRQS: 4, nr_cpu_ids: 1, preallocated irqs: 4"));
}

OV1 static void time_init_ov(void) {
    VIA_ACR |= 0x40U;
    VIA_T1L_L = 0xFFU;
    VIA_T1L_H = 0xFFU;
    VIA_T1C_H = 0xFFU;
    prev_t1_hi = VIA_T1C_H;
    jiffies += 12UL;
    printk(KERN_INFO, OV1_S("clocksource: via6522_t1: mask: 0xffff max_cycles: 0xffff, max_idle_ns: 6553500 ns"));
}

/* Port of init/calibrate.c: calibrate_delay() */
OV1 static void calibrate_delay_ov(void) {
    uint8_t t0 = VIA_T1C_L;
    uint16_t lpj = 256U;
    for (volatile uint16_t i = 0; i < 256U; i++) {}
    uint8_t t1 = VIA_T1C_L;
    uint8_t delta = (uint8_t)(t0 - t1);
    if (delta == 0U) delta = 4U;
    lpj = (uint16_t)(4096U + ((uint16_t)delta << 4));
    loops_per_jiffy = lpj;
    bogomips_int = (uint16_t)(lpj / 500U);
    bogomips_frac = (uint16_t)(((lpj % 500U) * 100U) / 500U);

    char bbuf[64];
    kstrcpy_lim(bbuf, OV1_S("Calibrating delay loop... "), sizeof(bbuf));
    fmt_u32_dec(bogomips_int, &bbuf[kstrlen(bbuf)]);
    size_t l = kstrlen(bbuf);
    bbuf[l++] = '.';
    if (bogomips_frac < 10U) bbuf[l++] = '0';
    fmt_u32_dec(bogomips_frac, &bbuf[l]);
    kstrcpy_lim(&bbuf[kstrlen(bbuf)], OV1_S(" BogoMIPS (lpj="), sizeof(bbuf) - kstrlen(bbuf));
    fmt_u32_dec(loops_per_jiffy, &bbuf[kstrlen(bbuf)]);
    l = kstrlen(bbuf);
    bbuf[l++] = ')';
    bbuf[l] = '\0';
    printk(KERN_INFO, bbuf);
}

OV1 static void drivers_init_ov(void) {
    printk(KERN_INFO, OV1_S("vfs: Dentry cache hash table entries: 48 (order: 0, 1344 bytes)"));
    printk(KERN_INFO, OV1_S("Mount-cache hash table entries: 8 (rootfs, proc, sysfs, devtmpfs, vfat)"));

    uint8_t acia_st = ACIA_STAT_REG;
    (void)acia_st;
    printk(KERN_INFO, OV1_S("Serial: 6850 ACIA driver, 1 ports, IRQ sharing disabled"));
    printk(KERN_INFO, OV1_S("0x7fc0: ttyS0 at MMIO 0x7fc0 (irq = 2, base_baud = 19200) is a MOS6850"));

    printk(KERN_INFO, OV1_S("via6522 0x7fd0: VIA 6522 adapter initialized (2x8-bit GPIO, 2x16-bit Timers)"));
    printk(KERN_INFO, OV1_S("spi_sdcard 0x7fc8: SPI SD Host Controller at MMIO 0x7fc8 CS=0"));
    printk(KERN_INFO, OV1_S("mmc0: new SDHC card at address 0001 -> mmcblk0: 32 MiB, mmcblk0p1 (FAT16)"));

    uint8_t esp_st = ESP8266_STATUS;
    (void)esp_st;
    printk(KERN_INFO, OV1_S("esp8266_net 0x7fc4: ESP8266 AT WiFi interface registered as wlan0"));

    sid_reset();
    printk(KERN_INFO, OV1_S("snd_mos_sid 0x7fe0: MOS 6581/8580 3-Voice SID Synthesizer registered (/dev/dsp)"));
    printk(KERN_INFO, OV1_S("NET: Registered PF_INET protocol family (TCP/IP offload via wlan0)"));
}

/* Port of kernel/panic.c: panic() */
OV1_ENTRY static void panic_ov(const char* msg) {
    boot_quiet = false;
    uint8_t sp = arch_get_sp();
    uint8_t fl = arch_get_flags();
    kputc('\n');
    printk(KERN_EMERG, OV1_S("----------------------------------------------------------------"));
    char pbuf[64];
    kstrcpy_lim(pbuf, OV1_S("Kernel panic - not syncing: "), sizeof(pbuf));
    kstrcpy_lim(&pbuf[kstrlen(pbuf)], msg ? msg : OV1_S("Fatal exception"), sizeof(pbuf) - kstrlen(pbuf));
    printk(KERN_EMERG, pbuf);

    printk(KERN_EMERG, OV1_S("CPU: 0 PID: 1 Comm: init Not tainted 6.6.0-mos65c02-nommu #1"));
    kputs("\x1b[31;1mRegisters: PC=0x");
    kprint_hex16((uint16_t)(uintptr_t)&panic_ov);
    kputs(" SP=0x01");
    kprint_hex8(sp);
    kputs(" P=0x");
    kprint_hex8(fl);
    kputs(" ZP_RC0=0x");
    kprint_hex8(*(volatile uint8_t*)0x00);
    kputs("\x1b[0m\n");
    printk(KERN_EMERG, OV1_S("Call Trace:"));
    printk(KERN_EMERG, OV1_S(" [<2800>] dump_stack_lvl+0x14/0x28 [overlay_1]"));
    printk(KERN_EMERG, OV1_S(" [<2842>] panic+0x38/0x90 [overlay_1]"));
    printk(KERN_EMERG, OV1_S(" [<0210>] _app_startup+0x12/0x24 [core_ram]"));
    printk(KERN_EMERG, OV1_S("---[ end Kernel panic - not syncing: Fatal exception ]---"));
    kernel_running = false;
}

/* Port of init/main.c: start_kernel() */
OV1_ENTRY static void start_kernel_ov(void) {
    setup_arch_ov();
    mm_core_init_ov();
    sched_init_ov();
    time_init_ov();
    calibrate_delay_ov();
    drivers_init_ov();
}


/* =========================================================================
 * OVERLAY 2 (page 1): init/initramfs.c (CPIO newc 070701) & /init runner
 * ========================================================================= */

#define CPIO_HDR(ino_s, mode_s, fsize_s, nsize_s) \
    "070701" ino_s mode_s "00000000" "00000000" "00000001" "654F0000" \
    fsize_s "00000000" "00000001" "00000000" "00000000" nsize_s "00000000"

/* Embedded CPIO newc (070701) initramfs archive in .overlay_2 */
static const char initramfs_cpio_archive[] __attribute__((section(".overlay_2.rodata"))) =
    /* 1. /bin (dir, namesize=4, pad=6) */
    CPIO_HDR("00000001", "000041ED", "00000000", "00000004") "bin\0\0\0"
    /* 2. /sbin (dir, namesize=5, pad=6) */
    CPIO_HDR("00000002", "000041ED", "00000000", "00000005") "sbin\0\0"
    /* 3. /etc (dir, namesize=4, pad=6) */
    CPIO_HDR("00000003", "000041ED", "00000000", "00000004") "etc\0\0\0"
    /* 4. /etc/init.d (dir, namesize=11, pad=14) */
    CPIO_HDR("00000004", "000041ED", "00000000", "0000000B") "etc/init.d\0\0\0\0"
    /* 5. /proc (dir, namesize=5, pad=6) */
    CPIO_HDR("00000005", "000041ED", "00000000", "00000005") "proc\0\0"
    /* 6. /sys (dir, namesize=4, pad=6) */
    CPIO_HDR("00000006", "000041ED", "00000000", "00000004") "sys\0\0\0"
    /* 7. /dev (dir, namesize=4, pad=6) */
    CPIO_HDR("00000007", "000041ED", "00000000", "00000004") "dev\0\0\0"
    /* 8. /root (dir, namesize=5, pad=6) */
    CPIO_HDR("00000008", "000041C0", "00000000", "00000005") "root\0\0"
    /* 9. /tmp (dir, namesize=4, pad=6) */
    CPIO_HDR("00000009", "000043FF", "00000000", "00000004") "tmp\0\0\0"
    /* 10. /var (dir, namesize=4, pad=6) */
    CPIO_HDR("0000000A", "000041ED", "00000000", "00000004") "var\0\0\0"
    /* 11. /mnt (dir, namesize=4, pad=6) */
    CPIO_HDR("0000000B", "000041ED", "00000000", "00000004") "mnt\0\0\0"
    /* 12. /mnt/sd (dir, namesize=7, pad=10) */
    CPIO_HDR("0000000C", "000041ED", "00000000", "00000007") "mnt/sd\0\0\0\0"
    /* 13. /etc/hostname (file, namesize=13->14, filesize=14->16) */
    CPIO_HDR("0000000D", "000081A4", "0000000E", "0000000D") "etc/hostname\0\0"
    "uclinux-65c02\n\0\0"
    /* 14. /etc/os-release (file, namesize=15->18, filesize=57->60) */
    CPIO_HDR("0000000E", "000081A4", "00000039", "0000000F") "etc/os-release\0\0\0\0"
    "NAME=\"uCLinux\"\nVERSION=\"6.6.0-mos65c02-nommu\"\nID=uclinux\n\0\0\0"
    /* 15. /etc/passwd (file, namesize=11->14, filesize=71->72) */
    CPIO_HDR("0000000F", "000081A4", "00000047", "0000000B") "etc/passwd\0\0\0\0"
    "root:x:0:0:root:/root:/bin/sh\nnobody:x:65534:65534:nobody:/:/bin/false\n\0"
    /* 16. /etc/fstab (file, namesize=10->10, filesize=94->96) */
    CPIO_HDR("00000010", "000081A4", "0000005E", "0000000A") "etc/fstab\0"
    "proc /proc proc defaults 0 0\nsysfs /sys sysfs defaults 0 0\n/dev/mmcblk0p1 /mnt/sd vfat rw 0 0\n\0\0"
    /* 17. /etc/motd (file, namesize=9->10, filesize=102->104) */
    CPIO_HDR("00000011", "000081A4", "00000066", "00000009") "etc/motd\0\0"
    "Welcome to uCLinux 6.6.0-mos65c02-nommu (WDC 65C02S)!\nType 'help' or 'busybox' for built-in commands.\n\0\0"
    /* 18. /etc/init.d/rcS (file, namesize=15->18, filesize=87->88) */
    CPIO_HDR("00000012", "000081ED", "00000057", "0000000F") "etc/init.d/rcS\0\0\0\0"
    "#!/bin/sh\nmount -t proc proc /proc\nmount -t sysfs sysfs /sys\nhostname -F /etc/hostname\n\0"
    /* 19. /init (file, namesize=5->6, filesize=31->32) */
    CPIO_HDR("00000013", "000081ED", "0000001F", "00000005") "init\0\0"
    "#!/bin/sh\nexec /etc/init.d/rcS\n\0"
    /* 20. /root/README (file, namesize=12->14, filesize=94->96) */
    CPIO_HDR("00000014", "000081A4", "0000005E", "0000000C") "root/README\0\0\0"
    "uCLinux 6.6.0 on 65C02-SIM (NOMMU)\nTry: uname -a, dmesg, ps, top, free, df, cat /proc/cpuinfo\n\0\0"
    /* 21. TRAILER!!! (namesize=11->14, filesize=0) */
    CPIO_HDR("00000000", "00000000", "00000000", "0000000B") "TRAILER!!!\0\0\0\0";

struct cpio_hdr_parsed {
    uint16_t ino;
    uint16_t mode;
    uint16_t uid;
    uint16_t gid;
    uint16_t filesize;
    uint16_t rdev;
    uint16_t namesize;
};

OV2 static uint16_t cpio_parse_hex8(const char* p) {
    uint16_t val = 0U;
    for (uint8_t i = 0; i < 8U; i++) {
        char c = p[i];
        uint8_t nib = 0U;
        if (c >= '0' && c <= '9') nib = (uint8_t)(c - '0');
        else if (c >= 'A' && c <= 'F') nib = (uint8_t)(c - 'A' + 10);
        else if (c >= 'a' && c <= 'f') nib = (uint8_t)(c - 'a' + 10);
        val = (uint16_t)((val << 4) | nib);
    }
    return val;
}

OV2 static bool cpio_parse_header(const char* s, struct cpio_hdr_parsed* h) {
    if (kstrncmp(s, OV2_S("070701"), 6) != 0) return false;
    h->ino      = cpio_parse_hex8(s + 6);
    h->mode     = cpio_parse_hex8(s + 14);
    h->uid      = cpio_parse_hex8(s + 22);
    h->gid      = cpio_parse_hex8(s + 30);
    h->filesize = cpio_parse_hex8(s + 54);
    uint8_t rma = (uint8_t)cpio_parse_hex8(s + 78);
    uint8_t rmi = (uint8_t)cpio_parse_hex8(s + 86);
    h->rdev     = MKDEV(rma, rmi);
    h->namesize = cpio_parse_hex8(s + 94);
    return true;
}

/* Port of init/initramfs.c: unpack_to_rootfs() state machine */
OV2 static bool unpack_to_rootfs_ov(const char* buf, uint16_t len) {
    enum cpio_state state = CPIO_Start;
    uint16_t pos = 0U;
    struct cpio_hdr_parsed hdr;
    int8_t cur_ino = -1;

    while (pos < len) {
        switch (state) {
            case CPIO_Start:
            case CPIO_Collect:
                if (pos + 110U > len) return false;
                if (!cpio_parse_header(&buf[pos], &hdr)) return false;
                state = CPIO_GotHeader;
                break;

            case CPIO_GotHeader: {
                const char* name_ptr = &buf[pos + 110U];
                if (kstrcmp(name_ptr, OV2_S("TRAILER!!!")) == 0) {
                    state = CPIO_Reset;
                    break;
                }
                scratch_path[0] = '/';
                kstrcpy_lim(&scratch_path[1], name_ptr, sizeof(scratch_path) - 1U);
                pos = (uint16_t)(pos + ((110U + hdr.namesize + 3U) & ~3U));
                state = CPIO_GotName;
                break;
            }

            case CPIO_GotName: {
                uint8_t parent_ino = 0U;
                int8_t existing = vfs_path_lookup(scratch_path, &parent_ino, scratch_arg);
                if (existing >= 0) {
                    cur_ino = existing;
                } else {
                    cur_ino = vfs_create_node(parent_ino, scratch_arg, hdr.mode, hdr.rdev);
                }
                unpacked_inodes_cnt++;
                state = (hdr.filesize > 0U) ? CPIO_CopyFile : CPIO_SkipIt;
                break;
            }

            case CPIO_CopyFile: {
                if (cur_ino >= 0 && hdr.filesize > 0U) {
                    uint16_t rem = hdr.filesize;
                    uint16_t off = 0U;
                    bool app = false;
                    while (rem > 0U) {
                        uint16_t chunk = (rem > sizeof(scratch_buf)) ? (uint16_t)sizeof(scratch_buf) : rem;
                        memcpy(scratch_buf, &buf[pos + off], chunk);
                        vfs_write_inode((uint8_t)cur_ino, scratch_buf, chunk, app);
                        app = true;
                        off = (uint16_t)(off + chunk);
                        rem = (uint16_t)(rem - chunk);
                    }
                    unpacked_bytes_cnt = (uint16_t)(unpacked_bytes_cnt + hdr.filesize);
                }
                pos = (uint16_t)(pos + ((hdr.filesize + 3U) & ~3U));
                state = CPIO_Start;
                break;
            }

            case CPIO_SkipIt:
                state = CPIO_Start;
                break;

            case CPIO_Reset:
                return true;
        }
    }
    return true;
}

OV2 static void create_dev_node_ov(const char* name, uint16_t mode, uint16_t rdev) {
    uint8_t dev_parent = 0U;
    int8_t dev_ino = vfs_path_lookup(kpath_copy(OV2_S("/dev")), &dev_parent, scratch_arg);
    if (dev_ino >= 0) {
        vfs_create_node((uint8_t)dev_ino, karg_copy(name), mode, rdev);
    }
}

OV2_ENTRY static void populate_rootfs_ov(void) {
    /* Root inode 0 ('/') */
    inode_table[0].ino = 0U;
    inode_table[0].parent_ino = 0U;
    inode_table[0].mode = (uint16_t)(S_IFDIR | 0755U);
    inode_table[0].uid = 0U;
    inode_table[0].gid = 0U;
    inode_table[0].size = 0U;
    inode_table[0].data_off = 0xFFFFU;
    kstrcpy_lim(inode_table[0].name, OV2_S("/"), sizeof(inode_table[0].name));
    cwd_ino = 0U;

    unpacked_inodes_cnt = 1U;
    unpacked_bytes_cnt = 0U;

    printk(KERN_INFO, OV2_S("Trying to unpack rootfs image as initramfs..."));
    bool ok = unpack_to_rootfs_ov(initramfs_cpio_archive, (uint16_t)(sizeof(initramfs_cpio_archive) - 1U));
    if (!ok) {
        panic(karg_copy(OV2_S("Unable to unpack initramfs CPIO archive")));
        return;
    }

    /* Populate devtmpfs (/dev) character & block device nodes */
    mnt_flags |= MNT_DEV;
    create_dev_node_ov(OV2_S("null"),      (uint16_t)(S_IFCHR | 0666U), MKDEV(1, 3));
    create_dev_node_ov(OV2_S("zero"),      (uint16_t)(S_IFCHR | 0666U), MKDEV(1, 5));
    create_dev_node_ov(OV2_S("urandom"),   (uint16_t)(S_IFCHR | 0666U), MKDEV(1, 9));
    create_dev_node_ov(OV2_S("mem"),       (uint16_t)(S_IFCHR | 0640U), MKDEV(1, 1));
    create_dev_node_ov(OV2_S("kmsg"),      (uint16_t)(S_IFCHR | 0644U), MKDEV(1, 11));
    create_dev_node_ov(OV2_S("ttyS0"),     (uint16_t)(S_IFCHR | 0660U), MKDEV(4, 64));
    create_dev_node_ov(OV2_S("console"),   (uint16_t)(S_IFCHR | 0600U), MKDEV(5, 1));
    create_dev_node_ov(OV2_S("dsp"),       (uint16_t)(S_IFCHR | 0666U), MKDEV(14, 3));
    create_dev_node_ov(OV2_S("mmcblk0p1"), (uint16_t)(S_IFBLK | 0660U), MKDEV(179, 1));

    /* Populate /bin/busybox and /bin/sh stub binaries */
    uint8_t p_ino = 0U;
    int8_t bin_ino = vfs_path_lookup(kpath_copy(OV2_S("/bin")), &p_ino, scratch_arg);
    if (bin_ino >= 0) {
        int8_t bb = vfs_create_node((uint8_t)bin_ino, karg_copy(OV2_S("busybox")), (uint16_t)(S_IFREG | 0755U), 0U);
        if (bb >= 0) {
            kstrcpy_lim(scratch_buf, OV2_S("\x7f""ELF-6502-NOMMU BusyBox v1.36.1 multi-call binary\n"), sizeof(scratch_buf));
            vfs_write_inode((uint8_t)bb, scratch_buf, (uint16_t)kstrlen(scratch_buf), false);
        }
        int8_t sh = vfs_create_node((uint8_t)bin_ino, karg_copy(OV2_S("sh")), (uint16_t)(S_IFLNK | 0777U), 0U);
        if (sh >= 0) {
            kstrcpy_lim(scratch_buf, OV2_S("/bin/busybox"), sizeof(scratch_buf));
            vfs_write_inode((uint8_t)sh, scratch_buf, (uint16_t)kstrlen(scratch_buf), false);
        }
    }

    printk(KERN_INFO, OV2_S("devtmpfs: mounted on /dev (9 device nodes registered)"));
    char msg[64];
    kstrcpy_lim(msg, OV2_S("Freeing initrd memory: 2K (unpacked "), sizeof(msg));
    fmt_u32_dec(unpacked_inodes_cnt, &msg[kstrlen(msg)]);
    kstrcpy_lim(&msg[kstrlen(msg)], OV2_S(" inodes, "), sizeof(msg) - kstrlen(msg));
    fmt_u32_dec(unpacked_bytes_cnt, &msg[kstrlen(msg)]);
    kstrcpy_lim(&msg[kstrlen(msg)], OV2_S(" bytes)"), sizeof(msg) - kstrlen(msg));
    printk(KERN_INFO, msg);

    if (sd_opendir(&lnx_sd_dir, OV2_S("")) == 0) {
        sd_closedir(&lnx_sd_dir);
        mnt_flags |= MNT_SD;
        irq_sd_cnt++;
        printk(KERN_INFO, OV2_S("FAT-fs (mmcblk0p1): Volume mounted on /mnt/sd (rw,fmask=0022)"));
    }
}

OV2_ENTRY static void run_init_process_ov(void) {
    if (boot_panic_test) {
        panic(karg_copy(OV2_S("Attempted to kill init! exitcode=0x00000100")));
        return;
    }

    printk(KERN_INFO, OV2_S("Freeing unused kernel image (initmem) memory: 8K"));
    printk(KERN_INFO, OV2_S("This architecture does not have kernel memory protection."));
    printk(KERN_INFO, OV2_S("Run /init as init process"));
    printk(KERN_DEBUG, OV2_S("  with arguments: /init"));
    printk(KERN_DEBUG, OV2_S("  with environment: HOME=/root TERM=vt100 PATH=/bin:/sbin"));

    if (!boot_single) {
        shell_run_script(kpath_copy(OV2_S("/etc/init.d/rcS")), true);
    } else {
        mnt_flags |= (MNT_PROC | MNT_SYS);
        printk(KERN_NOTICE, OV2_S("init: booting into single-user maintenance mode"));
    }

    /* Spawn interactive shell task (PID 7) */
    tasks[6].pid = 7U;
    tasks[6].ppid = 1U;
    tasks[6].state = TASK_RUNNING;
    tasks[6].flags = 0U;
    tasks[6].uid = 0U;
    tasks[6].nice = 0;
    tasks[6].rss_pages = 6U;
    tasks[6].ticks = 3U;
    kstrcpy_lim(tasks[6].comm, OV2_S("sh"), sizeof(tasks[6].comm));
    active_pid = 7U;

    /* Change working directory to /root if present */
    uint8_t p_ino = 0U;
    int8_t root_home = vfs_path_lookup(kpath_copy(OV2_S("/root")), &p_ino, scratch_arg);
    if (root_home >= 0) {
        cwd_ino = (uint8_t)root_home;
        env_set("PWD", "/root");
    }

    kputc('\n');
    int16_t n = vfs_read_by_path(kpath_copy(OV2_S("/etc/motd")), scratch_buf, sizeof(scratch_buf) - 1U);
    if (n > 0) {
        scratch_buf[n] = '\0';
        kputs("\x1b[36;1m");
        kputs(scratch_buf);
        kputs("\x1b[0m");
    }
}

/* =========================================================================
 * OVERLAY 3 (page 2): fs/namei.c, fs/ramfs/, drivers/char/mem.c, VFAT /mnt/sd
 * ========================================================================= */

OV3 static bool ino_is_used(uint8_t idx) {
    return inode_table[idx].mode != 0U;
}

OV3 static int8_t vfs_find_child(uint8_t dir_ino, const char* name) {
    if (kstrcmp(name, OV3_S(".")) == 0) return (int8_t)dir_ino;
    if (kstrcmp(name, OV3_S("..")) == 0) return (int8_t)inode_table[dir_ino].parent_ino;
    for (uint8_t i = 1U; i < MAX_INODES; i++) {
        if (ino_is_used(i) && inode_table[i].parent_ino == dir_ino && kstrcmp(inode_table[i].name, name) == 0) {
            return (int8_t)i;
        }
    }
    return -1;
}

OV3_ENTRY static int8_t vfs_path_lookup_ov(const char* path, uint8_t* parent_out, char* leaf_out) {
    if (!path || !path[0]) {
        if (parent_out) *parent_out = inode_table[cwd_ino].parent_ino;
        if (leaf_out) leaf_out[0] = '\0';
        return (int8_t)cwd_ino;
    }

    uint8_t cur = (path[0] == '/') ? 0U : cwd_ino;
    const char* p = path;
    while (*p == '/') p++;

    if (*p == '\0') {
        if (parent_out) *parent_out = 0U;
        if (leaf_out) { leaf_out[0] = '/'; leaf_out[1] = '\0'; }
        return 0;
    }

    char comp[14];
    while (*p) {
        uint8_t ci = 0U;
        while (*p && *p != '/') {
            if (ci < sizeof(comp) - 1U) comp[ci++] = *p;
            p++;
        }
        comp[ci] = '\0';
        while (*p == '/') p++;

        bool is_last = (*p == '\0');
        if (parent_out) *parent_out = cur;
        if (leaf_out) kstrcpy_lim(leaf_out, comp, 14U);

        int8_t next = vfs_find_child(cur, comp);
        if (next < 0) {
            return is_last ? -1 : -2;
        }
        cur = (uint8_t)next;
    }
    return (int8_t)cur;
}

OV3_ENTRY static int8_t vfs_create_node_ov(uint8_t parent, const char* name, uint16_t mode, uint16_t rdev) {
    if (!name || !name[0] || parent >= MAX_INODES) return -1;
    int8_t existing = vfs_find_child(parent, name);
    if (existing >= 0) return existing;

    for (uint8_t i = 1U; i < MAX_INODES; i++) {
        if (!ino_is_used(i)) {
            inode_table[i].ino = i;
            inode_table[i].parent_ino = parent;
            inode_table[i].mode = mode;
            inode_table[i].uid = current_uid;
            inode_table[i].gid = current_gid;
            inode_table[i].size = 0U;
            inode_table[i].data_off = 0xFFFFU;
            inode_table[i].rdev = rdev;
            kstrcpy_lim(inode_table[i].name, name, sizeof(inode_table[i].name));
            return (int8_t)i;
        }
    }
    return -1;
}

OV3_ENTRY static int16_t vfs_write_inode_ov(uint8_t ino, const void* data, uint16_t len, bool append) {
    if (ino >= MAX_INODES || !ino_is_used(ino)) return -1;
    struct inode* nd = &inode_table[ino];

    /* Handle character devices in /dev */
    if (S_ISCHR(nd->mode)) {
        uint8_t maj = MAJOR(nd->rdev);
        uint8_t min = MINOR(nd->rdev);
        const char* cdata = (const char*)data;
        if (maj == 1U && (min == 3U || min == 5U)) {
            return (int16_t)len; /* /dev/null, /dev/zero */
        }
        if (maj == 1U && min == 11U) {
            /* /dev/kmsg injects into kernel ring buffer */
            char kbuf[64];
            uint16_t clen = (len < sizeof(kbuf) - 1U) ? len : (uint16_t)(sizeof(kbuf) - 1U);
            for (uint16_t i = 0; i < clen; i++) {
                kbuf[i] = (cdata[i] == '\n') ? '\0' : cdata[i];
            }
            kbuf[clen] = '\0';
            printk(KERN_INFO, kbuf);
            return (int16_t)len;
        }
        if (maj == 4U || maj == 5U) {
            for (uint16_t i = 0; i < len; i++) kputc(cdata[i]);
            return (int16_t)len;
        }
        if (maj == 14U && min == 3U) {
            /* /dev/dsp drives SID 6581 Voice 1 */
            uint16_t freq = 0x1120U;
            if (len >= 2U) {
                freq = (uint16_t)(((uint8_t)cdata[0] << 8) | (uint8_t)cdata[1]);
                if (freq < 0x0400U) freq = (uint16_t)(0x0800U + (freq << 3));
            }
            sid_set_volume(15);
            sid_set_envelope(1, 0, 9, 10, 0);
            sid_trigger_note(1, freq, WAVE_TRI | WAVE_GATE);
            for (volatile uint16_t d = 0; d < 3500U; d++) {}
            sid_stop_note(1);
            return (int16_t)len;
        }
        return (int16_t)len;
    }

    uint16_t old_size = append ? nd->size : 0U;
    uint16_t new_size = (uint16_t)(old_size + len);
    if (new_size == 0U) {
        if (nd->data_off != 0xFFFFU) {
            ramfs_free_range(nd->data_off, nd->size);
            nd->data_off = 0xFFFFU;
        }
        nd->size = 0U;
        return 0;
    }

    uint16_t old_cap = (nd->data_off != 0xFFFFU && nd->size > 0U)
        ? (uint16_t)(((nd->size + RAMFS_BLK_SIZE - 1U) / RAMFS_BLK_SIZE) * RAMFS_BLK_SIZE)
        : 0U;

    if (!append && nd->data_off != 0xFFFFU) {
        ramfs_free_range(nd->data_off, nd->size);
        nd->data_off = 0xFFFFU;
        nd->size = 0U;
        old_cap = 0U;
    }

    if (nd->data_off == 0xFFFFU || new_size > old_cap) {
        int16_t new_off = ramfs_alloc_contiguous(new_size);
        if (new_off < 0) return -1;
        if (old_size > 0U && nd->data_off != 0xFFFFU) {
            memcpy(&ramfs_pool[(uint16_t)new_off], &ramfs_pool[nd->data_off], old_size);
            ramfs_free_range(nd->data_off, nd->size);
        }
        nd->data_off = (uint16_t)new_off;
    }

    if (len > 0U && data) {
        memcpy(&ramfs_pool[nd->data_off + old_size], data, len);
    }
    nd->size = new_size;
    return (int16_t)len;
}

OV3_ENTRY static int16_t vfs_read_inode_ov(uint8_t ino, void* buf, uint16_t max_len, uint16_t offset) {
    if (ino >= MAX_INODES || !ino_is_used(ino) || !buf) return -1;
    struct inode* nd = &inode_table[ino];

    if (S_ISCHR(nd->mode)) {
        uint8_t maj = MAJOR(nd->rdev);
        uint8_t min = MINOR(nd->rdev);
        uint8_t* ob = (uint8_t*)buf;
        if (maj == 1U && min == 3U) return 0; /* /dev/null -> EOF */
        if (maj == 1U && min == 5U) {
            uint16_t n = (max_len > 64U) ? 64U : max_len;
            memset(ob, 0, n);
            return (int16_t)n;
        }
        if (maj == 1U && min == 9U) {
            uint16_t n = (max_len > 32U) ? 32U : max_len;
            for (uint16_t i = 0; i < n; i++) ob[i] = next_prng_byte();
            return (int16_t)n;
        }
        if (maj == 1U && min == 1U) {
            uint16_t n = (max_len > 64U) ? 64U : max_len;
            for (uint16_t i = 0; i < n; i++) {
                ob[i] = *(volatile const uint8_t*)(uintptr_t)(offset + i);
            }
            return (int16_t)n;
        }
        return 0;
    }

    if (nd->data_off == 0xFFFFU || offset >= nd->size) return 0;
    uint16_t avail = (uint16_t)(nd->size - offset);
    if (avail > max_len) avail = max_len;
    memcpy(buf, &ramfs_pool[nd->data_off + offset], avail);
    return (int16_t)avail;
}

OV3_ENTRY static bool vfs_unlink_path_ov(const char* path, bool recursive) {
    uint8_t parent = 0U;
    char leaf[14];
    int8_t ino = vfs_path_lookup_ov(path, &parent, leaf);
    if (ino <= 0) return false;

    if (S_ISDIR(inode_table[(uint8_t)ino].mode)) {
        for (uint8_t i = 1U; i < MAX_INODES; i++) {
            if (ino_is_used(i) && inode_table[i].parent_ino == (uint8_t)ino) {
                if (!recursive) return false;
                if (inode_table[i].data_off != 0xFFFFU) {
                    ramfs_free_range(inode_table[i].data_off, inode_table[i].size);
                }
                memset(&inode_table[i], 0, sizeof(struct inode));
            }
        }
    }

    if (inode_table[(uint8_t)ino].data_off != 0xFFFFU) {
        ramfs_free_range(inode_table[(uint8_t)ino].data_off, inode_table[(uint8_t)ino].size);
    }
    memset(&inode_table[(uint8_t)ino], 0, sizeof(struct inode));
    return true;
}

OV3_ENTRY static void vfs_build_cwd_ov(char* out, uint8_t max_len) {
    if (cwd_ino == 0U || !ino_is_used(cwd_ino)) {
        out[0] = '/';
        out[1] = '\0';
        return;
    }
    uint8_t chain[8];
    uint8_t depth = 0U;
    uint8_t cur = cwd_ino;
    while (cur != 0U && depth < 8U && ino_is_used(cur)) {
        chain[depth++] = cur;
        cur = inode_table[cur].parent_ino;
    }
    uint8_t pos = 0U;
    for (int8_t d = (int8_t)(depth - 1); d >= 0; d--) {
        if (pos < max_len - 1U) out[pos++] = '/';
        const char* nm = inode_table[chain[(uint8_t)d]].name;
        for (uint8_t j = 0U; nm[j] && pos < max_len - 1U; j++) {
            out[pos++] = nm[j];
        }
    }
    if (pos == 0U) out[pos++] = '/';
    out[pos] = '\0';
}

OV3 static void vfs_resolve_abs_ov(const char* path, char* abs_out, uint8_t max_len) {
    if (path[0] == '/') {
        kstrcpy_lim(abs_out, path, max_len);
        return;
    }
    vfs_build_cwd_ov(abs_out, max_len);
    size_t l = kstrlen(abs_out);
    if (l > 1U && l < max_len - 1U) abs_out[l++] = '/';
    kstrcpy_lim(&abs_out[l], path, max_len - l);
}

OV3_ENTRY static int16_t vfs_read_by_path_ov(const char* path, char* out_buf, uint16_t max_len) {
    char abs[64];
    vfs_resolve_abs_ov(path, abs, sizeof(abs));

    if ((mnt_flags & MNT_SD) && kstrncmp(abs, OV3_S("/mnt/sd/"), 8) == 0) {
        irq_sd_cnt++;
        if (sd_open(&lnx_sd_fp, &abs[8], SD_READ) == 0) {
            int16_t br = sd_read(&lnx_sd_fp, out_buf, max_len);
            sd_close(&lnx_sd_fp);
            return (br >= 0) ? br : -1;
        }
        return -1;
    }

    uint8_t p_ino = 0U;
    char leaf[14];
    int8_t ino = vfs_path_lookup_ov(abs, &p_ino, leaf);
    if (ino < 0) return -1;
    return vfs_read_inode_ov((uint8_t)ino, out_buf, max_len, 0U);
}

OV3_ENTRY static bool vfs_write_by_path_ov(const char* path, const char* data, uint16_t len, bool append) {
    char abs[64];
    vfs_resolve_abs_ov(path, abs, sizeof(abs));

    if (kstrncmp(abs, OV3_S("/proc/"), 6) == 0 || kstrncmp(abs, OV3_S("/sys/"), 5) == 0) {
        kstrcpy_lim(scratch_path, abs, sizeof(scratch_path));
        kstrcpy_lim(scratch_arg, data, sizeof(scratch_arg));
        return proc_sys_handle(scratch_path, true, scratch_arg);
    }

    if ((mnt_flags & MNT_SD) && kstrncmp(abs, OV3_S("/mnt/sd/"), 8) == 0) {
        irq_sd_cnt++;
        uint8_t mode = (uint8_t)(SD_WRITE | (append ? SD_OPEN_ALWAYS : SD_CREATE_ALWAYS));
        if (sd_open(&lnx_sd_fp, &abs[8], mode) == 0) {
            if (append) sd_seek(&lnx_sd_fp, 0xFFFFFFFFUL);
            sd_write(&lnx_sd_fp, data, len);
            sd_close(&lnx_sd_fp);
            return true;
        }
        return false;
    }

    uint8_t p_ino = 0U;
    char leaf[14];
    int8_t ino = vfs_path_lookup_ov(abs, &p_ino, leaf);
    if (ino < 0) {
        if (ino == -2) return false;
        ino = vfs_create_node_ov(p_ino, leaf, (uint16_t)(S_IFREG | 0644U), 0U);
        if (ino < 0) return false;
    }
    return vfs_write_inode_ov((uint8_t)ino, data, len, append) >= 0;
}


/* =========================================================================
 * OVERLAY 4 (page 3): fs/proc/ & fs/sysfs/ Dynamic Virtual Filesystems
 * ========================================================================= */

OV4 static void proc_resolve_abs_ov(const char* path, char* out, uint8_t max_len) {
    if (path[0] == '/') {
        kstrcpy_lim(out, path, max_len);
        return;
    }
    vfs_build_cwd(out, max_len);
    size_t l = kstrlen(out);
    if (l > 1U && l < max_len - 1U) out[l++] = '/';
    kstrcpy_lim(&out[l], path, max_len - l);
}

OV4 static uint8_t count_active_tasks_ov(uint8_t* running_out) {
    uint8_t total = 0U;
    uint8_t run = 0U;
    for (uint8_t i = 0U; i < MAX_TASKS; i++) {
        if (tasks[i].pid != 0U) {
            total++;
            if (tasks[i].state == TASK_RUNNING) run++;
        }
    }
    if (running_out) *running_out = run;
    return total;
}

OV4_ENTRY static bool proc_sys_handle_ov(const char* path, bool is_write, const char* write_val) {
    update_jiffies();
    char abs[64];
    proc_resolve_abs_ov(path, abs, sizeof(abs));

    if (kstrncmp(abs, OV4_S("/proc"), 5) == 0 && !(mnt_flags & MNT_PROC)) return false;
    if (kstrncmp(abs, OV4_S("/sys"), 4) == 0 && !(mnt_flags & MNT_SYS)) return false;

    /* Writable sysfs / procfs knobs */
    if (is_write) {
        if (kstrcmp(abs, OV4_S("/sys/kernel/hostname")) == 0 ||
            kstrcmp(abs, OV4_S("/proc/sys/kernel/hostname")) == 0) {
            char hbuf[18];
            uint8_t i = 0U;
            while (write_val && write_val[i] && write_val[i] != '\n' && write_val[i] != '\r' && i < sizeof(hbuf) - 1U) {
                hbuf[i] = write_val[i];
                i++;
            }
            hbuf[i] = '\0';
            if (i > 0U) {
                kstrcpy_lim(init_uts_ns.nodename, hbuf, sizeof(init_uts_ns.nodename));
                env_set("HOSTNAME", init_uts_ns.nodename);
            }
            return true;
        }
        if (kstrcmp(abs, OV4_S("/proc/sys/kernel/printk")) == 0) {
            if (write_val && write_val[0] >= '0' && write_val[0] <= '8') {
                console_loglevel = (uint8_t)(write_val[0] - '0');
            }
            return true;
        }
        return false;
    }

    /* Read-only /proc generators */
    if (kstrcmp(abs, OV4_S("/proc/cpuinfo")) == 0) {
        kprintln(OV4_S("processor\t: 0"));
        kprintln(OV4_S("vendor_id\t: WesternDesignCenter"));
        kprintln(OV4_S("cpu family\t: 65C02"));
        kprintln(OV4_S("model name\t: WDC W65C02S CMOS 8-bit Microprocessor"));
        kprintln(OV4_S("stepping\t: 2"));
        kprintln(OV4_S("cpu MHz\t\t: 1.000"));
        kputs(OV4_S("bogomips\t: "));
        kprint_u16(bogomips_int);
        kputc('.');
        if (bogomips_frac < 10U) kputc('0');
        kprint_u16(bogomips_frac);
        kputc('\n');
        kprintln(OV4_S("flags\t\t: cmos zp_ind bit_ops bbr_bbs wai stp nommu"));
        kprintln(OV4_S("byte order\t: Little Endian"));
        return true;
    }

    if (kstrcmp(abs, OV4_S("/proc/meminfo")) == 0) {
        struct sysinfo si;
        si_meminfo(&si);
        uint16_t used_inodes = 0U;
        for (uint8_t i = 0U; i < MAX_INODES; i++) {
            if (inode_table[i].mode != 0U) used_inodes++;
        }
        uint16_t slab_b = (uint16_t)(used_inodes * sizeof(struct inode));
        kputs(OV4_S("MemTotal:           ")); kprint_u16(si.totalram);  kprintln(OV4_S(" B"));
        kputs(OV4_S("MemFree:            ")); kprint_u16(si.freeram);   kprintln(OV4_S(" B"));
        kputs(OV4_S("MemAvailable:       ")); kprint_u16(si.freeram);   kprintln(OV4_S(" B"));
        kputs(OV4_S("Buffers:            ")); kprint_u16(si.bufferram); kprintln(OV4_S(" B"));
        kputs(OV4_S("Cached:             ")); kprint_u16((uint16_t)(si.totalram - si.freeram)); kprintln(OV4_S(" B"));
        kputs(OV4_S("Slab:               ")); kprint_u16(slab_b);       kprintln(OV4_S(" B"));
        kprintln(OV4_S("OverlayPagedTotal: 57344 B (7 x 8192 B banks)"));
        kprintln(OV4_S("SwapTotal:             0 B"));
        kprintln(OV4_S("SwapFree:              0 B"));
        return true;
    }

    if (kstrcmp(abs, OV4_S("/proc/version")) == 0) {
        kputs(init_uts_ns.sysname);
        kputs(OV4_S(" version "));
        kputs(init_uts_ns.release);
        kputs(OV4_S(" (root@"));
        kputs(init_uts_ns.nodename);
        kputs(OV4_S(") (llvm-mos-clang 19.0.0) "));
        kprintln(init_uts_ns.version);
        return true;
    }

    if (kstrcmp(abs, OV4_S("/proc/uptime")) == 0) {
        uint32_t sec = jiffies / 100UL;
        uint32_t csec = jiffies % 100UL;
        kprint_u32(sec);
        kputc('.');
        if (csec < 10UL) kputc('0');
        kprint_u32(csec);
        kputc(' ');
        kprint_u32((sec * 92UL) / 100UL);
        kprintln(OV4_S(".40"));
        return true;
    }

    if (kstrcmp(abs, OV4_S("/proc/loadavg")) == 0) {
        uint8_t run = 0U;
        uint8_t tot = count_active_tasks_ov(&run);
        kputs(OV4_S("0.08 0.03 0.01 "));
        kprint_u16(run);
        kputc('/');
        kprint_u16(tot);
        kputc(' ');
        kprint_u16(active_pid);
        kputc('\n');
        return true;
    }

    if (kstrcmp(abs, OV4_S("/proc/cmdline")) == 0) {
        kprintln(OV4_S("console=ttyS0,19200n8 root=/dev/ram0 rw init=/init loglevel=7 nommu"));
        return true;
    }

    if (kstrcmp(abs, OV4_S("/proc/mounts")) == 0) {
        kprintln(OV4_S("rootfs / rootfs rw 0 0"));
        if (mnt_flags & MNT_DEV)  kprintln(OV4_S("devtmpfs /dev devtmpfs rw,mode=0755 0 0"));
        if (mnt_flags & MNT_PROC) kprintln(OV4_S("proc /proc proc rw,nosuid,nodev,noexec 0 0"));
        if (mnt_flags & MNT_SYS)  kprintln(OV4_S("sysfs /sys sysfs rw,nosuid,nodev,noexec 0 0"));
        if (mnt_flags & MNT_SD)   kprintln(OV4_S("/dev/mmcblk0p1 /mnt/sd vfat rw,fmask=0022 0 0"));
        return true;
    }

    if (kstrcmp(abs, OV4_S("/proc/interrupts")) == 0) {
        kprintln(OV4_S("           CPU0"));
        kputs(OV4_S("  2:       ")); kprint_u16(irq_acia_cnt); kprintln(OV4_S("   MOS6850  ttyS0"));
        kputs(OV4_S("  4:       ")); kprint_u16(irq_via_cnt);  kprintln(OV4_S("   VIA6522  timer1"));
        kputs(OV4_S("  6:       ")); kprint_u16(irq_sd_cnt);   kprintln(OV4_S("   SPI-MMC  mmc0"));
        kprintln(OV4_S("NMI:          0   Non-maskable interrupts"));
        return true;
    }

    if (kstrcmp(abs, OV4_S("/proc/iomem")) == 0) {
        kprintln(OV4_S("0000-005f : Zero Page Virtual Registers"));
        kprintln(OV4_S("0200-27ff : Kernel Resident Core (.text/.rodata)"));
        kprintln(OV4_S("2800-47ff : Overlay Paged Window (7x8KB)"));
        kprintln(OV4_S("4800-53ff : Kernel & User Software Stack"));
        kprintln(OV4_S("5fc0-7fbf : Kernel Slab & Initramfs Pool"));
        kprintln(OV4_S("7fc0-7fc3 : acia6850 serial (ttyS0)"));
        kprintln(OV4_S("7fc4-7fc5 : esp8266 wifi (wlan0)"));
        kprintln(OV4_S("7fc8-7fc9 : spi_sdcard (mmc0)"));
        kprintln(OV4_S("7fd0-7fdf : via6522 timer/gpio"));
        kprintln(OV4_S("7fe0-7fff : mos6581 sid audio (/dev/dsp)"));
        kprintln(OV4_S("8000-ffff : microDOS System ROM"));
        return true;
    }

    if (kstrcmp(abs, OV4_S("/proc/modules")) == 0) {
        kprintln(OV4_S("snd_mos_sid 1536 0 - Live 0x7fe0"));
        kprintln(OV4_S("esp8266_net 2048 0 - Live 0x7fc4"));
        kprintln(OV4_S("spi_sdcard 2560 1 - Live 0x7fc8"));
        kprintln(OV4_S("via6522_timer 1024 1 - Live 0x7fd0"));
        kprintln(OV4_S("acia6850_tty 1280 1 - Live 0x7fc0"));
        return true;
    }

    if (kstrcmp(abs, OV4_S("/proc/stat")) == 0) {
        kputs(OV4_S("cpu  "));
        kprint_u32(jiffies / 3UL);
        kputs(OV4_S(" 0 "));
        kprint_u32(jiffies / 4UL);
        kputc(' ');
        kprint_u32(jiffies);
        kprintln(OV4_S(" 0 0 0 0"));
        kputs(OV4_S("intr "));
        kprint_u16((uint16_t)(irq_acia_cnt + irq_via_cnt + irq_sd_cnt));
        kputc('\n');
        kputs(OV4_S("ctxt "));
        kprint_u32(jiffies * 2UL);
        kputc('\n');
        kprintln(OV4_S("btime 1700000000"));
        return true;
    }

    /* /proc/<pid>/status and /proc/self/status */
    if (kstrncmp(abs, OV4_S("/proc/"), 6) == 0) {
        const char* p = &abs[6];
        uint8_t target_pid = 0U;
        if (kstrncmp(p, OV4_S("self/"), 5) == 0) {
            target_pid = active_pid;
            p += 5;
        } else if (*p >= '0' && *p <= '9') {
            while (*p >= '0' && *p <= '9') {
                target_pid = (uint8_t)(target_pid * 10U + (uint8_t)(*p - '0'));
                p++;
            }
            if (*p == '/') p++;
        }
        if (target_pid > 0U) {
            for (uint8_t i = 0U; i < MAX_TASKS; i++) {
                if (tasks[i].pid == target_pid) {
                    if (kstrcmp(p, OV4_S("status")) == 0) {
                        kputs(OV4_S("Name:\t")); kprintln(tasks[i].comm);
                        kputs(OV4_S("State:\t"));
                        kprintln(tasks[i].state == TASK_RUNNING ? OV4_S("R (running)") : OV4_S("S (sleeping)"));
                        kputs(OV4_S("Pid:\t")); kprint_u16(tasks[i].pid); kputc('\n');
                        kputs(OV4_S("PPid:\t")); kprint_u16(tasks[i].ppid); kputc('\n');
                        kputs(OV4_S("Uid:\t")); kprint_u16(tasks[i].uid); kputc('\n');
                        kputs(OV4_S("VmRSS:\t")); kprint_u16((uint16_t)(tasks[i].rss_pages * 64U)); kprintln(OV4_S(" B"));
                        return true;
                    }
                    if (kstrcmp(p, OV4_S("cmdline")) == 0) {
                        kprintln(tasks[i].comm);
                        return true;
                    }
                }
            }
        }
    }

    /* /sys and /proc/sys nodes */
    if (kstrcmp(abs, OV4_S("/sys/kernel/hostname")) == 0 ||
        kstrcmp(abs, OV4_S("/proc/sys/kernel/hostname")) == 0) {
        kprintln(init_uts_ns.nodename);
        return true;
    }
    if (kstrcmp(abs, OV4_S("/sys/kernel/ostype")) == 0 ||
        kstrcmp(abs, OV4_S("/proc/sys/kernel/ostype")) == 0) {
        kprintln(init_uts_ns.sysname);
        return true;
    }
    if (kstrcmp(abs, OV4_S("/sys/kernel/osrelease")) == 0 ||
        kstrcmp(abs, OV4_S("/proc/sys/kernel/osrelease")) == 0) {
        kprintln(init_uts_ns.release);
        return true;
    }
    if (kstrcmp(abs, OV4_S("/sys/kernel/version")) == 0) {
        kprintln(init_uts_ns.version);
        return true;
    }
    if (kstrcmp(abs, OV4_S("/proc/sys/kernel/printk")) == 0) {
        kprint_u16(console_loglevel);
        kprintln(OV4_S("\t4\t1\t7"));
        return true;
    }
    if (kstrcmp(abs, OV4_S("/sys/class/tty/ttyS0/dev")) == 0) {
        kprintln(OV4_S("4:64"));
        return true;
    }
    if (kstrcmp(abs, OV4_S("/sys/class/net/wlan0/operstate")) == 0) {
        kprintln((ESP8266_STATUS != 0xFFU) ? OV4_S("up") : OV4_S("down"));
        return true;
    }
    if (kstrcmp(abs, OV4_S("/sys/block/mmcblk0/size")) == 0) {
        kprintln(OV4_S("65536"));
        return true;
    }

    return false;
}

OV4 static void print_virtual_entry_ov(const char* name, bool is_dir, bool long_fmt) {
    if (long_fmt) {
        kputs(is_dir ? OV4_S("dr-xr-xr-x  1 root root    0 ") : OV4_S("-r--r--r--  1 root root    0 "));
        if (is_dir) kputs("\x1b[34;1m");
        kputs(name);
        if (is_dir) kputs("\x1b[0m");
        kputc('\n');
    } else {
        if (is_dir) kputs("\x1b[34;1m");
        kputs(name);
        if (is_dir) kputs("\x1b[0m");
        kputs(OV4_S("  "));
    }
}

OV4_ENTRY static bool proc_sys_listdir_ov(const char* path, bool long_fmt) {
    char abs[64];
    proc_resolve_abs_ov(path, abs, sizeof(abs));

    if (kstrcmp(abs, OV4_S("/proc")) == 0) {
        if (!(mnt_flags & MNT_PROC)) return false;
        for (uint8_t i = 0U; i < MAX_TASKS; i++) {
            if (tasks[i].pid != 0U) {
                char pbuf[6];
                fmt_u32_dec(tasks[i].pid, pbuf);
                print_virtual_entry_ov(pbuf, true, long_fmt);
            }
        }
        print_virtual_entry_ov(OV4_S("self"), true, long_fmt);
        print_virtual_entry_ov(OV4_S("sys"), true, long_fmt);
        print_virtual_entry_ov(OV4_S("cpuinfo"), false, long_fmt);
        print_virtual_entry_ov(OV4_S("meminfo"), false, long_fmt);
        print_virtual_entry_ov(OV4_S("version"), false, long_fmt);
        print_virtual_entry_ov(OV4_S("uptime"), false, long_fmt);
        print_virtual_entry_ov(OV4_S("loadavg"), false, long_fmt);
        print_virtual_entry_ov(OV4_S("mounts"), false, long_fmt);
        print_virtual_entry_ov(OV4_S("interrupts"), false, long_fmt);
        print_virtual_entry_ov(OV4_S("iomem"), false, long_fmt);
        print_virtual_entry_ov(OV4_S("modules"), false, long_fmt);
        print_virtual_entry_ov(OV4_S("stat"), false, long_fmt);
        print_virtual_entry_ov(OV4_S("cmdline"), false, long_fmt);
        if (!long_fmt) kputc('\n');
        return true;
    }

    if (kstrcmp(abs, OV4_S("/sys")) == 0) {
        if (!(mnt_flags & MNT_SYS)) return false;
        print_virtual_entry_ov(OV4_S("kernel"), true, long_fmt);
        print_virtual_entry_ov(OV4_S("class"), true, long_fmt);
        print_virtual_entry_ov(OV4_S("block"), true, long_fmt);
        if (!long_fmt) kputc('\n');
        return true;
    }

    if (kstrcmp(abs, OV4_S("/sys/kernel")) == 0 || kstrcmp(abs, OV4_S("/proc/sys/kernel")) == 0) {
        print_virtual_entry_ov(OV4_S("hostname"), false, long_fmt);
        print_virtual_entry_ov(OV4_S("ostype"), false, long_fmt);
        print_virtual_entry_ov(OV4_S("osrelease"), false, long_fmt);
        print_virtual_entry_ov(OV4_S("version"), false, long_fmt);
        print_virtual_entry_ov(OV4_S("printk"), false, long_fmt);
        if (!long_fmt) kputc('\n');
        return true;
    }

    return false;
}

/* =========================================================================
 * OVERLAY 5 (page 4): drivers/tty/n_tty.c & BusyBox /bin/sh Shell Engine
 * ========================================================================= */

OV5_ENTRY static void vt100_draw_bar_ov(void) {
    if (!vt100_bar_enabled) return;
    uint8_t saved_redir = redir_mode;
    redir_mode = 0U;
    update_jiffies();
    struct sysinfo si;
    si_meminfo(&si);
    kputs(OV5_S("\x1b[s\x1b[1;1H\x1b[44;37;1m uCLinux 6.6.0-mos65c02 | up "));
    kprint_u32(jiffies / 100UL);
    kputs(OV5_S("s | free "));
    kprint_u16(si.freeram);
    kputs(OV5_S("/"));
    kprint_u16(si.totalram);
    kputs(OV5_S("B | ttyS0 19200n8 \x1b[K\x1b[0m\x1b[u"));
    redir_mode = saved_redir;
}

OV5 static void shell_print_prompt_ov(void) {
    uint8_t saved = redir_mode;
    redir_mode = 0U;
    char cwd[32];
    vfs_build_cwd(cwd, sizeof(cwd));
    const char* disp = cwd;
    if (current_uid == 0U && kstrcmp(cwd, OV5_S("/root")) == 0) {
        disp = OV5_S("~");
    }
    kputs(current_uid == 0U ? OV5_S("\x1b[32;1mroot@") : OV5_S("\x1b[36;1mnobody@"));
    kputs(init_uts_ns.nodename);
    kputs(OV5_S("\x1b[0m:\x1b[34;1m"));
    kputs(disp);
    kputs(current_uid == 0U ? OV5_S("\x1b[0m# ") : OV5_S("\x1b[0m$ "));
    redir_mode = saved;
}

OV5 static void shell_redraw_line_ov(const char* buf, uint8_t len, uint8_t cur) {
    kputc('\r');
    shell_print_prompt_ov();
    for (uint8_t i = 0U; i < len; i++) kputc(buf[i]);
    kputs(OV5_S("\x1b[K"));
    uint8_t back = (uint8_t)(len - cur);
    while (back > 0U) {
        kputc('\x08');
        back--;
    }
}

OV5 static void shell_tab_complete_ov(char* buf, uint8_t* len_io, uint8_t* cur_io, uint8_t max_len) {
    uint8_t len = *len_io;
    uint8_t start = len;
    while (start > 0U && buf[start - 1U] != ' ') start--;
    const char* prefix = &buf[start];
    uint8_t plen = (uint8_t)(len - start);
    if (plen == 0U) return;

    /* Match against VFS inodes in current working directory */
    for (uint8_t i = 1U; i < MAX_INODES; i++) {
        if (inode_table[i].mode != 0U && inode_table[i].parent_ino == cwd_ino) {
            if (kstrncmp(inode_table[i].name, prefix, plen) == 0) {
                const char* rest = &inode_table[i].name[plen];
                while (*rest && len < max_len - 2U) {
                    buf[len++] = *rest;
                    kputc(*rest);
                    rest++;
                }
                if (S_ISDIR(inode_table[i].mode) && len < max_len - 1U) {
                    buf[len++] = '/';
                    kputc('/');
                }
                buf[len] = '\0';
                *len_io = len;
                *cur_io = len;
                return;
            }
        }
    }

    /* Match against common commands if at first token */
    if (start == 0U) {
        static const char* const cmds[] __attribute__((section(".overlay_5.rodata"))) = {
            "busybox", "cat", "cd", "chmod", "clear", "cp", "date", "dd", "df",
            "dmesg", "echo", "env", "exit", "export", "find", "free", "grep",
            "halt", "head", "help", "hexdump", "hostname", "id", "ifconfig",
            "kill", "ls", "lsdev", "lsmod", "mkdir", "mount", "mv", "panic",
            "ping", "poweroff", "ps", "pwd", "reboot", "rm", "rmdir", "stat",
            "stty", "su", "sync", "sysctl", "tail", "top", "touch", "umount",
            "uname", "unset", "uptime", "vi", "wc", "whoami"
        };
        for (uint8_t c = 0U; c < sizeof(cmds) / sizeof(cmds[0]); c++) {
            if (kstrncmp(cmds[c], prefix, plen) == 0) {
                const char* rest = &cmds[c][plen];
                while (*rest && len < max_len - 2U) {
                    buf[len++] = *rest;
                    kputc(*rest);
                    rest++;
                }
                buf[len++] = ' ';
                kputc(' ');
                buf[len] = '\0';
                *len_io = len;
                *cur_io = len;
                return;
            }
        }
    }
}

/* Port of drivers/tty/n_tty.c + BusyBox line editing */
OV5 static int8_t shell_read_line_ov(char* buf, uint8_t max_len) {
    uint8_t len = 0U;
    uint8_t cur = 0U;
    int8_t hist_idx = (int8_t)hist_count;
    buf[0] = '\0';

    while (kernel_running) {
        int16_t ch = kgetc_nb();
        if (ch < 0) continue;

        if (ch == '\r' || ch == '\n') {
            kputc('\n');
            buf[len] = '\0';
            if (len > 0U) {
                if (hist_count == 0U || kstrcmp(cmd_hist[hist_count - 1U], buf) != 0) {
                    if (hist_count < HIST_MAX) {
                        kstrcpy_lim(cmd_hist[hist_count++], buf, HIST_LINE_LEN);
                    } else {
                        for (uint8_t i = 1U; i < HIST_MAX; i++) {
                            kstrcpy_lim(cmd_hist[i - 1U], cmd_hist[i], HIST_LINE_LEN);
                        }
                        kstrcpy_lim(cmd_hist[HIST_MAX - 1U], buf, HIST_LINE_LEN);
                    }
                }
            }
            return (int8_t)len;
        }

        if (ch == 0x03) { /* Ctrl+C -> SIGINT */
            kputs(OV5_S("^C\n"));
            buf[0] = '\0';
            last_exit_code = 130;
            return 0;
        }

        if (ch == 0x04) { /* Ctrl+D -> EOF if line is empty */
            if (len == 0U) {
                kputs(OV5_S("exit\n"));
                return -1;
            }
            continue;
        }

        if (ch == 0x0C) { /* Ctrl+L -> Clear screen */
            kputs(OV5_S("\x1b[2J\x1b[H"));
            if (vt100_bar_enabled) {
                kputs(OV5_S("\x1b[2;24r\x1b[2;1H"));
                vt100_draw_bar_ov();
            }
            shell_redraw_line_ov(buf, len, cur);
            continue;
        }

        if (ch == 0x01) { /* Ctrl+A -> Home */
            while (cur > 0U) { kputc('\x08'); cur--; }
            continue;
        }

        if (ch == 0x05) { /* Ctrl+E -> End */
            while (cur < len) { kputc(buf[cur]); cur++; }
            continue;
        }

        if (ch == 0x15) { /* Ctrl+U -> Kill line */
            len = 0U;
            cur = 0U;
            buf[0] = '\0';
            shell_redraw_line_ov(buf, len, cur);
            continue;
        }

        if (ch == '\t') {
            if (cur == len) {
                shell_tab_complete_ov(buf, &len, &cur, max_len);
            }
            continue;
        }

        if (ch == 0x08 || ch == 0x7F) { /* Backspace */
            if (cur > 0U) {
                for (uint8_t i = (uint8_t)(cur - 1U); i < len - 1U; i++) {
                    buf[i] = buf[i + 1U];
                }
                len--;
                cur--;
                buf[len] = '\0';
                shell_redraw_line_ov(buf, len, cur);
            }
            continue;
        }

        if (ch == 0x1B) { /* ANSI escape sequence */
            uint16_t spin = 0U;
            int16_t c1 = -1;
            while (spin++ < 1500U && (c1 = kgetc_nb()) < 0) {}
            if (c1 == '[' || c1 == 'O') {
                spin = 0U;
                int16_t c2 = -1;
                while (spin++ < 1500U && (c2 = kgetc_nb()) < 0) {}
                if (c2 == 'A') { /* Up arrow -> previous history */
                    if (hist_idx > 0) {
                        hist_idx--;
                        kstrcpy_lim(buf, cmd_hist[(uint8_t)hist_idx], max_len);
                        len = (uint8_t)kstrlen(buf);
                        cur = len;
                        shell_redraw_line_ov(buf, len, cur);
                    }
                } else if (c2 == 'B') { /* Down arrow -> next history */
                    if (hist_idx + 1 < (int8_t)hist_count) {
                        hist_idx++;
                        kstrcpy_lim(buf, cmd_hist[(uint8_t)hist_idx], max_len);
                    } else {
                        hist_idx = (int8_t)hist_count;
                        buf[0] = '\0';
                    }
                    len = (uint8_t)kstrlen(buf);
                    cur = len;
                    shell_redraw_line_ov(buf, len, cur);
                } else if (c2 == 'D') { /* Left */
                    if (cur > 0U) { kputc('\x08'); cur--; }
                } else if (c2 == 'C') { /* Right */
                    if (cur < len) { kputc(buf[cur]); cur++; }
                } else if (c2 == 'H') { /* Home */
                    while (cur > 0U) { kputc('\x08'); cur--; }
                } else if (c2 == 'F') { /* End */
                    while (cur < len) { kputc(buf[cur]); cur++; }
                } else if (c2 == '3') { /* Delete (\x1b[3~) */
                    spin = 0U;
                    while (spin++ < 1000U && kgetc_nb() < 0) {}
                    if (cur < len) {
                        for (uint8_t i = cur; i < len - 1U; i++) buf[i] = buf[i + 1U];
                        len--;
                        buf[len] = '\0';
                        shell_redraw_line_ov(buf, len, cur);
                    }
                }
            }
            continue;
        }

        if (ch >= 32 && ch < 127 && len < max_len - 1U) {
            if (cur == len) {
                buf[len++] = (char)ch;
                cur++;
                buf[len] = '\0';
                kputc((char)ch);
            } else {
                for (uint8_t i = len; i > cur; i--) {
                    buf[i] = buf[i - 1U];
                }
                buf[cur] = (char)ch;
                len++;
                cur++;
                buf[len] = '\0';
                shell_redraw_line_ov(buf, len, cur);
            }
        }
    }
    return -1;
}

OV5 static void shell_expand_vars_ov(const char* src, char* dst, uint8_t max_len) {
    uint8_t di = 0U;
    bool in_sq = false;
    while (*src && di < max_len - 1U) {
        if (*src == '\'') {
            in_sq = !in_sq;
            dst[di++] = *src++;
            continue;
        }
        if (*src == '$' && !in_sq && src[1] != '\0' && src[1] != ' ') {
            src++;
            char num[12];
            const char* val = nullptr;
            if (*src == '?') {
                src++;
                fmt_u32_dec((uint32_t)(uint16_t)last_exit_code, num);
                val = num;
            } else if (*src == '$') {
                src++;
                fmt_u32_dec(active_pid, num);
                val = num;
            } else {
                char vname[16];
                uint8_t vi = 0U;
                while ((*src >= 'A' && *src <= 'Z') || (*src >= 'a' && *src <= 'z') ||
                       (*src >= '0' && *src <= '9') || *src == '_') {
                    if (vi < sizeof(vname) - 1U) vname[vi++] = *src;
                    src++;
                }
                vname[vi] = '\0';
                if (kstrcmp(vname, OV5_S("HOSTNAME")) == 0) {
                    val = init_uts_ns.nodename;
                } else {
                    val = env_get(vname);
                }
            }
            if (val) {
                while (*val && di < max_len - 1U) dst[di++] = *val++;
            }
            continue;
        }
        dst[di++] = *src++;
    }
    dst[di] = '\0';
}

OV5 static int16_t shell_exec_single_ov(char* raw_cmd) {
    char expanded[CMD_LINE_MAX];
    shell_expand_vars_ov(raw_cmd, expanded, sizeof(expanded));

    char* p = expanded;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '\0' || *p == '#') return 0;

    /* Check for standalone VAR=VALUE assignment */
    char* eq = nullptr;
    for (char* s = p; *s && *s != ' ' && *s != '\t'; s++) {
        if (*s == '=') { eq = s; break; }
    }
    if (eq) {
        char* after = eq + 1;
        while (*after && *after != ' ' && *after != '\t') after++;
        while (*after == ' ' || *after == '\t') after++;
        if (*after == '\0') {
            *eq = '\0';
            env_set(p, eq + 1);
            return 0;
        }
    }

    /* Tokenize arguments and extract redirections (<, >, >>) */
    sh_argc = 0U;
    char* out_file = nullptr;
    bool out_append = false;
    char* in_file = nullptr;

    while (*p && sh_argc < MAX_ARGS_SH) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;

        if (*p == '>') {
            *p++ = '\0';
            if (*p == '>') { out_append = true; *p++ = '\0'; }
            else { out_append = false; }
            while (*p == ' ' || *p == '\t') p++;
            out_file = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) *p++ = '\0';
            continue;
        }
        if (*p == '<') {
            *p++ = '\0';
            while (*p == ' ' || *p == '\t') p++;
            in_file = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) *p++ = '\0';
            continue;
        }

        if (*p == '"' || *p == '\'') {
            char q = *p++;
            sh_argv[sh_argc++] = p;
            while (*p && *p != q) p++;
            if (*p == q) *p++ = '\0';
        } else {
            sh_argv[sh_argc++] = p;
            while (*p && *p != ' ' && *p != '\t' && *p != '>' && *p != '<') p++;
            if (*p == ' ' || *p == '\t') *p++ = '\0';
        }
    }

    if (sh_argc == 0U) return 0;

    /* Handle input redirection (< file) into pipe_in_buf */
    bool saved_stdin_pipe = stdin_from_pipe;
    if (in_file && in_file[0]) {
        int16_t rn = vfs_read_by_path(kpath_copy(in_file), pipe_in_buf, (uint16_t)(PIPE_BUF_CAP - 1U));
        if (rn < 0) {
            kputs(OV5_S("sh: can't open "));
            kprintln(in_file);
            return 1;
        }
        pipe_in_buf[rn] = '\0';
        pipe_in_len = (uint16_t)rn;
        pipe_in_pos = 0U;
        stdin_from_pipe = true;
    }

    /* Handle output redirection (> or >>) via pipe_buf capture */
    uint8_t saved_redir = redir_mode;
    if (out_file && out_file[0]) {
        pipe_len = 0U;
        pipe_buf[0] = '\0';
        redir_mode = 1U;
    }

    /* Strip leading /bin/ or /sbin/ if invoked by full path */
    const char* cmd = sh_argv[0];
    if (kstrncmp(cmd, OV5_S("/bin/"), 5) == 0) cmd += 5;
    else if (kstrncmp(cmd, OV5_S("/sbin/"), 6) == 0) cmd += 6;

    /* If invoked as `busybox <applet> ...`, shift argv */
    if (kstrcmp(cmd, OV5_S("busybox")) == 0 && sh_argc > 1U && sh_argv[1][0] != '-') {
        for (uint8_t i = 0U; i < sh_argc - 1U; i++) sh_argv[i] = sh_argv[i + 1U];
        sh_argc--;
        cmd = sh_argv[0];
    } else {
        sh_argv[0] = (char*)cmd;
    }

    int16_t rc = -1;

    /* Shell built-ins */
    if (kstrcmp(cmd, OV5_S(":")) == 0 || kstrcmp(cmd, OV5_S("true")) == 0) {
        rc = 0;
    } else if (kstrcmp(cmd, OV5_S("false")) == 0) {
        rc = 1;
    } else if (kstrcmp(cmd, OV5_S("pwd")) == 0) {
        char cwd[48];
        vfs_build_cwd(cwd, sizeof(cwd));
        kprintln(cwd);
        rc = 0;
    } else if (kstrcmp(cmd, OV5_S("cd")) == 0) {
        const char* target = (sh_argc > 1U) ? sh_argv[1] : env_get("HOME");
        if (!target || kstrcmp(target, OV5_S("~")) == 0) target = OV5_S("/root");
        uint8_t p_ino = 0U;
        int8_t ino = vfs_path_lookup(kpath_copy(target), &p_ino, scratch_arg);
        if (ino >= 0 && S_ISDIR(inode_table[(uint8_t)ino].mode)) {
            cwd_ino = (uint8_t)ino;
            char cwd[48];
            vfs_build_cwd(cwd, sizeof(cwd));
            env_set("PWD", cwd);
            rc = 0;
        } else {
            kputs(OV5_S("sh: cd: can't cd to "));
            kprintln(target);
            rc = 1;
        }
    } else if (kstrcmp(cmd, OV5_S("source")) == 0 || kstrcmp(cmd, OV5_S(".")) == 0 ||
               kstrcmp(cmd, OV5_S("sh")) == 0 || kstrcmp(cmd, OV5_S("ash")) == 0 ||
               kstrcmp(cmd, OV5_S("exec")) == 0) {
        if (sh_argc > 1U) {
            rc = shell_run_script_ov(kpath_copy(sh_argv[1]), false) ? 0 : 1;
        } else {
            rc = 0;
        }
    } else {
        /* Try Overlay 6, Overlay 7, then Overlay 8 */
        rc = bb_file_applets();
        if (rc == -32768) {
            rc = bb_sys_applets();
        }
        if (rc == -32768) {
            rc = bb_ext_applets();
        }
        if (rc == -32768) {
            /* Check if sh_argv[0] is an executable script in VFS */
            if (shell_run_script_ov(kpath_copy(sh_argv[0]), false)) {
                rc = 0;
            } else {
                kputs(OV5_S("sh: "));
                kputs(cmd);
                kprintln(OV5_S(": not found"));
                rc = 127;
            }
        }
    }

    tasks[6].ticks++;

    if (out_file && out_file[0]) {
        redir_mode = saved_redir;
        if (!vfs_write_by_path(kpath_copy(out_file), pipe_buf, pipe_len, out_append)) {
            kputs(OV5_S("sh: can't create "));
            kprintln(out_file);
            rc = 1;
        }
    }
    stdin_from_pipe = saved_stdin_pipe;
    last_exit_code = rc;
    return rc;
}

OV5 static int16_t shell_exec_pipeline_ov(char* line) {
    /* Split pipeline on unquoted '|' */
    char* stages[4];
    uint8_t nstages = 0U;
    stages[nstages++] = line;
    bool in_q = false;
    for (char* p = line; *p && nstages < 4U; p++) {
        if (*p == '"' || *p == '\'') in_q = !in_q;
        else if (*p == '|' && !in_q) {
            *p = '\0';
            stages[nstages++] = p + 1;
        }
    }

    if (nstages == 1U) {
        stdin_from_pipe = false;
        return shell_exec_single_ov(stages[0]);
    }

    int16_t rc = 0;
    for (uint8_t i = 0U; i < nstages; i++) {
        bool is_first = (i == 0U);
        bool is_last  = (i == nstages - 1U);

        stdin_from_pipe = !is_first;
        if (!is_last) {
            pipe_len = 0U;
            pipe_buf[0] = '\0';
            redir_mode = 1U;
        } else {
            redir_mode = 0U;
        }

        rc = shell_exec_single_ov(stages[i]);

        if (!is_last) {
            memcpy(pipe_in_buf, pipe_buf, pipe_len);
            pipe_in_buf[pipe_len] = '\0';
            pipe_in_len = pipe_len;
            pipe_in_pos = 0U;
        }
    }
    redir_mode = 0U;
    stdin_from_pipe = false;
    return rc;
}

OV5_ENTRY static int16_t shell_exec_line_ov(char* line) {
    /* Strip unquoted comment '#' */
    bool in_q = false;
    for (char* p = line; *p; p++) {
        if (*p == '"' || *p == '\'') in_q = !in_q;
        else if (*p == '#' && !in_q) { *p = '\0'; break; }
    }

    char* cur = line;
    uint8_t prev_op = 0U; /* 0=none, 1=;, 2=&&, 3=|| */
    int16_t rc = 0;

    while (*cur) {
        while (*cur == ' ' || *cur == '\t') cur++;
        if (!*cur) break;

        char* seg = cur;
        uint8_t next_op = 0U;
        in_q = false;
        while (*cur) {
            if (*cur == '"' || *cur == '\'') in_q = !in_q;
            else if (!in_q) {
                if (*cur == ';') {
                    *cur++ = '\0';
                    next_op = 1U;
                    break;
                }
                if (cur[0] == '&' && cur[1] == '&') {
                    *cur = '\0';
                    cur += 2;
                    next_op = 2U;
                    break;
                }
                if (cur[0] == '|' && cur[1] == '|') {
                    *cur = '\0';
                    cur += 2;
                    next_op = 3U;
                    break;
                }
            }
            cur++;
        }

        bool should_run = true;
        if (prev_op == 2U && rc != 0) should_run = false;
        if (prev_op == 3U && rc == 0) should_run = false;

        if (should_run && *seg) {
            rc = shell_exec_pipeline_ov(seg);
        }
        prev_op = next_op;
    }
    return rc;
}

OV5_ENTRY static bool shell_run_script_ov(const char* path, bool verbose_rc) {
    char sbuf[160];
    int16_t n = vfs_read_by_path(kpath_copy(path), sbuf, (uint16_t)(sizeof(sbuf) - 1U));
    if (n <= 0) return false;
    sbuf[n] = '\0';

    if (verbose_rc) {
        printk(KERN_INFO, OV5_S("init: executing /etc/init.d/rcS startup script"));
    }

    char* p = sbuf;
    while (*p) {
        char* line = p;
        while (*p && *p != '\n' && *p != '\r') p++;
        while (*p == '\n' || *p == '\r') *p++ = '\0';
        if (line[0] && line[0] != '#') {
            char lcopy[CMD_LINE_MAX];
            kstrcpy_lim(lcopy, line, sizeof(lcopy));
            shell_exec_line_ov(lcopy);
        }
    }
    return true;
}

OV5_ENTRY static void shell_run_loop_ov(void) {
    if (vt100_bar_enabled) {
        kputs(OV5_S("\x1b[2;24r\x1b[24;1H"));
        vt100_draw_bar_ov();
    }

    while (kernel_running) {
        vt100_draw_bar_ov();
        shell_print_prompt_ov();
        int8_t len = shell_read_line_ov(cmd_line_buf, sizeof(cmd_line_buf));
        if (len < 0) break;
        if (len > 0) {
            shell_exec_line_ov(cmd_line_buf);
        }
    }
}


/* =========================================================================
 * OVERLAY 6 (page 5): BusyBox File, Text, Binary & Editor Applets
 * ========================================================================= */

OV6 static void format_mode_str_ov(uint16_t mode, char* out) {
    if (S_ISDIR(mode)) out[0] = 'd';
    else if (S_ISCHR(mode)) out[0] = 'c';
    else if (S_ISBLK(mode)) out[0] = 'b';
    else if (S_ISLNK(mode)) out[0] = 'l';
    else out[0] = '-';

    static const char rwx[] __attribute__((section(".overlay_6.rodata"))) = "rwxrwxrwx";
    for (uint8_t i = 0U; i < 9U; i++) {
        uint16_t bit = (uint16_t)(1U << (8U - i));
        out[i + 1U] = (mode & bit) ? rwx[i] : '-';
    }
    out[10] = '\0';
}

OV6 static void ls_print_inode_ov(const struct inode* nd, bool long_fmt) {
    if (long_fmt) {
        char mstr[12];
        format_mode_str_ov(nd->mode, mstr);
        kputs(mstr);
        kputs(nd->uid == 0U ? OV6_S("  1 root root ") : OV6_S("  1 nobody nogrp "));
        if (S_ISCHR(nd->mode) || S_ISBLK(nd->mode)) {
            kprint_u16(MAJOR(nd->rdev));
            kputs(OV6_S(", "));
            kprint_u16(MINOR(nd->rdev));
            kputc(' ');
        } else {
            if (nd->size < 1000U) kputc(' ');
            if (nd->size < 100U)  kputc(' ');
            if (nd->size < 10U)   kputc(' ');
            kprint_u16(nd->size);
            kputc(' ');
        }
    }

    if (S_ISDIR(nd->mode)) kputs(OV6_S("\x1b[34;1m"));
    else if (S_ISCHR(nd->mode) || S_ISBLK(nd->mode)) kputs(OV6_S("\x1b[33;1m"));
    else if (S_ISLNK(nd->mode)) kputs(OV6_S("\x1b[36;1m"));
    else if (nd->mode & 0111U) kputs(OV6_S("\x1b[32;1m"));

    kputs(nd->name);
    kputs(OV6_S("\x1b[0m"));

    if (long_fmt) {
        if (S_ISLNK(nd->mode) && nd->data_off != 0xFFFFU && nd->size > 0U) {
            kputs(OV6_S(" -> "));
            for (uint16_t j = 0U; j < nd->size; j++) {
                kputc((char)ramfs_pool[nd->data_off + j]);
            }
        }
        kputc('\n');
    } else {
        kputs(OV6_S("  "));
    }
}

OV6 static int16_t cmd_ls_ov(void) {
    bool long_fmt = false;
    bool show_all = false;
    const char* target = nullptr;

    for (uint8_t i = 1U; i < sh_argc; i++) {
        if (sh_argv[i][0] == '-') {
            for (uint8_t j = 1U; sh_argv[i][j]; j++) {
                if (sh_argv[i][j] == 'l') long_fmt = true;
                else if (sh_argv[i][j] == 'a') show_all = true;
            }
        } else {
            target = sh_argv[i];
        }
    }

    char path[64];
    if (!target) {
        vfs_build_cwd(path, sizeof(path));
    } else {
        kstrcpy_lim(path, target, sizeof(path));
    }

    if (proc_sys_listdir(kpath_copy(path), long_fmt)) {
        return 0;
    }

    /* Check /mnt/sd FAT16 directory listing */
    if ((mnt_flags & MNT_SD) && (kstrcmp(path, OV6_S("/mnt/sd")) == 0 || kstrncmp(path, OV6_S("/mnt/sd/"), 8) == 0)) {
        const char* sd_sub = (path[7] == '/') ? &path[8] : OV6_S("");
        if (sd_opendir(&lnx_sd_dir, sd_sub) == 0) {
            while (sd_readdir(&lnx_sd_dir, &lnx_sd_fno) == 0 && lnx_sd_fno.fname[0] != '\0') {
                bool is_d = (lnx_sd_fno.fattrib & AM_DIR) != 0U;
                if (long_fmt) {
                    kputs(is_d ? OV6_S("drwxr-xr-x  1 root root ") : OV6_S("-rw-r--r--  1 root root "));
                    kprint_u32(lnx_sd_fno.fsize);
                    kputc(' ');
                }
                if (is_d) kputs(OV6_S("\x1b[34;1m"));
                kputs(lnx_sd_fno.fname);
                if (is_d) kputs(OV6_S("\x1b[0m"));
                kputs(long_fmt ? OV6_S("\n") : OV6_S("  "));
            }
            sd_closedir(&lnx_sd_dir);
            if (!long_fmt) kputc('\n');
            return 0;
        }
    }

    uint8_t p_ino = 0U;
    int8_t ino = vfs_path_lookup(kpath_copy(path), &p_ino, scratch_arg);
    if (ino < 0) {
        kputs(OV6_S("ls: cannot access '"));
        kputs(path);
        kprintln(OV6_S("': No such file or directory"));
        return 1;
    }

    if (!S_ISDIR(inode_table[(uint8_t)ino].mode)) {
        ls_print_inode_ov(&inode_table[(uint8_t)ino], long_fmt);
        if (!long_fmt) kputc('\n');
        return 0;
    }

    if (show_all) {
        if (long_fmt) {
            kprintln(OV6_S("drwxr-xr-x  2 root root    0 \x1b[34;1m.\x1b[0m"));
            kprintln(OV6_S("drwxr-xr-x  2 root root    0 \x1b[34;1m..\x1b[0m"));
        } else {
            kputs(OV6_S("\x1b[34;1m.  ..\x1b[0m  "));
        }
    }

    for (uint8_t i = 1U; i < MAX_INODES; i++) {
        if (inode_table[i].mode != 0U && inode_table[i].parent_ino == (uint8_t)ino) {
            if (!show_all && inode_table[i].name[0] == '.') continue;
            ls_print_inode_ov(&inode_table[i], long_fmt);
        }
    }
    if (!long_fmt) kputc('\n');
    return 0;
}

OV6 static int16_t cmd_cat_ov(void) {
    if (sh_argc == 1U) {
        if (stdin_from_pipe) {
            for (uint16_t i = 0U; i < pipe_in_len; i++) kputc(pipe_in_buf[i]);
        }
        return 0;
    }

    int16_t rc = 0;
    for (uint8_t a = 1U; a < sh_argc; a++) {
        const char* path = sh_argv[a];
        if (proc_sys_handle(kpath_copy(path), false, nullptr)) {
            continue;
        }
        int16_t n = vfs_read_by_path(kpath_copy(path), scratch_buf, sizeof(scratch_buf));
        if (n < 0) {
            kputs(OV6_S("cat: can't open '"));
            kputs(path);
            kprintln(OV6_S("': No such file or directory"));
            rc = 1;
            continue;
        }
        for (int16_t i = 0; i < n; i++) kputc(scratch_buf[i]);
    }
    return rc;
}

OV6 static int16_t cmd_echo_ov(void) {
    bool no_nl = false;
    bool interp = false;
    uint8_t idx = 1U;
    while (idx < sh_argc && sh_argv[idx][0] == '-') {
        if (kstrcmp(sh_argv[idx], OV6_S("-n")) == 0) no_nl = true;
        else if (kstrcmp(sh_argv[idx], OV6_S("-e")) == 0) interp = true;
        else if (kstrcmp(sh_argv[idx], OV6_S("-ne")) == 0 || kstrcmp(sh_argv[idx], OV6_S("-en")) == 0) {
            no_nl = true;
            interp = true;
        } else break;
        idx++;
    }

    for (uint8_t i = idx; i < sh_argc; i++) {
        if (i > idx) kputc(' ');
        const char* s = sh_argv[i];
        while (*s) {
            if (interp && *s == '\\' && s[1]) {
                s++;
                if (*s == 'n') kputc('\n');
                else if (*s == 't') kputc('\t');
                else if (*s == 'r') kputc('\r');
                else if (*s == '\\') kputc('\\');
                else kputc(*s);
                s++;
            } else {
                kputc(*s++);
            }
        }
    }
    if (!no_nl) kputc('\n');
    return 0;
}

OV6 static uint16_t parse_octal_ov(const char* s) {
    uint16_t v = 0U;
    while (*s >= '0' && *s <= '7') {
        v = (uint16_t)((v << 3) | (uint8_t)(*s - '0'));
        s++;
    }
    return v;
}

OV6_ENTRY static int16_t bb_file_applets_ov(void) {
    const char* cmd = sh_argv[0];

    if (kstrcmp(cmd, OV6_S("ls")) == 0) return cmd_ls_ov();
    if (kstrcmp(cmd, OV6_S("cat")) == 0) return cmd_cat_ov();
    if (kstrcmp(cmd, OV6_S("echo")) == 0) return cmd_echo_ov();

    if (kstrcmp(cmd, OV6_S("mkdir")) == 0) {
        for (uint8_t i = 1U; i < sh_argc; i++) {
            if (sh_argv[i][0] == '-') continue;
            uint8_t p_ino = 0U;
            int8_t ex = vfs_path_lookup(kpath_copy(sh_argv[i]), &p_ino, scratch_arg);
            if (ex < 0) {
                if (vfs_create_node(p_ino, scratch_arg, (uint16_t)(S_IFDIR | 0755U), 0U) < 0) {
                    kprintln(OV6_S("mkdir: cannot create directory"));
                    return 1;
                }
            }
        }
        return 0;
    }

    if (kstrcmp(cmd, OV6_S("touch")) == 0) {
        for (uint8_t i = 1U; i < sh_argc; i++) {
            uint8_t p_ino = 0U;
            int8_t ex = vfs_path_lookup(kpath_copy(sh_argv[i]), &p_ino, scratch_arg);
            if (ex < 0) vfs_create_node(p_ino, scratch_arg, (uint16_t)(S_IFREG | 0644U), 0U);
        }
        return 0;
    }

    if (kstrcmp(cmd, OV6_S("rm")) == 0 || kstrcmp(cmd, OV6_S("rmdir")) == 0) {
        bool rec = (kstrcmp(cmd, OV6_S("rm")) == 0);
        for (uint8_t i = 1U; i < sh_argc; i++) {
            if (sh_argv[i][0] == '-') continue;
            if (!vfs_unlink_path(kpath_copy(sh_argv[i]), rec)) {
                kputs(cmd); kputs(OV6_S(": cannot remove '")); kputs(sh_argv[i]); kprintln(OV6_S("'"));
                return 1;
            }
        }
        return 0;
    }

    if (kstrcmp(cmd, OV6_S("cp")) == 0 || kstrcmp(cmd, OV6_S("mv")) == 0) {
        if (sh_argc < 3U) return 1;
        int16_t n = vfs_read_by_path(kpath_copy(sh_argv[1]), scratch_buf, sizeof(scratch_buf));
        if (n < 0) return 1;
        if (!vfs_write_by_path(kpath_copy(sh_argv[2]), scratch_buf, (uint16_t)n, false)) return 1;
        if (kstrcmp(cmd, OV6_S("mv")) == 0) {
            vfs_unlink_path(kpath_copy(sh_argv[1]), false);
        }
        return 0;
    }

    if (kstrcmp(cmd, OV6_S("chmod")) == 0) {
        if (sh_argc < 3U) return 1;
        uint8_t p_ino = 0U;
        int8_t ino = vfs_path_lookup(kpath_copy(sh_argv[2]), &p_ino, scratch_arg);
        if (ino < 0) return 1;
        if (kstrcmp(sh_argv[1], OV6_S("+x")) == 0) {
            inode_table[(uint8_t)ino].mode |= 0111U;
        } else {
            uint16_t perm = parse_octal_ov(sh_argv[1]) & 07777U;
            inode_table[(uint8_t)ino].mode = (uint16_t)((inode_table[(uint8_t)ino].mode & S_IFMT) | perm);
        }
        return 0;
    }

    if (kstrcmp(cmd, OV6_S("stat")) == 0) {
        if (sh_argc < 2U) return 1;
        uint8_t p_ino = 0U;
        int8_t ino = vfs_path_lookup(kpath_copy(sh_argv[1]), &p_ino, scratch_arg);
        if (ino < 0) return 1;
        struct inode* nd = &inode_table[(uint8_t)ino];
        kputs(OV6_S("  File: ")); kprintln(nd->name);
        kputs(OV6_S("  Size: ")); kprint_u16(nd->size);
        kputs(OV6_S("\tInode: ")); kprint_u16(nd->ino);
        kputs(OV6_S("\tMode: 0"));
        kprint_u16((uint16_t)((nd->mode >> 6) & 7U));
        kprint_u16((uint16_t)((nd->mode >> 3) & 7U));
        kprint_u16((uint16_t)(nd->mode & 7U));
        kputc('\n');
        return 0;
    }

    if (kstrcmp(cmd, OV6_S("find")) == 0) {
        for (uint8_t i = 0U; i < MAX_INODES; i++) {
            if (inode_table[i].mode != 0U) {
                if (i == 0U) { kprintln(OV6_S("/")); continue; }
                uint8_t saved_cwd = cwd_ino;
                cwd_ino = i;
                char pbuf[48];
                vfs_build_cwd(pbuf, sizeof(pbuf));
                cwd_ino = saved_cwd;
                kprintln(pbuf);
            }
        }
        return 0;
    }

    return -32768;
}

/* =========================================================================
 * OVERLAY 8 (page 7): BusyBox Text, Editor, Binary, Network & HW Applets
 * ========================================================================= */

OV8 static int16_t cmd_grep_ov(void) {
    bool ignore_case = false;
    bool line_nums = false;
    bool invert = false;
    uint8_t idx = 1U;
    while (idx < sh_argc && sh_argv[idx][0] == '-') {
        for (uint8_t j = 1U; sh_argv[idx][j]; j++) {
            if (sh_argv[idx][j] == 'i') ignore_case = true;
            else if (sh_argv[idx][j] == 'n') line_nums = true;
            else if (sh_argv[idx][j] == 'v') invert = true;
        }
        idx++;
    }
    if (idx >= sh_argc) return 1;
    const char* pat = sh_argv[idx++];

    const char* text = pipe_in_buf;
    uint16_t tlen = pipe_in_len;
    if (idx < sh_argc) {
        if (kstrncmp(sh_argv[idx], OV8_S("/proc/"), 6) == 0 || kstrncmp(sh_argv[idx], OV8_S("/sys/"), 5) == 0) {
            uint8_t sr = redir_mode;
            pipe_len = 0U;
            redir_mode = 1U;
            proc_sys_handle(kpath_copy(sh_argv[idx]), false, nullptr);
            redir_mode = sr;
            memcpy(pipe_in_buf, pipe_buf, pipe_len);
            pipe_in_buf[pipe_len] = '\0';
            tlen = pipe_len;
        } else {
            int16_t n = vfs_read_by_path(kpath_copy(sh_argv[idx]), pipe_in_buf, (uint16_t)(PIPE_BUF_CAP - 1U));
            if (n < 0) {
                kputs(OV8_S("grep: ")); kputs(sh_argv[idx]); kprintln(OV8_S(": No such file"));
                return 2;
            }
            pipe_in_buf[n] = '\0';
            tlen = (uint16_t)n;
        }
    }

    uint16_t pos = 0U;
    uint16_t lno = 1U;
    int16_t matched_any = 1;
    size_t plen = kstrlen(pat);

    while (pos < tlen) {
        uint16_t lstart = pos;
        while (pos < tlen && text[pos] != '\n') pos++;
        uint16_t llen = (uint16_t)(pos - lstart);
        if (pos < tlen && text[pos] == '\n') pos++;

        bool found = false;
        if (plen == 0U) {
            found = true;
        } else if (llen >= plen) {
            for (uint16_t i = 0U; i <= llen - plen; i++) {
                bool ok = true;
                for (uint16_t k = 0U; k < plen; k++) {
                    char c1 = text[lstart + i + k];
                    char c2 = pat[k];
                    if (ignore_case) {
                        if (c1 >= 'A' && c1 <= 'Z') c1 = (char)(c1 + 32);
                        if (c2 >= 'A' && c2 <= 'Z') c2 = (char)(c2 + 32);
                    }
                    if (c1 != c2) { ok = false; break; }
                }
                if (ok) { found = true; break; }
            }
        }

        if (invert) found = !found;
        if (found) {
            matched_any = 0;
            if (line_nums) {
                kprint_u16(lno);
                kputc(':');
            }
            for (uint16_t i = 0U; i < llen; i++) kputc(text[lstart + i]);
            kputc('\n');
        }
        lno++;
    }
    return matched_any;
}

OV8 static int16_t cmd_wc_ov(void) {
    const char* text = pipe_in_buf;
    uint16_t tlen = pipe_in_len;
    const char* fname = nullptr;
    for (uint8_t i = 1U; i < sh_argc; i++) {
        if (sh_argv[i][0] != '-') fname = sh_argv[i];
    }
    if (fname) {
        int16_t n = vfs_read_by_path(kpath_copy(fname), pipe_in_buf, (uint16_t)(PIPE_BUF_CAP - 1U));
        if (n < 0) return 1;
        tlen = (uint16_t)n;
    }

    uint16_t lines = 0U, words = 0U;
    bool in_w = false;
    for (uint16_t i = 0U; i < tlen; i++) {
        char c = text[i];
        if (c == '\n') lines++;
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') in_w = false;
        else if (!in_w) { in_w = true; words++; }
    }
    kputs(OV8_S("  ")); kprint_u16(lines);
    kputs(OV8_S("  ")); kprint_u16(words);
    kputs(OV8_S("  ")); kprint_u16(tlen);
    if (fname) { kputc(' '); kputs(fname); }
    kputc('\n');
    return 0;
}

OV8 static int16_t cmd_hexdump_ov(void) {
    const char* fname = (sh_argc > 1U && sh_argv[1][0] != '-') ? sh_argv[1] : ((sh_argc > 2U) ? sh_argv[2] : nullptr);
    int16_t n = 0;
    if (fname) {
        n = vfs_read_by_path(kpath_copy(fname), scratch_buf, 64U);
        if (n < 0) return 1;
    } else if (stdin_from_pipe) {
        n = (pipe_in_len > 64U) ? 64 : (int16_t)pipe_in_len;
        memcpy(scratch_buf, pipe_in_buf, (uint16_t)n);
    }

    for (uint16_t off = 0U; off < (uint16_t)n; off = (uint16_t)(off + 16U)) {
        kprint_hex16(off);
        kputs(OV8_S("  "));
        for (uint8_t j = 0U; j < 16U; j++) {
            if (off + j < (uint16_t)n) {
                kprint_hex8((uint8_t)scratch_buf[off + j]);
                kputc(' ');
            } else {
                kputs(OV8_S("   "));
            }
        }
        kputs(OV8_S(" |"));
        for (uint8_t j = 0U; j < 16U && (off + j) < (uint16_t)n; j++) {
            char c = scratch_buf[off + j];
            kputc((c >= 32 && c < 127) ? c : '.');
        }
        kprintln(OV8_S("|"));
    }
    return 0;
}

OV8 static int16_t cmd_vi_ov(void) {
    if (sh_argc < 2U) {
        kprintln(OV8_S("vi: usage: vi <file>"));
        return 1;
    }
    char target[48];
    kstrcpy_lim(target, sh_argv[1], sizeof(target));
    int16_t len = vfs_read_by_path(kpath_copy(target), pipe_buf, (uint16_t)(PIPE_BUF_CAP - 1U));
    if (len < 0) len = 0;
    pipe_buf[len] = '\0';

    kputs(OV8_S("\x1b[33;1m--- BusyBox vi/ed ["));
    kputs(target);
    kprintln(OV8_S("] (commands: p=print, a=append, c=clear, w=write, q=quit, wq=save&exit) ---\x1b[0m"));

    char lbuf[64];
    while (kernel_running) {
        kputs(OV8_S(":"));
        uint8_t li = 0U;
        while (kernel_running) {
            int16_t ch = kgetc_nb();
            if (ch < 0) continue;
            if (ch == '\r' || ch == '\n') { kputc('\n'); lbuf[li] = '\0'; break; }
            if ((ch == 0x08 || ch == 0x7F) && li > 0U) { li--; kputs(OV8_S("\x08 \x08")); continue; }
            if (ch >= 32 && ch < 127 && li < sizeof(lbuf) - 1U) { lbuf[li++] = (char)ch; kputc((char)ch); }
        }
        if (kstrcmp(lbuf, OV8_S("q")) == 0 || kstrcmp(lbuf, OV8_S(":q!")) == 0) break;
        if (kstrcmp(lbuf, OV8_S("p")) == 0) {
            kputs(pipe_buf);
            if (len > 0 && pipe_buf[len - 1] != '\n') kputc('\n');
        } else if (kstrcmp(lbuf, OV8_S("c")) == 0) {
            len = 0;
            pipe_buf[0] = '\0';
            kprintln(OV8_S("buffer cleared."));
        } else if (kstrcmp(lbuf, OV8_S("a")) == 0 || kstrcmp(lbuf, OV8_S("i")) == 0) {
            kprintln(OV8_S("(enter lines, single '.' to finish)"));
            while (kernel_running) {
                uint8_t ai = 0U;
                while (kernel_running) {
                    int16_t ch = kgetc_nb();
                    if (ch < 0) continue;
                    if (ch == '\r' || ch == '\n') { kputc('\n'); lbuf[ai] = '\0'; break; }
                    if ((ch == 0x08 || ch == 0x7F) && ai > 0U) { ai--; kputs(OV8_S("\x08 \x08")); continue; }
                    if (ch >= 32 && ch < 127 && ai < sizeof(lbuf) - 1U) { lbuf[ai++] = (char)ch; kputc((char)ch); }
                }
                if (kstrcmp(lbuf, OV8_S(".")) == 0) break;
                size_t al = kstrlen(lbuf);
                if ((uint16_t)len + al + 2U < PIPE_BUF_CAP) {
                    memcpy(&pipe_buf[len], lbuf, al);
                    len = (int16_t)(len + (int16_t)al);
                    pipe_buf[len++] = '\n';
                    pipe_buf[len] = '\0';
                }
            }
        } else if (kstrcmp(lbuf, OV8_S("w")) == 0 || kstrcmp(lbuf, OV8_S("wq")) == 0 || kstrcmp(lbuf, OV8_S(":wq")) == 0) {
            vfs_write_by_path(kpath_copy(target), pipe_buf, (uint16_t)len, false);
            kputs(OV8_S("\"")); kputs(target); kputs(OV8_S("\" ")); kprint_u16((uint16_t)len); kprintln(OV8_S("B written"));
            if (lbuf[1] == 'q' || lbuf[2] == 'q') break;
        }
    }
    return 0;
}

OV8_ENTRY static int16_t bb_ext_applets_ov(void) {
    const char* cmd = sh_argv[0];

    if (kstrcmp(cmd, OV8_S("grep")) == 0) return cmd_grep_ov();
    if (kstrcmp(cmd, OV8_S("wc")) == 0) return cmd_wc_ov();
    if (kstrcmp(cmd, OV8_S("hexdump")) == 0) return cmd_hexdump_ov();
    if (kstrcmp(cmd, OV8_S("vi")) == 0 || kstrcmp(cmd, OV8_S("ed")) == 0) return cmd_vi_ov();

    if (kstrcmp(cmd, OV8_S("head")) == 0 || kstrcmp(cmd, OV8_S("tail")) == 0) {
        const char* fname = (sh_argc > 1U) ? sh_argv[sh_argc - 1U] : nullptr;
        int16_t n = fname ? vfs_read_by_path(kpath_copy(fname), pipe_in_buf, (uint16_t)(PIPE_BUF_CAP - 1U)) : (int16_t)pipe_in_len;
        if (n < 0) return 1;
        uint8_t max_l = 5U;
        uint8_t cnt = 0U;
        for (int16_t i = 0; i < n && cnt < max_l; i++) {
            kputc(pipe_in_buf[i]);
            if (pipe_in_buf[i] == '\n') cnt++;
        }
        return 0;
    }

    if (kstrcmp(cmd, OV8_S("dd")) == 0) {
        const char* in_f = OV8_S("/dev/zero");
        const char* out_f = nullptr;
        uint16_t count = 1U;
        for (uint8_t i = 1U; i < sh_argc; i++) {
            if (kstrncmp(sh_argv[i], OV8_S("if="), 3) == 0) in_f = &sh_argv[i][3];
            else if (kstrncmp(sh_argv[i], OV8_S("of="), 3) == 0) out_f = &sh_argv[i][3];
            else if (kstrncmp(sh_argv[i], OV8_S("count="), 6) == 0) count = (uint16_t)(sh_argv[i][6] - '0');
        }
        int16_t br = vfs_read_by_path(kpath_copy(in_f), scratch_buf, (uint16_t)(count * 32U));
        if (br < 0) br = 0;
        if (out_f) vfs_write_by_path(kpath_copy(out_f), scratch_buf, (uint16_t)br, false);
        kprint_u16(count); kprintln(OV8_S("+0 records in"));
        kprint_u16(count); kprintln(OV8_S("+0 records out"));
        kprint_u16((uint16_t)br); kprintln(OV8_S(" bytes copied"));
        return 0;
    }

    if (kstrcmp(cmd, OV8_S("ifconfig")) == 0 || kstrcmp(cmd, OV8_S("ip")) == 0) {
        kprintln(OV8_S("lo        Link encap:Local Loopback"));
        kprintln(OV8_S("          inet addr:127.0.0.1  Mask:255.0.0.0"));
        kprintln(OV8_S("          UP LOOPBACK RUNNING  MTU:1500"));
        kprintln(OV8_S("wlan0     Link encap:Ethernet  HWaddr 5C:CF:7F:65:C0:02"));
        kprintln(OV8_S("          inet addr:192.168.1.65  Bcast:192.168.1.255  Mask:255.255.255.0"));
        kprintln(OV8_S("          UP BROADCAST RUNNING MULTICAST  MTU:1500  Interrupt:3 Base:0x7fc4"));
        return 0;
    }

    if (kstrcmp(cmd, OV8_S("ping")) == 0) {
        const char* host = (sh_argc > 1U) ? sh_argv[1] : OV8_S("127.0.0.1");
        kputs(OV8_S("PING ")); kputs(host); kprintln(OV8_S(" (56 data bytes)"));
        for (uint8_t seq = 0U; seq < 3U; seq++) {
            kputs(OV8_S("64 bytes from ")); kputs(host);
            kputs(OV8_S(": seq=")); kprint_u16(seq);
            kputs(OV8_S(" ttl=64 time=1.")); kprint_u16((uint16_t)(next_prng_byte() % 9U));
            kprintln(OV8_S(" ms"));
        }
        kputs(OV8_S("--- ")); kputs(host); kprintln(OV8_S(" ping statistics ---"));
        kprintln(OV8_S("3 packets transmitted, 3 packets received, 0% packet loss"));
        return 0;
    }

    if (kstrcmp(cmd, OV8_S("lsdev")) == 0) {
        kprintln(OV8_S("Device        Major Minor   MMIO       Description"));
        kprintln(OV8_S("ttyS0             4    64   0x7fc0     MOS 6850 ACIA Serial UART"));
        kprintln(OV8_S("wlan0            10   200   0x7fc4     ESP8266 AT WiFi Bridge"));
        kprintln(OV8_S("mmcblk0p1       179     1   0x7fc8     SPI SDHC Card (FAT16)"));
        kprintln(OV8_S("via6522          10   135   0x7fd0     VIA 6522 System Timer/GPIO"));
        kprintln(OV8_S("dsp              14     3   0x7fe0     MOS 6581/8580 SID Audio"));
        return 0;
    }

    if (kstrcmp(cmd, OV8_S("beep")) == 0) {
        char tone[2] = { 0x18, 0x40 };
        vfs_write_by_path(kpath_copy(OV8_S("/dev/dsp")), tone, 2U, false);
        return 0;
    }

    if (kstrcmp(cmd, OV8_S("lsmod")) == 0) {
        kprintln(OV8_S("Module                  Size  Used by"));
        proc_sys_handle(kpath_copy(OV8_S("/proc/modules")), false, nullptr);
        return 0;
    }

    if (kstrcmp(cmd, OV8_S("sysctl")) == 0) {
        if (sh_argc > 1U && kstrcmp(sh_argv[1], OV8_S("-a")) == 0) {
            kputs(OV8_S("kernel.hostname = ")); kprintln(init_uts_ns.nodename);
            kputs(OV8_S("kernel.ostype = "));   kprintln(init_uts_ns.sysname);
            kputs(OV8_S("kernel.osrelease = ")); kprintln(init_uts_ns.release);
            kputs(OV8_S("kernel.printk = "));   kprint_u16(console_loglevel); kputc('\n');
            return 0;
        }
        return 0;
    }

    if (kstrcmp(cmd, OV8_S("busybox")) == 0 || kstrcmp(cmd, OV8_S("help")) == 0) {
        kprintln(OV8_S("BusyBox v1.36.1 (uCLinux 6.6.0-mos65c02-nommu) multi-call binary."));
        kprintln(OV8_S("Currently defined applets:"));
        kprintln(OV8_S("  [, ash, beep, busybox, cat, cd, chmod, clear, cp, date, dd, df,"));
        kprintln(OV8_S("  dmesg, echo, ed, env, exec, exit, export, false, find, free, grep,"));
        kprintln(OV8_S("  halt, head, help, hexdump, hostname, htop, id, ifconfig, ip, kill,"));
        kprintln(OV8_S("  ls, lsdev, lsmod, mkdir, mount, mv, panic, ping, poweroff, printenv,"));
        kprintln(OV8_S("  ps, pwd, reboot, rm, rmdir, sh, source, stat, stty, su, sync,"));
        kprintln(OV8_S("  sysctl, tail, top, touch, true, umount, uname, unset, uptime, vi,"));
        kprintln(OV8_S("  wc, whoami"));
        return 0;
    }

    return -32768;
}

/* =========================================================================
 * OVERLAY 7 (page 6): BusyBox System, Process, Network & Admin Applets
 * ========================================================================= */

OV7 static int16_t cmd_ps_ov(void) {
    kprintln(OV7_S("  PID USER       VSZ STAT COMMAND"));
    for (uint8_t i = 0U; i < MAX_TASKS; i++) {
        if (tasks[i].pid != 0U) {
            if (tasks[i].pid < 10U) kputs(OV7_S("    "));
            else kputs(OV7_S("   "));
            kprint_u16(tasks[i].pid);
            kputs(tasks[i].uid == 0U ? OV7_S(" root      ") : OV7_S(" nobody    "));
            uint16_t vsz = (uint16_t)(tasks[i].rss_pages * 64U);
            if (vsz == 0U) kputs(OV7_S("   0 "));
            else { kputc(' '); kprint_u16(vsz); kputc(' '); }
            kputs(tasks[i].state == TASK_RUNNING ? OV7_S("R    ") : OV7_S("S    "));
            if (tasks[i].flags & PF_KTHREAD) {
                kputc('['); kputs(tasks[i].comm); kprintln(OV7_S("]"));
            } else {
                kprintln(tasks[i].comm);
            }
        }
    }
    return 0;
}

OV7 static int16_t cmd_top_ov(void) {
    while (kernel_running) {
        update_jiffies();
        struct sysinfo si;
        si_meminfo(&si);
        kputs(OV7_S("\x1b[2J\x1b[H\x1b[7mMem: "));
        kprint_u16((uint16_t)(si.totalram - si.freeram));
        kputs(OV7_S("B used, "));
        kprint_u16(si.freeram);
        kputs(OV7_S("B free, 56K paged | Uptime: "));
        kprint_u32(jiffies / 100UL);
        kprintln(OV7_S("s (press 'q' to exit)\x1b[0m"));
        cmd_ps_ov();

        for (uint16_t w = 0U; w < 6000U; w++) {
            int16_t ch = kgetc_nb();
            if (ch == 'q' || ch == 'Q' || ch == 0x03) {
                if (vt100_bar_enabled) vt100_draw_bar();
                return 0;
            }
        }
    }
    return 0;
}

OV7 static int16_t cmd_free_ov(void) {
    struct sysinfo si;
    si_meminfo(&si);
    kprintln(OV7_S("              total        used        free      shared  buff/cache   available"));
    kputs(OV7_S("Mem:           "));
    kprint_u16(si.totalram);
    kputs(OV7_S("        "));
    kprint_u16((uint16_t)(si.totalram - si.freeram));
    kputs(OV7_S("        "));
    kprint_u16(si.freeram);
    kputs(OV7_S("           0         "));
    kprint_u16(si.bufferram);
    kputs(OV7_S("        "));
    kprint_u16(si.freeram);
    kputc('\n');
    kprintln(OV7_S("Swap:             0           0           0"));
    return 0;
}

OV7 static int16_t cmd_df_ov(void) {
    struct sysinfo si;
    si_meminfo(&si);
    uint16_t used = (uint16_t)(si.totalram - si.freeram);
    uint16_t pct  = (uint16_t)((used * 100U) / si.totalram);
    kprintln(OV7_S("Filesystem           1B-blocks      Used Available Use% Mounted on"));
    kputs(OV7_S("rootfs                    "));
    kprint_u16(si.totalram);
    kputs(OV7_S("      "));
    kprint_u16(used);
    kputs(OV7_S("      "));
    kprint_u16(si.freeram);
    kputs(OV7_S("  "));
    kprint_u16(pct);
    kprintln(OV7_S("% /"));
    if (mnt_flags & MNT_DEV) kprintln(OV7_S("devtmpfs                    64        64         0 100% /dev"));
    if (mnt_flags & MNT_SD)  kprintln(OV7_S("/dev/mmcblk0p1        33554432     65536  33488896   1% /mnt/sd"));
    return 0;
}

OV7 static int16_t cmd_uname_ov(void) {
    bool all = false, s = false, n = false, r = false, v = false, m = false;
    if (sh_argc == 1U) s = true;
    for (uint8_t i = 1U; i < sh_argc; i++) {
        for (uint8_t j = 1U; sh_argv[i][0] == '-' && sh_argv[i][j]; j++) {
            char c = sh_argv[i][j];
            if (c == 'a') all = true;
            else if (c == 's') s = true;
            else if (c == 'n') n = true;
            else if (c == 'r') r = true;
            else if (c == 'v') v = true;
            else if (c == 'm') m = true;
        }
    }
    if (all || s) { kputs(init_uts_ns.sysname); kputc(' '); }
    if (all || n) { kputs(init_uts_ns.nodename); kputc(' '); }
    if (all || r) { kputs(init_uts_ns.release); kputc(' '); }
    if (all || v) { kputs(init_uts_ns.version); kputc(' '); }
    if (all || m) { kputs(init_uts_ns.machine); }
    if (all) { kputs(OV7_S(" GNU/Linux")); }
    kputc('\n');
    return 0;
}

OV7 static int16_t cmd_dmesg_ov(void) {
    if (sh_argc > 1U && kstrcmp(sh_argv[1], OV7_S("-c")) == 0) {
        log_next_idx = 0U;
        log_buf[0] = '\0';
        return 0;
    }
    for (uint16_t i = 0U; i < log_next_idx; i++) {
        kputc(log_buf[i]);
    }
    return 0;
}

OV7_ENTRY static int16_t bb_sys_applets_ov(void) {
    const char* cmd = sh_argv[0];

    if (kstrcmp(cmd, OV7_S("ps")) == 0) return cmd_ps_ov();
    if (kstrcmp(cmd, OV7_S("top")) == 0 || kstrcmp(cmd, OV7_S("htop")) == 0) return cmd_top_ov();
    if (kstrcmp(cmd, OV7_S("free")) == 0) return cmd_free_ov();
    if (kstrcmp(cmd, OV7_S("df")) == 0) return cmd_df_ov();
    if (kstrcmp(cmd, OV7_S("uname")) == 0) return cmd_uname_ov();
    if (kstrcmp(cmd, OV7_S("dmesg")) == 0) return cmd_dmesg_ov();

    if (kstrcmp(cmd, OV7_S("mount")) == 0) {
        if (sh_argc == 1U) {
            proc_sys_handle(kpath_copy(OV7_S("/proc/mounts")), false, nullptr);
            return 0;
        }
        for (uint8_t i = 1U; i < sh_argc; i++) {
            if (kstrcmp(sh_argv[i], OV7_S("/proc")) == 0 || kstrcmp(sh_argv[i], OV7_S("proc")) == 0) mnt_flags |= MNT_PROC;
            else if (kstrcmp(sh_argv[i], OV7_S("/sys")) == 0 || kstrcmp(sh_argv[i], OV7_S("sysfs")) == 0) mnt_flags |= MNT_SYS;
            else if (kstrcmp(sh_argv[i], OV7_S("/mnt/sd")) == 0 || kstrcmp(sh_argv[i], OV7_S("vfat")) == 0) {
                mnt_flags |= MNT_SD;
            }
        }
        return 0;
    }

    if (kstrcmp(cmd, OV7_S("umount")) == 0) {
        if (sh_argc > 1U) {
            if (kstrcmp(sh_argv[1], OV7_S("/proc")) == 0) mnt_flags &= (uint8_t)~MNT_PROC;
            else if (kstrcmp(sh_argv[1], OV7_S("/sys")) == 0) mnt_flags &= (uint8_t)~MNT_SYS;
            else if (kstrcmp(sh_argv[1], OV7_S("/mnt/sd")) == 0) { mnt_flags &= (uint8_t)~MNT_SD; }
        }
        return 0;
    }

    if (kstrcmp(cmd, OV7_S("uptime")) == 0) {
        update_jiffies();
        kputs(OV7_S(" 00:0"));
        kprint_u32((jiffies / 6000UL) % 10UL);
        kputs(OV7_S(":"));
        uint32_t s = (jiffies / 100UL) % 60UL;
        if (s < 10UL) kputc('0');
        kprint_u32(s);
        kputs(OV7_S(" up "));
        kprint_u32(jiffies / 100UL);
        kprintln(OV7_S(" sec,  1 user,  load average: 0.08, 0.03, 0.01"));
        return 0;
    }

    if (kstrcmp(cmd, OV7_S("id")) == 0) {
        if (current_uid == 0U) kprintln(OV7_S("uid=0(root) gid=0(root) groups=0(root)"));
        else kprintln(OV7_S("uid=65534(nobody) gid=65534(nogroup) groups=65534(nogroup)"));
        return 0;
    }

    if (kstrcmp(cmd, OV7_S("whoami")) == 0) {
        kprintln(current_uid == 0U ? OV7_S("root") : OV7_S("nobody"));
        return 0;
    }

    if (kstrcmp(cmd, OV7_S("su")) == 0) {
        if (sh_argc > 1U && kstrcmp(sh_argv[1], OV7_S("nobody")) == 0) {
            current_uid = 1U;
            current_gid = 1U;
            env_set("USER", "nobody");
        } else {
            current_uid = 0U;
            current_gid = 0U;
            env_set("USER", "root");
        }
        return 0;
    }

    if (kstrcmp(cmd, OV7_S("hostname")) == 0) {
        if (sh_argc == 1U) {
            kprintln(init_uts_ns.nodename);
        } else if (kstrcmp(sh_argv[1], OV7_S("-F")) == 0 && sh_argc > 2U) {
            int16_t n = vfs_read_by_path(kpath_copy(sh_argv[2]), scratch_buf, 17U);
            if (n > 0) {
                scratch_buf[n] = '\0';
                proc_sys_handle(kpath_copy(OV7_S("/sys/kernel/hostname")), true, karg_copy(scratch_buf));
            }
        } else {
            proc_sys_handle(kpath_copy(OV7_S("/sys/kernel/hostname")), true, karg_copy(sh_argv[1]));
        }
        return 0;
    }

    if (kstrcmp(cmd, OV7_S("date")) == 0) {
        update_jiffies();
        kputs(OV7_S("Mon Nov 11 02:"));
        uint32_t m = (15UL + (jiffies / 6000UL)) % 60UL;
        if (m < 10UL) kputc('0');
        kprint_u32(m);
        kputc(':');
        uint32_t s = (jiffies / 100UL) % 60UL;
        if (s < 10UL) kputc('0');
        kprint_u32(s);
        kprintln(OV7_S(" UTC 2026"));
        return 0;
    }

    if (kstrcmp(cmd, OV7_S("env")) == 0 || kstrcmp(cmd, OV7_S("printenv")) == 0) {
        for (uint8_t i = 0U; i < MAX_ENV_VARS; i++) {
            if (env_vars[i][0]) kprintln(env_vars[i]);
        }
        return 0;
    }

    if (kstrcmp(cmd, OV7_S("export")) == 0) {
        if (sh_argc == 1U) {
            for (uint8_t i = 0U; i < MAX_ENV_VARS; i++) {
                if (env_vars[i][0]) { kputs(OV7_S("export ")); kprintln(env_vars[i]); }
            }
            return 0;
        }
        for (uint8_t i = 1U; i < sh_argc; i++) {
            char* eq = nullptr;
            for (char* s = sh_argv[i]; *s; s++) { if (*s == '=') { eq = s; break; } }
            if (eq) { *eq = '\0'; env_set(sh_argv[i], eq + 1); }
        }
        return 0;
    }

    if (kstrcmp(cmd, OV7_S("unset")) == 0) {
        if (sh_argc > 1U) {
            size_t kl = kstrlen(sh_argv[1]);
            for (uint8_t i = 0U; i < MAX_ENV_VARS; i++) {
                if (kstrncmp(env_vars[i], sh_argv[1], kl) == 0 && env_vars[i][kl] == '=') {
                    env_vars[i][0] = '\0';
                }
            }
        }
        return 0;
    }

    if (kstrcmp(cmd, OV7_S("kill")) == 0) {
        if (sh_argc < 2U) return 1;
        const char* pstr = sh_argv[sh_argc - 1U];
        uint8_t pid = (uint8_t)(pstr[0] - '0');
        if (pid == 1U) {
            panic(karg_copy(OV7_S("Attempted to kill init! exitcode=0x00000009")));
            return 1;
        }
        for (uint8_t i = 0U; i < MAX_TASKS; i++) {
            if (tasks[i].pid == pid && !(tasks[i].flags & PF_KTHREAD)) {
                tasks[i].pid = 0U;
                return 0;
            }
        }
        kprintln(OV7_S("kill: operation not permitted (kernel thread)"));
        return 1;
    }

    if (kstrcmp(cmd, OV7_S("clear")) == 0) {
        kputs(OV7_S("\x1b[2J\x1b[H"));
        if (vt100_bar_enabled) {
            kputs(OV7_S("\x1b[2;24r\x1b[2;1H"));
            vt100_draw_bar();
        }
        return 0;
    }

    if (kstrcmp(cmd, OV7_S("stty")) == 0) {
        if (sh_argc > 2U && kstrcmp(sh_argv[1], OV7_S("bar")) == 0) {
            if (kstrcmp(sh_argv[2], OV7_S("off")) == 0) {
                vt100_bar_enabled = false;
                kputs(OV7_S("\x1b[r\x1b[2J\x1b[H"));
            } else {
                vt100_bar_enabled = true;
                kputs(OV7_S("\x1b[2;24r\x1b[24;1H"));
                vt100_draw_bar();
            }
            return 0;
        }
        kprintln(OV7_S("speed 19200 baud; line = 0; rows 24; columns 80; vt100;"));
        return 0;
    }

    if (kstrcmp(cmd, OV7_S("sync")) == 0) {
        return 0;
    }

    if (kstrcmp(cmd, OV7_S("panic")) == 0) {
        const char* pmsg = (sh_argc > 1U) ? sh_argv[1] : OV7_S("sysrq triggered crash");
        panic(karg_copy(pmsg));
        return 1;
    }

    if (kstrcmp(cmd, OV7_S("exit")) == 0 || kstrcmp(cmd, OV7_S("reboot")) == 0 ||
        kstrcmp(cmd, OV7_S("halt")) == 0 || kstrcmp(cmd, OV7_S("poweroff")) == 0) {
        printk(KERN_NOTICE, OV7_S("init: Sending SIGTERM to all processes..."));
        printk(KERN_NOTICE, OV7_S("init: Sending SIGKILL to all processes..."));
        if (mnt_flags & MNT_SD) mnt_flags &= (uint8_t)~MNT_SD;
        printk(KERN_EMERG, OV7_S("reboot: System halted."));
        kernel_running = false;
        return 0;
    }

    return -32768;
}

/* =========================================================================
 * KERNEL ENTRY POINT (main in Resident Core RAM)
 * ========================================================================= */

int main(int argc, char** argv) {
    jiffies          = 0UL;
    log_next_idx     = 0U;
    console_loglevel = 7U;
    current_uid      = 0U;
    current_gid      = 0U;
    cwd_ino          = 0U;
    active_pid       = 1U;
    last_exit_code   = 0;
    kernel_running   = true;
    boot_quiet       = false;
    boot_single      = false;
    boot_panic_test  = false;
    vt100_bar_enabled = true;
    redir_mode       = 0U;
    stdin_from_pipe  = false;
    mnt_flags        = 0U;
    hist_count       = 0U;
    irq_acia_cnt     = 1U;
    irq_via_cnt      = 1U;
    irq_sd_cnt       = 0U;
    prng_state       = 0xACE1U;

    memset(env_vars, 0, sizeof(env_vars));
    env_set("PATH",     "/bin:/sbin");
    env_set("HOME",     "/root");
    env_set("TERM",     "vt100");
    env_set("USER",     "root");
    env_set("SHELL",    "/bin/sh");
    env_set("PWD",      "/");
    env_set("HOSTNAME", "uclinux-65c02");

    const char* dash_c_cmd = nullptr;
    for (int i = 1; i < argc; i++) {
        if (kstrcmp(argv[i], "quiet") == 0) boot_quiet = true;
        else if (kstrcmp(argv[i], "single") == 0) boot_single = true;
        else if (kstrcmp(argv[i], "panic") == 0) boot_panic_test = true;
        else if (kstrcmp(argv[i], "nomodeset") == 0 || kstrcmp(argv[i], "nobar") == 0) vt100_bar_enabled = false;
        else if (kstrcmp(argv[i], "-c") == 0 && (i + 1) < argc) {
            dash_c_cmd = argv[++i];
            vt100_bar_enabled = false;
        }
    }

    start_kernel();
    if (kernel_running) populate_rootfs();
    if (kernel_running) run_init_process();

    if (kernel_running) {
        if (dash_c_cmd) {
            kstrcpy_lim(cmd_line_buf, dash_c_cmd, sizeof(cmd_line_buf));
            shell_exec_line(cmd_line_buf);
        } else {
            shell_run_loop();
        }
    }

    if (vt100_bar_enabled) {
        vt100_bar_enabled = false;
        kputs("\x1b[r\x1b[0m\n");
    }
    return 0;
}
