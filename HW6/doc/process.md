# Linux 行程管理

## 行程（Process）

行程是執行中的程式實例。每個行程有獨立的 PID（Process ID）、位址空間、檔案描述子表等資源。

```
+------------------+
|     PCB          |  → process control block (struct task_struct in Linux)
|  - PID           |
|  - PPID          |
|  - 狀態          |
|  - 檔案描述子表   |
|  - 記憶體對映     |
|  - 暫存器        |
+------------------+
```

## fork()

`fork()` 是 Linux 中建立新行程的唯一方式（除了一些特殊情況）。

```c
pid_t fork(void);
```

- 回傳值：
  - **-1**：失敗
  - **0**：在子行程中回傳
  - **>0**：在父行程中回傳（值為子行程 PID）

### fork 做了什麼？

```
fork() 前：
+------------------+
|    父行程        |
|  PID = 100       |
|  程式碼           |
|  資料             |
|  堆積 / 堆疊      |
|  檔案描述子表      |
+------------------+

fork() 後：
+------------------+    +------------------+
|    父行程        |    |    子行程        |
|  PID = 100       |    |  PID = 101       |
|  程式碼 (共享)    |    |  程式碼 (共享)    |
|  資料 (複製)      |    |  資料 (複製)      |
|  堆積 / 堆疊 (複製)|    |  堆積 / 堆疊 (複製)|
|  檔案描述子表 (複製) |    | 檔案描述子表 (複製) |
+------------------+    +------------------+
```

注意：fork 使用 **copy-on-write (COW)** 技術，只有在寫入時才真正複製記憶體頁面。

### fork 範例

```c
pid_t pid = fork();

if (pid == 0) {
    // 子行程
    printf("我是子行程, PID=%d\n", getpid());
} else if (pid > 0) {
    // 父行程
    printf("我是父行程, 子行程 PID=%d\n", pid);
} else {
    perror("fork 失敗");
}
```

## execvp()

`execvp()` 系列函式用於取代當前行程的程式映像。

```c
int execvp(const char *file, char *const argv[]);
```

- 執行成功**不返回**（程式被完全取代）
- 執行失敗回傳 -1

```
execvp() 前：
+------------------+
|    行程          |
|  程式 = a.out    |
|  程式碼 (main)    |
+------------------+

execvp("/bin/ls", args) 後：
+------------------+
|    同一個行程     |
|  PID 不變！       |
|  程式 = /bin/ls   |
|  程式碼 (ls 的)   |
+------------------+
```

### fork + execvp 組合

```c
pid_t pid = fork();

if (pid == 0) {
    // 子行程：取代為 ls
    execlp("ls", "ls", "-l", NULL);
    perror("exec 失敗");  // 只有在 exec 失敗才會執行
    exit(1);
} else if (pid > 0) {
    // 父行程：等待子行程結束
    wait(NULL);
}
```

這是 Unix/Linux 建立新程式的標準模式。

## wait() 與 waitpid()

父行程用於等待子行程結束：

```c
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);
```

## 殭屍行程（Zombie）

當子行程結束但父行程尚未呼叫 `wait()`，子行程的 PCB 仍保留在核心中：

```
子行程結束 → 子行程變成殭屍
              ↓
父行程未呼叫 wait() → 殭屍留在系統中
              ↓
父行程呼叫 wait() 或父行程結束 → 殭屍被清除
```

殭屍行程**不佔用記憶體**（除 PCB 外），但**佔用 PID**。

### 產生殭屍

```c
pid_t pid = fork();

if (pid == 0) {
    exit(0);        // 子行程立即結束 → 變成殭屍
}
sleep(10);          // 父行程不呼叫 wait → 10 秒內的殭屍
wait(NULL);         // 清除殭屍
```

## 孤兒行程（Orphan）

當父行程先於子行程結束，子行程變成孤兒，由 `init`（PID=1）收養：

```
父行程結束（沒有呼叫 wait）
    ↓
子行程變成「孤兒」
    ↓
init (PID=1) 收養子行程
    ↓
init 自動呼叫 wait() 清理子行程
```

```c
pid_t pid = fork();

if (pid == 0) {
    sleep(10);      // 子行程睡眠
    // 此時父行程已結束，我的 PPID 變成 1
    printf("我的父行程 PID: %d\n", getppid());
}
exit(0);            // 父行程立即結束
```

## 總結

| 函式 | 作用 | 特點 |
|------|------|------|
| `fork()` | 建立子行程 | 回傳兩次（0 在子行程，>0 在父行程） |
| `execvp()` | 取代行程映像 | 成功不返回 |
| `wait()` | 等待子行程 | 清除殭屍行程 |
| `getpid()` | 取得當前 PID | - |
| `getppid()` | 取得父行程 PID | 孤兒行程回傳 1 |
