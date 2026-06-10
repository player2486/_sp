/*
 * HW6 - file/04-dup2-pipe.c
 *
 * pipe() + dup2() + fork() + execvp()
 *
 * 模擬 shell pipe:
 *   ls -l | wc -l
 *
 * 流程：
 *   父行程 (ls)  →  寫入 pipe  →  dup2(pipe[1], 1)
 *   子行程 (wc)  ←  讀取 pipe  →  dup2(pipe[0], 0)
 *
 *   等價於 shell: (ls -l) | (wc -l)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    int pipefd[2];
    pid_t pid1, pid2;

    /* 建立 pipe */
    if (pipe(pipefd) < 0) {
        perror("pipe");
        return 1;
    }

    printf("pipe 建立成功: 讀取端=%d, 寫入端=%d\n\n", pipefd[0], pipefd[1]);

    /*
     * 第一個子行程：執行 ls -l
     * stdout → pipe 寫入端
     */
    pid1 = fork();
    if (pid1 < 0) {
        perror("fork1");
        return 1;
    }

    if (pid1 == 0) {
        /* 子行程 1：ls -l */

        /* stdout → pipe 寫入端 */
        dup2(pipefd[1], 1);
        close(pipefd[0]);
        close(pipefd[1]);

        execlp("ls", "ls", "-l", NULL);

        perror("execlp ls");
        exit(1);
    }

    /*
     * 第二個子行程：執行 wc -l
     * stdin ← pipe 讀取端
     */
    pid2 = fork();
    if (pid2 < 0) {
        perror("fork2");
        return 1;
    }

    if (pid2 == 0) {
        /* 子行程 2：wc -l */

        /* stdin ← pipe 讀取端 */
        dup2(pipefd[0], 0);
        close(pipefd[0]);
        close(pipefd[1]);

        execlp("wc", "wc", "-l", NULL);

        perror("execlp wc");
        exit(1);
    }

    /*
     * 父行程：關閉 pipe，等待兩個子行程結束
     */
    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    printf("\n--- pipe 模擬完成: ls -l | wc -l ---\n");

    return 0;
}
