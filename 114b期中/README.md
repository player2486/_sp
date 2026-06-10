# 114b期中：Mini Shell (微殼)

**本作業使用 AI 輔助**

- **使用 AI**: Claude (opencode)
- **使用方式**: 由 AI 協助產生程式碼架構與報告內容，本人審閱並修改每一行程式碼與文字
- **參考來源**:
  - https://github.com/ccc-c/ (c-classical, c5 等專案)
  - https://github.com/ccckmit/course0/tree/main/code (系統程式課程)
  - https://github.com/ccckmit/ (陳鍾誠老師教學資源)
- **原創性說明**: 程式架構由 AI 建議，實作細節、除錯與測試均由本人完成。本專案為原創作品，非複製他人程式碼。

## 專案簡介

這是一個用 C 語言實作的簡易 Shell（微殼），支援以下功能：

- 外部命令執行（fork + execvp）
- 管線（`|`）：多個指令串接
- 重新導向（`>`、`<`、`>>`）
- 背景執行（`&`）
- 內建指令：`cd`、`exit`、`jobs`、`kill`
- 信號處理（SIGINT、SIGCHLD）

## 目錄結構

```
114b期中/
├── README.md         # 本文件
├── Makefile          # 編譯設定
├── src/
│   └── msh.c         # Mini Shell 完整實作 (~500 行)
└── doc/
    ├── report.md     # 學習報告
    └── examples.md   # 執行範例與截圖
```

## 編譯與執行

```bash
make       # 編譯
./msh      # 進入 shell

# 或指定命令執行
./msh -c "ls -l | wc -l"
```

## 功能說明

### 1. 外部命令執行

```bash
msh> ls -la
msh> ps aux
```

使用 `fork()` 建立子行程 → `execvp()` 執行命令 → `waitpid()` 等待結束。

### 2. 管線（Pipe）

```bash
msh> ls -l | grep .c | wc -l
```

支援多段管線，使用 `pipe()` + `dup2()` 串接多個行程。

### 3. 重新導向

```bash
msh> echo hello > output.txt      # stdout → 檔案
msh> cat < input.txt              # stdin ← 檔案
msh> echo more >> output.txt      # stdout → 檔案（附加）
msh> ls nonexist 2> error.log     # stderr → 檔案
```

使用 `dup2()` 重新導向標準輸入/輸出/錯誤。

### 4. 背景執行

```bash
msh> sleep 10 &
[1] 1234
msh> jobs
[1] Running    sleep 10 &
```

背景行程以 `&` 結尾，Shell 不等待其結束。

### 5. 內建指令

| 指令 | 功能 |
|------|------|
| `cd <dir>` | 切換目錄 |
| `exit` | 離開 Shell |
| `jobs` | 顯示背景行程 |
| `kill <pid>` | 傳送 SIGTERM |
| `kill -9 <pid>` | 傳送 SIGKILL |

### 6. 信號處理

- `Ctrl+C`：終止前景行程（不終止 Shell）
- `Ctrl+Z`：暫停前景行程（SUSP）
- 子行程結束時自動清理（避免殭屍）

## 心得

見 [doc/report.md](doc/report.md)
