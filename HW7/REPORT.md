# 學習報告

## 實作重點

### /proc 檔案系統

Linux 將所有系統資訊以檔案形式暴露在 `/proc`：

- `/proc/stat`：CPU 使用時間
- `/proc/meminfo`：記憶體資訊
- `/proc/[pid]/stat`：行程狀態與資源使用
- `/proc/[pid]/status`：行程詳細資訊

### CPU 使用率計算

透過兩次取樣的差值計算：

```c
total_diff = curr_cpu.total - prev_cpu.total;
time_diff = curr_proc.total_time - prev_proc.total_time;
cpu_percent = time_diff / total_diff * 100;
```

### 終端機顯示

使用 ANSI escape code 控制游標：

- `\033[H`：移動到左上角
- `\033[J`：清除畫面
- `\033[?25l`：隱藏游標
- `\033[31m`：設定顏色

## 遇到的問題

1. `/proc/[pid]/stat` 解析：程式名稱用括號包圍且可能含空格，不能直接用 sscanf 以空格分割
   - 解法：手動找到 `(` 和 `)` 的位置

2. 終端機 raw mode：需要即時讀取按鍵
   - 解法：使用 `tcsetattr` 關閉 ICANON 和 ECHO
