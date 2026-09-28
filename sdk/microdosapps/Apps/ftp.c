/*
 * ftp.c - microDOS FTP Client and Server
 */
#include <stdint.h>
#include <string.h>

#include "Libs/app-bios.h"

uint8_t ftp_abort = 0;

static char get_net_char(void) {
    while (!net_has_data()) {
        char k = bios_getchar_nb();
        if (k == 'q' || k == 'Q') {
            ftp_abort = 1;
            return '\n';
        }
    }
    return (char)net_getc();
}

uint8_t link_state[5] = {0};

static void resolve_path(const char* cwd, const char* path, char* out) {
    if (strcmp(path, "..") == 0) {
        int i;
        strcpy(out, cwd);
        i = strlen(out) - 1;
        while (i > 0 && out[i] != '/') i--;
        if (i > 0) out[i] = '\0';
        else { out[0] = '/'; out[1] = '\0'; }
        return;
    }
    if (path[0] == '/') {
        strcpy(out, path);
    } else {
        strcpy(out, cwd);
        if (strcmp(cwd, "/") != 0) {
            strcat(out, "/");
        }
        strcat(out, path);
    }
}

#define TYPE_NONE 0
#define TYPE_DATA 1
#define TYPE_CONNECT 2
#define TYPE_CLOSED 3
#define TYPE_OK 4
#define TYPE_ERROR 5
#define TYPE_PROMPT 6

typedef struct {
    uint8_t type;
    uint8_t link;
    char c;
} NetEvent;

static void next_event(NetEvent* ev) {
    static uint8_t in_ipd = 0;
    static uint8_t cur_ipd_link = 0;
    static uint16_t cur_ipd_len = 0;
    char c;
    char last_c = 0;

    ev->type = 0;
    ev->link = 0;
    ev->c = 0;

    ev->c = 0;

    while (1) {
        if (ftp_abort) return;
        if (in_ipd && cur_ipd_len > 0) {
            ev->type = TYPE_DATA;
            ev->link = cur_ipd_link;
            ev->c = get_net_char();
            cur_ipd_len--;
            if (cur_ipd_len == 0) in_ipd = 0;
            return;
        }

        c = get_net_char();

        if (c == '+') {
            if (get_net_char() == 'I' && get_net_char() == 'P' && get_net_char() == 'D' && get_net_char() == ',') {
                cur_ipd_link = get_net_char() - '0';
                get_net_char();
                cur_ipd_len = 0;
                while (1) {
                    char d = get_net_char();
                    if (ftp_abort) return;
                    if (d == ':') break;
                    cur_ipd_len = cur_ipd_len * 10 + (d - '0');
                }
                in_ipd = 1;
            } else {
                last_c = 0;
            }
        } else if (c >= '0' && c <= '4') {
            uint8_t link = c - '0';
            char d = get_net_char();
            if (d == ',') {
                char e = get_net_char();
                if (e == 'C') {
                    char f = get_net_char();
                    if (f == 'O') {
                        int i;
                        for (i = 0; i < 5; i++) get_net_char();
                        ev->type = TYPE_CONNECT;
                        ev->link = link;
                        link_state[link] = 1;
                        return;
                    } else if (f == 'L') {
                        int i;
                        for (i = 0; i < 4; i++) get_net_char();
                        ev->type = TYPE_CLOSED;
                        ev->link = link;
                        link_state[link] = 0;
                        return;
                    }
                }
            }
            last_c = 0;
        } else if (c == 'O' && (last_c == '\n' || last_c == ' ')) {
            if (get_net_char() == 'K') {
                while (get_net_char() != '\n');
                ev->type = TYPE_OK;
                return;
            }
            last_c = 'K';
        } else if ((c == 'E' && last_c == '\n') || (c == 'F' && last_c == ' ')) {
            if (get_net_char() == (c == 'E' ? 'R' : 'A')) {
                while (get_net_char() != '\n');
                ev->type = TYPE_ERROR;
                return;
            }
            last_c = (c == 'E' ? 'R' : 'A');
        } else if (c == '>') {
            ev->type = TYPE_PROMPT;
            return;
        } else {
            last_c = c;
        }
    }
}

