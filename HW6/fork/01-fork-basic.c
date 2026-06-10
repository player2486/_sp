/*
 * HW6 - fork/01-fork-basic.c
 *
 * fork 基本概念：建立子行程，觀察父子行程同時執行。
 *
 * fork() 回傳值：
 *   - 負數：失敗
 *   - 0：在子行程中
 *   - 正數（子 PID）：在父行程中
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main()
{
    printf("【父行程】PID=%d，準備 fork...\n\n", getpid());

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork 失敗");
        return 1;
    }

    if (pid == 0) {
        /* 子行程 */
        printf("【子行程】PID=%d，父行程 PPID=%d\n", getpid(), getppid());
        printf("【子行程】fork() 回傳值 = %d (0 表示我是子行程)\n", pid);
        printf("【子行程】我在執行...\n");
    } else {
        /* 父行程 */
        printf("【父行程】PID=%d\n", getpid());
        printf("【父行程】fork() 回傳值 = %d (這是子行程的 PID)\n", pid);
        printf("【父行程】子行程 PID=%d 正在執行...\n", pid);
    }

    /* 父子行程都會執行這行 */
    printf("【PID=%d】這行程式碼父子都會執行！\n\n", getpid());

    return 0;
}
