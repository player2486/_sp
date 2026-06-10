# 學習報告

## 一、專案動機

這個專案是系統程式課程的期中作業。從 HW1 到 HW7，我學了編譯器、執行緒同步、行程管理、檔案操作等觀念，這個 Shell 專案正好可以綜合運用這些知識。

我參考了陳鍾誠老師的 course0 教材中的系統程式章節，以及 c-classical 專案中的 C 語言程式風格。

## 二、實作歷程

### 第一版：基本命令執行

最簡單的版本：讀取命令 → fork → execvp → wait。

```c
pid_t pid = fork();
if (pid == 0) {
    execvp(cmd->argv[0], cmd->argv);
    exit(1);
}
waitpid(pid, &status, 0);
```

### 第二版：加入管線

實作 `|` 時才真正理解 pipe + dup2 的威力。

```c
pipe(pipefd);
pid_t pid = fork();
if (pid == 0) {
    dup2(pipefd[1], STDOUT_FILENO);
    // 執行左邊命令
}
pid_t pid2 = fork();
if (pid2 == 0) {
    dup2(pipefd[0], STDIN_FILENO);
    // 執行右邊命令
}
```

### 第三版：加入重新導向

支援 `>`、`<`、`>>`、`2>`。

### 第四版：背景執行與信號處理

處理 `&`、SIGINT、SIGCHLD。

## 三、遇到的問題

1. **管線解析**：`ls -l | grep c | wc -l` 要正確分割成三段命令，再用 pipe 串接。
   - 解法：遞迴處理管線，每遇到一個 `|` 就 fork 一組 pipe。

2. **背景行程清理**：子行程結束時要避免變成殭屍。
   - 解法：註冊 SIGCHLD handler，在 handler 中呼叫 `waitpid(-1, NULL, WNOHANG)`。

3. **Stderr 重新導向**：`2>` 與 `>` 的解析不同。
   - 解法：檢查 token 是否為 `2>`，若是則開啟檔案並 `dup2(fd, STDERR_FILENO)`。

## 四、學到的觀念

- fork 後的檔案描述子複製行為
- pipe 的讀寫端關閉時機（避免死鎖）
- SIGCHLD 的非同步處理
- execvp 的 PATH 搜尋機制

## 五、參考資源

- https://github.com/ccc-c/c-classical
- https://github.com/ccckmit/course0/tree/main/code/系統程式
- Linux man pages: fork(2), execvp(3), pipe(2), dup2(2), signal(7)