static SD_FILE early_fp;
static uint8_t early_open = 0;
static uint32_t early_total = 0;
static uint8_t early_closed = 0;
static char early_fbuf[1024];
static uint16_t early_fbuf_len = 0;

static void push_early_data(char c) {
    if (!early_open) {
        if (sd_open(&early_fp, "/TEMP.DAT", SD_WRITE | SD_CREATE_ALWAYS)) {
            early_open = 1;
            early_fbuf_len = 0;
            early_total = 0;
        } else {
            return;
        }
    }
    early_fbuf[early_fbuf_len++] = c;
    early_total++;
    if (early_fbuf_len == sizeof(early_fbuf)) {
        sd_write(&early_fp, early_fbuf, early_fbuf_len);
        early_fbuf_len = 0;
    }
}

static void read_line(uint8_t link, char* buf, int max_len) {
    int pos = 0;
    while (pos < max_len - 1) {
        NetEvent ev;
        if (ftp_abort) { buf[0] = '\0'; return; }
        next_event(&ev);
        if (ev.type == TYPE_DATA && ev.link == link) {
            if (ev.c == '\n') break;
            if (ev.c != '\r') buf[pos++] = ev.c;
        } else if (ev.type == TYPE_DATA && ev.link == 1) {
            push_early_data(ev.c);
        } else if (ev.type == TYPE_CLOSED && ev.link == 1) {
            early_closed = 1;
        } else if (ev.type == TYPE_CLOSED && ev.link == link) {
            break;
        }
    }
    buf[pos] = '\0';
}

static int read_ftp_code(uint8_t link, char* buf, int max_len) {
    while (1) {
        if (ftp_abort) return -1;
        read_line(link, buf, max_len);
        if (ftp_abort) return -1;
        if (strlen(buf) >= 3 && buf[3] == ' ') {
            return (buf[0] - '0') * 100 + (buf[1] - '0') * 10 + (buf[2] - '0');
        }
    }
}

static void send_link(uint8_t link, const char* str) {
    uint16_t len = strlen(str);
    char num[2];
    net_send("AT+CIPSEND=");
    num[0] = '0' + link;
    num[1] = '\0';
    net_send(num);
    net_send(",");
    net_send_num(len);
    net_send("\r\n");

    while (1) {
        NetEvent ev;
        if (ftp_abort) return;
        next_event(&ev);
        if (ev.type == TYPE_PROMPT) break;
        if (ev.type == TYPE_ERROR) return;
        if (ev.type == TYPE_DATA && ev.link == 1) {
            push_early_data(ev.c);
        } else if (ev.type == TYPE_CLOSED && ev.link == 1) {
            early_closed = 1;
        }
    }
    net_send(str);
}

static void send_link_bin(uint8_t link, const void* data, uint16_t len) {
    char num[2];
    const uint8_t* p;
    uint16_t i;

    net_send("AT+CIPSEND=");
    num[0] = '0' + link;
    num[1] = '\0';
    net_send(num);
    net_send(",");
    net_send_num(len);
    net_send("\r\n");

    while (1) {
        NetEvent ev;
        if (ftp_abort) return;
        next_event(&ev);
        if (ev.type == TYPE_PROMPT) break;
        if (ev.type == TYPE_ERROR) return;
        if (ev.type == TYPE_DATA && ev.link == 1) {
            push_early_data(ev.c);
        } else if (ev.type == TYPE_CLOSED && ev.link == 1) {
            early_closed = 1;
        }
    }

    p = (const uint8_t*)data;
    for (i = 0; i < len; i++) {
        net_putc(p[i]);
    }
}

static void append_num(int val, char* str) {
    int len = strlen(str);
    if (val >= 100) {
        str[len++] = '0' + (val / 100);
        str[len++] = '0' + ((val / 10) % 10);
        str[len++] = '0' + (val % 10);
    } else if (val >= 10) {
        str[len++] = '0' + (val / 10);
        str[len++] = '0' + (val % 10);
    } else {
        str[len++] = '0' + val;
    }
    str[len] = '\0';
}

