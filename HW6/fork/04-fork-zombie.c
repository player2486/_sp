/*
 * HW6 - fork/04-fork-zombie.c
 *
 * 殭屍行程（Zombie）：
 * 子行程先結束，父行程未呼叫 wait() 來回收。
 * 子行程的 PCB 殘留在系統中，變成殭屍。
 *
 * 觀察方式：執行後在另一個終端輸入 "ps aux | grep Z"
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main()
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork 失敗");
        return 1;
    }

    if (pid == 0) {
        /* 子行程：立即結束，變成殭屍 */
        printf("【子行程】PID=%d，即將結束...\n", getpid());
        printf("【子行程】結束後父行程未 wait，我將變成殭屍！\n");
        exit(0);
    }

    /* 父行程 */
    printf("【父行程】子行程 PID=%d\n", pid);
    printf("【父行程】睡眠 15 秒，期間子行程是殭屍狀態\n");
    printf("【父行程】請在另一個終端機執行: ps aux | grep Z\n\n");

    /*
     * 在這 15 秒內，子行程已結束但父行程沒呼叫 wait()，
     * 子行程處於殭屍狀態（Z+）。
     *
     * 可以用 "ps aux" 看到狀態為 Z 的行程。
     */
    for (int i = 15; i > 0; i--) {
        printf("  剩餘 %2d 秒... (子行程是殭屍)\n", i);
        sleep(1);
    }

    printf("\n【父行程】呼叫 wait() 清除殭屍\n");
    wait(NULL);

    printf("【父行程】殭屍已清除！\n");
    printf("【父行程】結束\n");

    return 0;
}
