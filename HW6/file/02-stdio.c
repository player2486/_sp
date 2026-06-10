/*
 * HW6 - file/02-stdio.c
 *
 * 標準輸入 (0)、標準輸出 (1)、標準錯誤 (2) 的操作。
 *
 * stdin  = fd 0 = 鍵盤輸入
 * stdout = fd 1 = 螢幕輸出
 * stderr = fd 2 = 螢幕錯誤輸出
 *
 * 所有裝置在 Linux 中都是檔案，可以用 read/write 操作！
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main()
{
    /* === stdout (fd=1) === */

    /* 直接用 write 輸出到 stdout */
    write(1, "=== stdout (fd=1) ===\n", 22);
    write(1, "這行用 write(1, ...) 輸出\n", 28);

    /* printf 底層也是 write(1, ...) */
    printf("這行用 printf() 輸出\n");

    /* fprintf 也可以指定 stdout */
    fprintf(stdout, "這行用 fprintf(stdout) 輸出\n\n");

    /* === stderr (fd=2) === */

    /* stderr 和 stdout 都輸出到螢幕，但分開 */
    write(2, "=== stderr (fd=2) ===\n", 22);
    write(2, "這行用 write(2, ...) 輸出\n", 28);

    fprintf(stderr, "這行用 fprintf(stderr) 輸出\n\n");

    /*
     * stderr vs stdout 的差異：
     *
     * stdout 有緩衝，stderr 無緩衝
     * 重新導向時可分別處理：
     *   ./a.out > output.txt       # stdout → output.txt
     *   ./a.out 2> error.txt      # stderr → error.txt
     *   ./a.out &> all.txt        # 兩者都 → all.txt
     */

    /* === stdin (fd=0) === */

    write(1, "=== stdin (fd=0) ===\n", 21);
    write(1, "請輸入一行文字: ", 18);

    char buf[256];
    ssize_t n = read(0, buf, sizeof(buf));  /* 從 stdin 讀取 */

    if (n > 0) {
        buf[n] = '\0';
        write(1, "你輸入了: ", 11);
        write(1, buf, n);
    }

    return 0;
}