static void append_u32(uint32_t val, char* str) {
    int len = strlen(str);
    char buf[12];
    int i = 0;
    if (val == 0) {
        str[len++] = '0';
        str[len] = '\0';
        return;
    }
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    while (i > 0) {
        str[len++] = buf[--i];
    }
    str[len] = '\0';
}

static void run_client(const char* server, const char* user, const char* pass, const char* remote, const char* local) {
    char buf[128];
    char* p;
    int h1, h2, h3, h4, p1, p2, port;
    char ip[32];
    SD_FILE fp;
    uint32_t wr_cnt = 0;
    NetEvent ev;

    net_send("AT+CIPMUX=1\r\n");
    next_event(&ev);
    while (ev.type != TYPE_OK) {
        if (ftp_abort) return;
        next_event(&ev);
    }

    net_send("AT+CIPSTART=0,\"TCP\",\"");
    net_send(server);
    net_send("\",21\r\n");

    while (1) {
        if (ftp_abort) return;
        next_event(&ev);
        if (ev.type == TYPE_CONNECT && ev.link == 0) break;
        if (ev.type == TYPE_ERROR) {
            print_str("Error: ");
            println("Connection failed");
            return;
        }
    }

    read_ftp_code(0, buf, sizeof(buf));

    strcpy(buf, "USER ");
    strcat(buf, user);
    strcat(buf, "\r\n");
    send_link(0, buf);
    if (read_ftp_code(0, buf, sizeof(buf)) != 331) return;

    strcpy(buf, "PASS ");
    strcat(buf, pass);
    strcat(buf, "\r\n");
    send_link(0, buf);
    if (read_ftp_code(0, buf, sizeof(buf)) != 230) return;

    send_link(0, "TYPE I\r\n");
    read_ftp_code(0, buf, sizeof(buf));

    send_link(0, "PASV\r\n");
    if (read_ftp_code(0, buf, sizeof(buf)) != 227) return;

    p = buf;
    while (*p && *p != '(') p++;
    if (!*p) return;
    p++;
    h1 = 0;
    while (*p != ',') {
        h1 = h1 * 10 + (*p - '0');
        p++;
    }
    p++;
    h2 = 0;
    while (*p != ',') {
        h2 = h2 * 10 + (*p - '0');
        p++;
    }
    p++;
    h3 = 0;
    while (*p != ',') {
        h3 = h3 * 10 + (*p - '0');
        p++;
    }
    p++;
    h4 = 0;
    while (*p != ',') {
        h4 = h4 * 10 + (*p - '0');
        p++;
    }
    p++;
    p1 = 0;
    while (*p != ',') {
        p1 = p1 * 10 + (*p - '0');
        p++;
    }
    p++;
    p2 = 0;
    while (*p != ')') {
        p2 = p2 * 10 + (*p - '0');
        p++;
    }

    ip[0] = '\0';
    append_num(h1, ip);
    strcat(ip, ".");
    append_num(h2, ip);
    strcat(ip, ".");
    append_num(h3, ip);
    strcat(ip, ".");
    append_num(h4, ip);

    port = p1 * 256 + p2;

    net_send("AT+CIPSTART=1,\"TCP\",\"");
    net_send(ip);
    net_send("\",");
    net_send_num(port);
    net_send("\r\n");
    while (1) {
        if (ftp_abort) return;
        next_event(&ev);
        if (ev.type == TYPE_CONNECT && ev.link == 1) break;
        if (ev.type == TYPE_ERROR) {
            print_str("Error: ");
            println("Data connection failed");
            return;
        }
    }

    strcpy(buf, "RETR ");
    strcat(buf, remote);
    strcat(buf, "\r\n");
    send_link(0, buf);
    read_ftp_code(0, buf, sizeof(buf));

    if (!sd_open(&fp, local, SD_WRITE | SD_CREATE_ALWAYS)) {
        println("SD error");
        return;
    }

    while (1) {
        if (ftp_abort) break;
        next_event(&ev);
        if (ev.type == TYPE_DATA && ev.link == 1) {
            sd_write(&fp, &ev.c, 1);
            wr_cnt++;
        } else if (ev.type == TYPE_CLOSED && ev.link == 1) {
            break;
        }
    }
    sd_close(&fp);

    read_ftp_code(0, buf, sizeof(buf));
    send_link(0, "QUIT\r\n");

    net_send("AT+CIPMUX=0\r\n");
    print_num((uint16_t)wr_cnt);
    println(" bytes transferred");
}

