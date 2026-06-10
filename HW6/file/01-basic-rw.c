/*
 * HW6 - file/01-basic-rw.c
 *
 * open / read / write / close 基本操作。
 *
 * 流程：
 *   1. open → 建立/清空檔案
 *   2. write → 寫入資料
 *   3. close → 關閉
 *   4. open → 唯讀開啟
 *   5. read → 讀取資料
 *   6. close → 關閉
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main()
{
    const char *filename = "hw6-test.txt";
    const char *message  = "HW6: 系統程式檔案操作\n";

    /* === 寫入 === */

    /*
     * O_WRONLY  : 唯寫
     * O_CREAT   : 不存在則建立
     * O_TRUNC   : 清空檔案
     * 0644      : 權限 rw-r--r--
     */
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open 寫入失敗");
        return 1;
    }

    printf("開啟檔案（寫入）: fd=%d\n", fd);

    ssize_t written = write(fd, message, strlen(message));
    printf("寫入 %ld bytes\n", (long)written);

    close(fd);
    printf("關閉檔案\n\n");

    /* === 讀取 === */

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open 讀取失敗");
        return 1;
    }

    printf("開啟檔案（讀取）: fd=%d\n", fd);

    char buf[128];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);

    if (n < 0) {
        perror("read 失敗");
        close(fd);
        return 1;
    }

    buf[n] = '\0';  /* 字串結尾 */
    printf("讀取 %ld bytes: %s", (long)n, buf);

    close(fd);
    printf("關閉檔案\n");

    /* 清理測試檔案 */
    unlink(filename);

    return 0;
}
