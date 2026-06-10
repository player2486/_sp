# 檔案描述子（File Descriptor）

## 什麼是檔案描述子？

檔案描述子（fd）是一個非負整數，核心用來識別行程開啟的檔案。

```
行程 A 的檔案描述子表：
+------+-------------------+
| fd 0 | stdin  (鍵盤)     |
| fd 1 | stdout (螢幕)    |
| fd 2 | stderr (螢幕)    |
| fd 3 | /home/file.txt   |
| fd 4 | network socket    |
+------+-------------------+
```

## 標準檔案描述子

在 Linux 中，每個行程啟動時自動開啟三個檔案描述子：

| 編號 | 名稱 | 符號常數 | 預設裝置 |
|------|------|---------|---------|
| **0** | 標準輸入 | `STDIN_FILENO` | 鍵盤 |
| **1** | 標準輸出 | `STDOUT_FILENO` | 螢幕 |
| **2** | 標準錯誤 | `STDERR_FILENO` | 螢幕 |

```c
#include <unistd.h>

// 等價關係
read(0, buf, size);    // 從鍵盤讀取
write(1, buf, size);   // 輸出到螢幕
write(2, msg, len);    // 輸出錯誤到螢幕

// 與 printf/fprintf 對應
printf(...)         → write(1, ...)
fprintf(stdout, ...) → write(1, ...)
fprintf(stderr, ...) → write(2, ...)
```

## open(), close(), read(), write()

### open()

```c
#include <fcntl.h>
int open(const char *path, int flags, mode_t mode);
```

| flags | 說明 |
|-------|------|
| `O_RDONLY` | 唯讀 |
| `O_WRONLY` | 唯寫 |
| `O_RDWR` | 讀寫 |
| `O_CREAT` | 不存在則建立 |
| `O_TRUNC` | 清空檔案 |
| `O_APPEND` | 附加模式 |

回傳值：新的檔案描述子編號（>=3），失敗回傳 -1。

### close()

```c
int close(int fd);
```

關閉檔案描述子，釋放核心資源。

### read()

```c
ssize_t read(int fd, void *buf, size_t count);
```

從 fd 讀取最多 count 位元組到 buf。
- 回傳：實際讀取的位元組數
- 回傳 **0**：檔案結尾（EOF）
- 回傳 **-1**：錯誤

### write()

```c
ssize_t write(int fd, const void *buf, size_t count);
```

將 buf 中的 count 位元組寫入 fd。
- 回傳：實際寫入的位元組數
- 回傳 **-1**：錯誤

### 完整範例

```c
int fd = open("test.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
write(fd, "Hello", 5);
close(fd);

fd = open("test.txt", O_RDONLY);
char buf[64];
int n = read(fd, buf, sizeof(buf));
buf[n] = '\0';
printf("讀取: %s\n", buf);  // → 讀取: Hello
close(fd);
```

## dup2()

### 作用

`dup2()` 複製檔案描述子，讓兩個 fd 指向同一個核心檔案表項。

```c
int dup2(int oldfd, int newfd);
```

效果：關閉 newfd（如果已開啟），然後讓 newfd 複製 oldfd 的內容。

```
dup2(fd, 1) 之前：
+------+-------------------+
| fd 1 | stdout (螢幕)    |
| fd 3 | /tmp/output.txt  |
+------+-------------------+

dup2(fd, 1) 之後：
+------+-------------------+
| fd 1 | /tmp/output.txt   | ← fd 3 的檔案被 dup 到 fd 1
| fd 3 | /tmp/output.txt   |
+------+-------------------+
```

這意味著以後 `printf()` 或 `write(1, ...)` 實際上會寫入 `/tmp/output.txt`。

### 用 dup2 重新導向 stdout 到檔案

```c
int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
dup2(fd, 1);       // fd=1 (stdout) 現在指向 output.txt
close(fd);         // 可以關閉原 fd

printf("這行會寫入檔案\n");  // 寫入 output.txt！
```

### 用 dup2 重新導向 stdin 從檔案

```c
int fd = open("input.txt", O_RDONLY);
dup2(fd, 0);       // fd=0 (stdin) 現在指向 input.txt
close(fd);

char buf[256];
scanf("%s", buf);  // 從 input.txt 讀取！
```

### 重新導向 stderr

```c
dup2(1, 2);        // stderr → stdout（都輸出到同一地方）
// 等同於 shell 的 2>&1
```

## pipe()

pipe 建立一個單向資料通道：

```c
int pipe(int pipefd[2]);
// pipefd[0] = 讀取端
// pipefd[1] = 寫入端
```

```
       寫入端         讀取端
pipefd[1] ────────── pipefd[0]
          (核心緩衝區)
```

### fork + pipe

```c
int fd[2];
pipe(fd);

pid_t pid = fork();

if (pid == 0) {
    // 子行程：寫入 pipe
    close(fd[0]);           // 關閉讀取端
    write(fd[1], "hello", 5);
    close(fd[1]);
} else {
    // 父行程：從 pipe 讀取
    close(fd[1]);           // 關閉寫入端
    char buf[64];
    read(fd[0], buf, sizeof(buf));
    close(fd[0]);
}
```

### pipe + dup2：模擬 shell pipe

```c
// 模擬 ls | wc -l
int fd[2];
pipe(fd);

pid_t pid = fork();

if (pid == 0) {
    // 子行程：執行 wc -l
    // wc 從 stdin 讀取 → 把 pipe 讀取端 dup 到 stdin
    dup2(fd[0], 0);     // fd=0 (stdin) → pipe 讀取端
    close(fd[0]);
    close(fd[1]);
    execlp("wc", "wc", "-l", NULL);
} else {
    // 父行程：執行 ls
    // ls 輸出到 stdout → 把 pipe 寫入端 dup 到 stdout
    dup2(fd[1], 1);     // fd=1 (stdout) → pipe 寫入端
    close(fd[0]);
    close(fd[1]);
    execlp("ls", "ls", NULL);
}
```

## /proc 檔案系統

Linux 將所有行程/系統資訊以檔案形式放在 `/proc`：

```bash
cat /proc/self/status      # 當前行程狀態
cat /proc/cpuinfo          # CPU 資訊
cat /proc/meminfo          # 記憶體資訊
ls /proc/PID/fd/           # 查看某行程開啟的 fd
```

## 總結

| 系統呼叫 | 作用 | 常用 flags |
|---------|------|-----------|
| `open()` | 開啟/建立檔案 | O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC |
| `close()` | 關閉 fd | - |
| `read()` | 從 fd 讀取 | - |
| `write()` | 寫入 fd | - |
| `dup2()` | 複製 fd | 用於重新導向 |
| `pipe()` | 建立管道 | 用於行程間通訊 |
