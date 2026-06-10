#include "display.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>

static struct termios old_term;

void display_init(void)
{
    /* 儲存原始終端機設定 */
    tcgetattr(STDIN_FILENO, &old_term);

    /* 設定終端機為 raw mode（即時讀取按鍵）*/
    struct termios raw = old_term;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    printf(HIDE_CURSOR CLEAR HOME);
    fflush(stdout);
}

void display_restore(void)
{
    printf(SHOW_CURSOR);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_term);
}

void display_goto(int row, int col)
{
    printf(ESC "%d;%dH", row, col);
}

/* 獲取終端機寬度 */
int get_term_cols(void)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0)
        return ws.ws_col;
    return 80;
}

/* 獲取終端機高度 */
int get_term_rows(void)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0)
        return ws.ws_row;
    return 24;
}

void display_header(const MemInfo *mi, const CpuTime *ct,
                    int process_count, int sort_mode)
{
    char time_str[64];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);

    /* 使用 /proc/uptime 之類的方法也可以，但簡化 */
    long mem_used_kb = mi->mem_total_kb - mi->mem_free_kb;

    display_goto(1, 1);
    printf(BOLD "HW7 Process Monitor" RESET "     %s     "
           "更新中... (按 q 離開)\n", time_str);

    printf(GRAY "행정: %-5d  "
           "CPU: user=%-4ld sys=%-4ld idle=%-4ld  "
           "Mem: %ld MB / %ld MB (%.0f%%)\n" RESET,
           process_count,
           ct->user / 100, ct->system / 100, ct->idle / 100,
           mem_used_kb / 1024, mi->mem_total_kb / 1024,
           (float)mem_used_kb / mi->mem_total_kb * 100);

    printf("\n");

    /* 表頭 */
    const char *sort_ind_pid  = (sort_mode == 0) ? BOLD REV "  " RESET : "  ";
    const char *sort_ind_cpu  = (sort_mode == 1) ? BOLD REV "  " RESET : "  ";
    const char *sort_ind_mem  = (sort_mode == 2) ? BOLD REV "  " RESET : "  ";
    const char *sort_ind_name = (sort_mode == 3) ? BOLD REV "  " RESET : "  ";

    printf("%s PID    %s %%CPU  %s %%MEM  %s NAME                  STATE     RSS\n",
           sort_ind_pid, sort_ind_cpu, sort_ind_mem, sort_ind_name);
    printf(GRAY "%-8s %-7s %-7s %-22s %-8s %s\n" RESET,
           "", "", "", "", "", "");
}

void display_processes(const ProcessInfo *procs, int count,
                       int max_rows, int sort_mode)
{
    (void)sort_mode;
    int cols = get_term_cols();

    for (int i = 0; i < max_rows && i < count; i++) {
        int row = i + 5;
        const ProcessInfo *p = &procs[i];

        /* 根據狀態著色 */
        const char *color = WHITE;
        if (p->state == 'R') color = GREEN;
        if (p->state == 'Z') color = RED;
        if (p->state == 'S') color = CYAN;

        /* 狀態字串 */
        const char *state_str = "?";
        switch (p->state) {
            case 'R': state_str = "RUNNING"; break;
            case 'S': state_str = "SLEEP";   break;
            case 'D': state_str = "DISK";    break;
            case 'Z': state_str = "ZOMBIE";  break;
            case 'T': state_str = "STOP";    break;
        }

        /* RSS 單位換算 */
        long rss_kb = p->rss_pages * sysconf(_SC_PAGESIZE) / 1024;
        char rss_str[32];
        if (rss_kb > 1024)
            snprintf(rss_str, sizeof(rss_str), "%ldM", rss_kb / 1024);
        else
            snprintf(rss_str, sizeof(rss_str), "%ldK", rss_kb);

        char line[512];
        snprintf(line, sizeof(line),
            "%-8d %-7.1f %-7.1f %-22s %-8s %s",
            p->pid, p->cpu_percent, p->mem_percent,
            p->name, state_str, rss_str);

        /* 截斷行（避免 wrap）*/
        line[cols - 1] = '\0';

        display_goto(row, 1);
        printf("%s%s" RESET, color, line);
    }

    /* 清除空白行 */
    for (int i = count; i < max_rows; i++) {
        display_goto(i + 5, 1);
        printf("%-*s", get_term_cols(), "");
    }
}

void display_footer(int sort_mode)
{
    int rows = get_term_rows();
    int cols = get_term_cols();

    const char *sort_names[] = {"PID", "%%CPU", "%%MEM", "NAME"};
    char footer[256];
    snprintf(footer, sizeof(footer),
        "排序: %s  |  1:CPU  2:MEM  3:PID  4:NAME  q:離開",
        sort_names[sort_mode]);

    display_goto(rows, 1);
    printf(REV "%-*s" RESET, cols, footer);
}
