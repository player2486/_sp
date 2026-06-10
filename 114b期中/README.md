# 114b期中：Process Monitor (行程監控工具)

**本作業使用 AI 輔助**

- **使用 AI**: Claude (opencode)
- **使用方式**: 由 AI 協助產生程式碼架構與報告內容，本人審閱並修改每一行程式碼與文字
- **參考來源**:
  - https://github.com/ccc-c/ (c-classical, c5 等專案)
  - https://github.com/ccckmit/course0/tree/main/code (系統程式課程)
  - https://github.com/ccckmit/ (陳鍾誠老師教學資源)
  - Linux man pages, /proc filesystem 文件
- **原創性說明**: 程式架構由 AI 建議，實作細節、除錯與測試均由本人完成。本專案為原創作品，非複製他人程式碼。

## 簡介

這是一個類似 `top` 的即時行程監控工具，透過讀取 Linux `/proc` 檔案系統取得系統與行程資訊，以 ANSI escape code 實作即時更新介面。

本專案整合了系統程式課程中的多項核心概念：
- **檔案操作**：open/read/close 讀取 `/proc` 檔案系統
- **行程管理**：讀取行程狀態、PID、記憶體使用
- **同步處理**：時序控制與資料更新
- **系統呼叫**：全面應用 Linux 系統呼叫

## 目錄結構

```
114b期中/
├── README.md               # 本文件
├── Makefile                # 編譯設定
├── src/
│   ├── main.c              # 主程式（顯示迴圈、按鍵處理）
│   ├── procfs.c / procfs.h # 讀取 /proc 檔案系統模組
│   └── display.c / display.h # 終端機顯示模組（ANSI escape）
└── doc/
    ├── report.md           # 學習報告
    └── examples.md         # 執行範例
```

## 編譯與執行

```bash
make          # 編譯
./hw7-top     # 執行（也可從 HW7 目錄執行）

# 或直接編譯
gcc -o hw7-top src/main.c src/procfs.c src/display.c
./hw7-top
```

## 功能

| 功能 | 說明 |
|------|------|
| 行程列表 | 顯示 PID、名稱、狀態、CPU%、MEM%、RSS |
| 即時更新 | 每秒更新一次 |
| CPU 使用率 | 透過兩次取樣差值計算 |
| 排序 | 依 CPU%、MEM%、PID、名稱排序 |
| 狀態著色 | Running(綠)、Sleep(青)、Zombie(紅) |
| 系統資訊 | 總行程數、CPU 時間、記憶體使用 |

## 操作

| 按鍵 | 功能 |
|------|------|
| `1` | 依 PID 排序 |
| `2` | 依 CPU% 排序 |
| `3` | 依 MEM% 排序 |
| `4` | 依名稱排序 |
| `q` / `Q` | 離開 |

## 實作重點

### 讀取 /proc 檔案系統

```
/proc/stat          → CPU 使用時間（user, system, idle, ...）
/proc/meminfo       → 記憶體資訊（MemTotal, MemFree, MemAvailable）
/proc/[pid]/stat    → 行程狀態、CPU 時間、RSS
```

### CPU 使用率計算

```c
total_diff = curr_cpu.total - prev_cpu.total;
time_diff = curr_proc.total_time - prev_proc.total_time;
cpu_percent = time_diff / total_diff * 100;
```

### ANSI Escape Code 顯示

使用跳脫序列控制游標位置與顏色：
- `\033[H`：游標歸位
- `\033[J`：清除畫面
- `\033[?25l`：隱藏游標
- `\033[31m`：設定文字顏色

### 終端機 Raw Mode

```c
struct termios raw = old_term;
raw.c_lflag &= ~(ECHO | ICANON);
tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
```

## 學習心得

見 [doc/report.md](doc/report.md)
