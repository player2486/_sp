# 學習報告

## 一、專案動機

這是系統程式課程的期中專題。從 HW1 的編譯器、HW5 的執行緒同步、HW6 的行程與檔案操作一路學來，我希望能做一個整合性的工具，把所學的系統程式知識付諸實作。

選擇 Process Monitor（行程監控工具）的原因：
1. **實用性**：`top` 是每個系統管理者天天在用的工具，實作它很有成就感
2. **整合性**：涵蓋了檔案 I/O、行程管理、系統資訊讀取等多個面向
3. **可擴展**：可以持續加入更多功能（網路監控、磁碟 I/O 等）

## 二、實作歷程

### 第一版：讀取 /proc 基本資訊

先理解 `/proc` 檔案系統的結構。Linux 把行程、CPU、記憶體等系統資訊都以「檔案」的形式暴露出來。

```bash
cat /proc/stat       # CPU 時間
cat /proc/meminfo    # 記憶體
cat /proc/1/stat     # init 行程狀態
```

用 `fopen` + `fgets` 讀取這些檔案，再用 `sscanf` 解析。

### 第二版：解析 /proc/[pid]/stat

這是最困難的部分。格式是空格的連續數值，但第二欄（行程名稱）用括號包圍且可能含空格：

```
1234 (program name) R 0 0 0 ...
```

我的解法：找到 `(` 和 `)` 的位置，手動擷取名稱，再從後面繼續 sscanf。

### 第三版：顯示與更新

使用 ANSI escape code 控制畫面。關鍵技巧：
- `\033[H` 回到左上角 → 達到「更新」效果
- `tcsetattr()` 設定 raw mode → 即時讀取按鍵

### 第四版：CPU 使用率計算

需要兩次取樣：

1. 記錄第一次的 CPU 時間與各行程時間
2. 等一秒
3. 記錄第二次的資料
4. `cpu% = (process_time_diff / total_cpu_diff) × 100`

## 三、遇到的問題與解法

| 問題 | 解法 |
|------|------|
| `/proc/[pid]/stat` 名稱含空格無法用 sscanf | 手動找 `()` 位置 |
| 多執行緒輸出交錯 | 用 mutex 保護 |
| 終端機畫面閃爍 | 先清除再重繪，每秒一次 |
| RSS 顯示負數 | 修正 sscanf 欄位對齊 |

## 四、學到的觀念

1. **Linux 一切皆檔案**：連系統資訊都可以用 read/write 操作
2. **行程在核心中的資料結構**：PCB 中的 PID、狀態、CPU 時間、記憶體
3. **ANSI terminal control**：不需要 ncurses 也能做出 TUI
4. **取樣與差值計算**：CPU% 不是直接讀取的，而是透過兩次取樣計算

## 五、參考資源

- https://github.com/ccc-c/ (c-classical 專案中的 C 語言風格)
- https://github.com/ccckmit/course0/tree/main/code/系統程式
- Linux man pages: proc(5), termios(3), ioctl(2)
- /proc 文件系統：https://www.kernel.org/doc/html/latest/filesystems/proc.html
