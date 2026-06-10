/*
 * HW6 - file/03-dup2-redirect.c
 *
 * dup2 重新導向示範：
 *
 * 1. 將 stdout (fd=1) 重新導向到檔案
 *    → printf() 和 write(1, ...) 寫入檔案而非螢幕
 *
 * 2. 將 stderr (fd=2) 重新導向到檔案
 *    → fprintf(stderr, ...) 寫入檔案
 *
 * 3. 將 stdin (fd=0) 重新導向從檔案讀取
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

void redirect_stdout_to_file()
{
    int fd = open("stdout.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open 失敗");
        return;
    }

    /*
     * dup2(fd, 1)：
     *   1. 關閉 fd=1 (stdout)
     *   2. 讓 fd=1 指向 fd 的檔案
     *
     * 從此所有 write(1, ...) 或 printf() 都寫入檔案
     */
    dup2(fd, 1);
    close(fd);  /* 關閉原 fd，不再需要 */

    printf("這行被重新導向寫入檔案 stdout.txt\n");
    write(1, "這行也是（透過 write）\n", 22);
}

void redirect_stderr_to_file()
{
    int fd = open("stderr.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    dup2(fd, 2);
    close(fd);

    fprintf(stderr, "這行 stderr 被導向到 stderr.txt\n");
}

void redirect_stdin_from_file()
{
    int fd = open("stdout.txt", O_RDONLY);
    dup2(fd, 0);
    close(fd);

    /* 從 stdin 讀取 → 實際從 stdout.txt 讀取 */
    char buf[256];
    fgets(buf, sizeof(buf), stdin);
    printf("從檔案讀取: %s", buf);
}

int main()
{
    printf("=== dup2 重新導向示範 ===\n\n");

    printf("--- 1. stdout → 檔案 ---\n");
    redirect_stdout_to_file();

    /* 恢復 stdout */
    int fd = open("/dev/tty", O_WRONLY);
    dup2(fd, 1);
    close(fd);

    printf("--- 2. stderr → 檔案 ---\n");
    redirect_stderr_to_file();

    /* 恢復 stderr */
    fd = open("/dev/tty", O_WRONLY);
    dup2(fd, 2);
    close(fd);

    printf("--- 3. stdin ← 檔案 ---\n");
    redirect_stdin_from_file();

    printf("\n請查看當前目錄的 stdout.txt 和 stderr.txt\n");

    return 0;
}
