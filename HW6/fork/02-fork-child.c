/*
 * HW6 - fork/02-fork-child.c
 *
 * 父子行程關係：展示 fork 後父子行程的獨立性。
 * 子行程修改變數不影響父行程（copy-on-write）。
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    int x = 100;  // 在 fork 前定義的變數

    printf("【fork 前】x = %d (位址: %p)\n\n", x, (void *)&x);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork 失敗");
        return 1;
    }

    if (pid == 0) {
        /* 子行程 */
        printf("【子行程】一開始 x = %d\n", x);

        x = 200;  /* 子行程修改 x — 只影響子行程自己的副本 */
        printf("【子行程】修改 x = %d (位址: %p)\n", x, (void *)&x);
        printf("【子行程】結束\n");
    } else {
        /* 父行程 */
        printf("【父行程】一開始 x = %d\n", x);
        printf("【父行程】睡眠 1 秒，讓子行程先修改 x...\n");
        sleep(1);

        /* 父行程的 x 不受子行程影響！*/
        printf("【父行程】x 仍然是 %d (位址: %p)\n", x, (void *)&x);
        printf("【父行程】子行程的修改不影響父行程\n");

        wait(NULL);  /* 等待子行程結束 */
        printf("\n【父行程】子行程已結束\n");
    }

    return 0;
}
