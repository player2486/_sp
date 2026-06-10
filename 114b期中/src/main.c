/*
 * HW7 - Process Monitor (mini top)
 *
 * 讀取 /proc 檔案系統，即時顯示行程資訊。
 *
 * 編譯：gcc -o hw7-top src/main.c src/procfs.c src/display.c
 * 執行：./hw7-top
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>

#include "procfs.h"
#include "display.h"

volatile sig_atomic_t running = 1;

void handle_sigint(int sig)
{
    (void)sig;
    running = 0;
}

int main(void)
{
    /* 信號處理 */
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    /* 初始化終端機 */
    display_init();
    atexit(display_restore);

    ProcessInfo prev_procs[MAX_PROCESSES];
    ProcessInfo curr_procs[MAX_PROCESSES];
    CpuTime prev_cpu, curr_cpu;

    int sort_mode = 1;  /* 0=PID, 1=CPU, 2=MEM, 3=NAME */
    int process_count = 0;

    /* 第一次取樣 */
    prev_cpu = read_cpu_time();
    process_count = read_processes(prev_procs, MAX_PROCESSES);
    sort_by_cpu(prev_procs, process_count);

    int frame = 0;

    while (running) {
        /* 第二次取樣 */
        curr_cpu = read_cpu_time();
        process_count = read_processes(curr_procs, MAX_PROCESSES);

        /* 計算 CPU 使用率 */
        calc_cpu_percent(prev_procs, process_count,
                        &prev_cpu, &curr_cpu,
                        prev_procs, curr_procs);

        /* 排序 */
        switch (sort_mode) {
            case 0:  sort_by_pid(curr_procs, process_count);  break;
            case 1:  sort_by_cpu(curr_procs, process_count);  break;
            case 2:  sort_by_mem(curr_procs, process_count);  break;
            case 3:  sort_by_name(curr_procs, process_count); break;
        }

        /* 取得終端機大小 */
        int rows = get_term_rows();

        /* 顯示 */
        MemInfo mi = read_mem_info();
        display_header(&mi, &curr_cpu, process_count, sort_mode);
        display_processes(curr_procs, process_count, rows - 6, sort_mode);
        display_footer(sort_mode);

        /* 處理按鍵 */
        char ch;
        if (read(STDIN_FILENO, &ch, 1) > 0) {
            switch (ch) {
                case '1': sort_mode = 0; break;  /* PID */
                case '2': sort_mode = 1; break;  /* CPU (not 1 because '1' is PID) */
                case '3': sort_mode = 2; break;  /* MEM */
                case '4': sort_mode = 3; break;  /* NAME */
                case 'q': case 'Q': running = 0; break;
            }
        }

        /* 交換 prev/curr（只保留 CPU 時間欄位給下次計算）*/
        prev_cpu = curr_cpu;
        memcpy(prev_procs, curr_procs, sizeof(ProcessInfo) * process_count);

        frame++;
        usleep(1000000);  /* 1 秒 */
    }

    /* 結束 */
    display_restore();
    printf("\n結束。共更新 %d 次。\n", frame);

    return 0;
}
