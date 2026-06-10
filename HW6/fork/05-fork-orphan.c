/*
 * HW6 - fork/05-fork-orphan.c
 *
 * 孤兒行程（Orphan）：
 * 父行程先結束，子行程被 init (PID=1) 收養。
 * 子行程的 PPID 會變成 1。
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork 失敗");
        return 1;
    }

    if (pid == 0) {
        /* 子行程 */
        printf("【子行程】剛開始 PPID=%d (父行程)\n", getppid());

        /*
         * 睡眠 5 秒讓父行程先結束。
         * 醒來後父行程已結束，我的 PPID 會變成 1（init）。
         */
        sleep(5);

        printf("【子行程】5 秒後 PPID=%d (1 = init 收養了我)\n", getppid());
        printf("【子行程】我變成了孤兒，但 init 會照顧我\n");
        printf("【子行程】結束\n");
        exit(0);
    }

    /* 父行程：先結束 */
    printf("【父行程】PID=%d，子行程 PID=%d\n", getpid(), pid);
    printf("【父行程】我先結束，子行程將變成孤兒\n");

    /*
     * 父行程結束時子行程會變成孤兒。
     * init (PID=1) 會收養子行程並自動清理。
     */
    exit(0);
}