static void run_server(const char* user, const char* pass, const char* port_str) {
    static char buf[128];
    NetEvent ev;
    static char current_dir[128];
    uint8_t quotes = 0;
    int port = 2121;

    strcpy(current_dir, "/");
    if (port_str) {
        port = 0;
        while (*port_str >= '0' && *port_str <= '9') {
            port = port * 10 + (*port_str - '0');
            port_str++;
        }
    }
    println("Press 'q' to exit");
    net_send("AT+CIFSR\r\n");
    print_str("Listening on IP: ");
    while (1) {
        char c = get_net_char();
        if (ftp_abort) return;
        if (c == 'O') {
            if (get_net_char() == 'K') {
                if (quotes == 0) print_str("Unknown");
                break;
            }
        } else if (c == 'E') {
            if (get_net_char() == 'R') {
                if (quotes == 0) print_str("Disconnected");
                break;
            }
        } else if (c == '"') {
            quotes++;
            if (quotes == 1) continue;
            if (quotes == 2) break;
        } else if (quotes == 1) {
            bios_putchar(c);
        }
    }
    print_str(" Port: ");
    print_num((uint16_t)port);
    println("");

    if (quotes == 2) {
        while (1) {
            char c = get_net_char();
            if (ftp_abort) return;
            if (c == 'O') {
                if (get_net_char() == 'K') {
                    while (get_net_char() != '\n');
                    break;
                }
            } else if (c == 'E') {
                if (get_net_char() == 'R') {
                    while (get_net_char() != '\n');
                    break;
                }
            }
        }
    }

    net_send("AT+CIPMUX=1\r\n");
    next_event(&ev);
    while (ev.type != TYPE_OK) {
        if (ftp_abort) return;
        if (ev.type == TYPE_ERROR) {
            println("Failed to set MUX");
            return;
        }
        next_event(&ev);
    }

    net_send("AT+CIPSERVER=1,");
    net_send_num((uint16_t)port);
    net_send("\r\n");
    next_event(&ev);
    while (ev.type != TYPE_OK) {
        if (ftp_abort) return;
        if (ev.type == TYPE_ERROR) {
            print_str("Failed to start server on port ");
            print_num((uint16_t)port);
            println("");
            return;
        }
        next_event(&ev);
    }

    while (1) {
        if (ftp_abort) break;
        next_event(&ev);
        if (ev.type == TYPE_CONNECT && ev.link == 0) {
            println("Client connected");
            send_link(0, "220 microDOS FTP Server Ready\r\n");

            while (link_state[0]) {
                if (ftp_abort) break;
                read_line(0, buf, sizeof(buf));
                if (ftp_abort) break;
                if (buf[0] == '\0') continue;

                print_str(">> ");
                println(buf);

                if (strncmp(buf, "USER ", 5) == 0) {
                    char* u = buf + 5;
                    print_str("[LOGIN] User attempt: "); println(u);
                    if (strcmp(u, user) == 0)
                        send_link(0, "331 \r\n");
                    else
                        send_link(0, "530 Invalid user\r\n");
                } else if (strncmp(buf, "PASS ", 5) == 0) {
                    char* p_pass = buf + 5;
                    print_str("[LOGIN] Password attempt: "); println(p_pass);
                    if (strcmp(p_pass, pass) == 0)
                        send_link(0, "230 \r\n");
                    else
                        send_link(0, "530 Invalid password\r\n");
                } else if (strncmp(buf, "SYST", 4) == 0) {
                    send_link(0, "215 UNIX Type: L8\r\n");
                } else if (strncmp(buf, "FEAT", 4) == 0) {
                    send_link(0, "211-Features:\r\n211 End\r\n");
                } else if (strncmp(buf, "PWD", 3) == 0) {
                    char resp[140] = "257 \"";
                    strcat(resp, current_dir);
                    strcat(resp, "\" is current directory\r\n");
                    send_link(0, resp);
                } else if (strncmp(buf, "CWD", 3) == 0) {
                    char* target = buf + 4;
                    if (*target == ' ') target++;
                    print_str("[DIR] Change directory request to: "); println(target);
                    resolve_path(current_dir, target, current_dir);
                    send_link(0, "250 CWD command successful\r\n");
                } else if (strncmp(buf, "CDUP", 4) == 0) {
                    print_str("[DIR] CDUP\r\n");
                    resolve_path(current_dir, "..", current_dir);
                    send_link(0, "250 CDUP command successful\r\n");
                } else if (strncmp(buf, "LIST", 4) == 0) {
                    static SD_DIR dp;
                    static SD_INFO fno;
                    static char target_dir[128];
                    char* target = buf + 4;
                    if (*target == ' ') target++;
                    if (*target == '-' && *(target+1) == 'a') {
                        target += 2;
                        if (*target == ' ') target++;
                    }
                    if (*target == '\0') {
                        strcpy(target_dir, current_dir);
                    } else {
                        resolve_path(current_dir, target, target_dir);
                    }
                    print_str("[LIST] Directory list request: "); println(target_dir);
                    send_link(0, "150 Here comes the directory listing.\r\n");
                    if (sd_opendir(&dp, target_dir)) {
                        while (sd_readdir(&dp, &fno) && fno.fname[0] != 0) {
                            static char line[128];
                            int is_dir;
                            is_dir = fno.fattrib & AM_DIR;
                            line[0] = '\0';
                            if (is_dir) {
                                strcat(line, "drwxr-xr-x 1 root root ");
                            } else {
                                strcat(line, "-rw-r--r-- 1 root root ");
                            }
                            append_u32(fno.fsize, line);
                            strcat(line, " Jan 01 00:00 ");
                            strcat(line, fno.fname);
                            strcat(line, "\r\n");
                            send_link_bin(1, line, strlen(line));
                        }
                        sd_closedir(&dp);
                    }
                    net_send("AT+CIPCLOSE=1\r\n");
                    send_link(0, "226 Directory send OK.\r\n");
                } else if (strncmp(buf, "TYPE I", 6) == 0) {
                    send_link(0, "200 Type set to I\r\n");
                } else if (strncmp(buf, "PORT", 4) == 0) {
                    char* p = buf + 5;
                    int h1, h2, h3, h4, p1, p2, port;
                    char ip[32];
                    uint8_t ok = 0;
                    NetEvent ev2;

                    if (early_open) {
                        sd_close(&early_fp);
                    }
                    early_closed = 0;
                    early_total = 0;
                    early_open = 0;

                    h1 = 0;
                    while (*p != ',') {
                        h1 = h1 * 10 + (*p - '0');
                        p++;
                    }
                    p++;
                    h2 = 0;
                    while (*p != ',') {
                        h2 = h2 * 10 + (*p - '0');
                        p++;
                    }
                    p++;
                    h3 = 0;
                    while (*p != ',') {
                        h3 = h3 * 10 + (*p - '0');
                        p++;
                    }
                    p++;
                    h4 = 0;
                    while (*p != ',') {
                        h4 = h4 * 10 + (*p - '0');
                        p++;
                    }
                    p++;
                    p1 = 0;
                    while (*p != ',') {
                        p1 = p1 * 10 + (*p - '0');
                        p++;
                    }
                    p++;
                    p2 = 0;
                    while (*p >= '0' && *p <= '9') {
                        p2 = p2 * 10 + (*p - '0');
                        p++;
                    }

                    ip[0] = '\0';
                    append_num(h1, ip);
                    strcat(ip, ".");
                    append_num(h2, ip);
                    strcat(ip, ".");
                    append_num(h3, ip);
                    strcat(ip, ".");
                    append_num(h4, ip);
                    port = p1 * 256 + p2;

                    net_send("AT+CIPSTART=1,\"TCP\",\"");
                    net_send(ip);
                    net_send("\",");
                    net_send_num(port);
                    net_send("\r\n");

                    while (1) {
                        if (ftp_abort) break;
                        next_event(&ev2);
                        if (ev2.type == TYPE_CONNECT && ev2.link == 1) {
                            ok = 1;
                            break;
                        }
                        if (ev2.type == TYPE_ERROR) {
                            break;
                        }
                    }
                    if (ok)
                        send_link(0, "200 PORT command successful\r\n");
                    else
                        send_link(0, "500 PORT failed\r\n");
                } else if (strncmp(buf, "RETR", 4) == 0) {
                    char* file = buf + 5;
                    static char abs_path[128];
                    static SD_FILE fp;
                    resolve_path(current_dir, file, abs_path);
                    print_str("[TRANSFER] Downloading: "); println(abs_path);
                    send_link(0, "150 Opening data connection\r\n");
                    if (sd_open(&fp, abs_path, SD_READ)) {
                        static char fbuf[2048];
                        int16_t r;
                        while ((r = sd_read(&fp, fbuf, sizeof(fbuf))) > 0) {
                            send_link_bin(1, fbuf, r);
                        }
                        sd_close(&fp);
                    }
                    net_send("AT+CIPCLOSE=1\r\n");
                    send_link(0, "226 Transfer complete\r\n");
                } else if (strncmp(buf, "STOR", 4) == 0) {
                    char* file = buf + 5;
                    static char abs_path[128];
                    static SD_FILE fp;
                    resolve_path(current_dir, file, abs_path);
                    print_str("[TRANSFER] Uploading: "); println(abs_path);
                    send_link(0, "150 Ready to receive\r\n");

                    if (!early_closed) {
                        while (1) {
                            NetEvent ev2;
                            if (ftp_abort) break;
                            next_event(&ev2);
                            if (ev2.type == TYPE_DATA && ev2.link == 1) {
                                push_early_data(ev2.c);
                            } else if (ev2.type == TYPE_CLOSED && ev2.link == 1) {
                                break;
                            } else if (ev2.type == TYPE_ERROR) {
                                break;
                            }
                        }
                    }

                    if (early_open) {
                        if (early_fbuf_len > 0) {
                            sd_write(&early_fp, early_fbuf, early_fbuf_len);
                            early_fbuf_len = 0;
                        }
                        sd_close(&early_fp);
                        early_open = 0;
                    }

                    send_link(0, "226 Transfer complete\r\n");

                    if (early_total > 0) {
                        if (sd_open(&fp, abs_path, SD_WRITE | SD_CREATE_ALWAYS)) {
                            static SD_FILE tfp;
                            if (sd_open(&tfp, "/TEMP.DAT", SD_READ)) {
                                static char fbuf[2048];
                                int16_t r;
                                while ((r = sd_read(&tfp, fbuf, sizeof(fbuf))) > 0) {
                                    sd_write(&fp, fbuf, r);
                                }
                                sd_close(&tfp);
                            }
                            sd_close(&fp);
                        }
                        sd_remove("/TEMP.DAT");
                    } else {
                        if (sd_open(&fp, abs_path, SD_WRITE | SD_CREATE_ALWAYS)) {
                            sd_close(&fp);
                        }
                    }

                    early_total = 0;
                    early_closed = 0;
                } else if (strncmp(buf, "QUIT", 4) == 0) {
                    println("[COMM] Client disconnected");
                    send_link(0, "221 Goodbye\r\n");
                    net_send("AT+CIPCLOSE=0\r\n");
                    break;
                } else {
                    send_link(0, "502 Command not implemented\r\n");
                }
            }
            break;
        }
    }
    net_send("AT+CIPSERVER=0\r\n");
    net_send("AT+CIPMUX=0\r\n");
    println("\r\nServer stopped");
}

int main(void) {
    char** args = _args_ptr;
    if (arg_count == 3) {
        run_server(args[1], args[2], NULL);
    } else if (arg_count == 4) {
        run_server(args[1], args[2], args[3]);
    } else if (arg_count >= 6) {
        run_client(args[1], args[2], args[3], args[4], args[5]);
    } else {
        println("Usage:");
        println("  ftp <user> <password> [port]");
        println("  ftp <server> <user> <password> <remote> <local>");
    }
    return 0;
}
