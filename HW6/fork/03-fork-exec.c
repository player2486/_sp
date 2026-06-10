/*
 * HW6 - fork/03-fork-exec.c
 *
 * fork + execvp：建立新程式。
 * 子行程用 execvp 執行 "/bin/ls -l"，
 * 父行程等待子行程結束。
 *
 * execvp 成功後不返回，當前行程的程式碼被完全取代。
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    printf("【父行程】PID=%d，準備 fork + exec...\n\n", getpid());

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork 失敗");
        return 1;
    }

    if (pid == 0) {
        /* 子行程 */
        printf("【子行程】PID=%d，準備執行 ls...\n", getpid());

        /* execvp 取代當前行程 */
        char *args[] = {"ls", "-l", NULL};
        execvp("ls", args);

        /* 只有在 execvp 失敗時才會執行以下程式碼 */
        perror("execvp 失敗");
        exit(1);
    }

    /* 父行程 */
    printf("【父行程】等待子行程 (PID=%d) 結束...\n", pid);

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        printf("\n【父行程】子行程正常結束，exit code = %d\n",
               WEXITSTATUS(status));
    }

    printf("【父行程】繼續執行其他工作...\n");
    printf("【父行程】結束\n");

    return 0;
}
