# HW7：Process Monitor (行程監控工具)

**本作業使用 AI 輔助**

- **使用 AI**: Claude (opencode)
- **使用方式**: 由 AI 協助產生程式碼架構與報告內容，本人審閱並修改每一行程式碼與文字
- **參考來源**: Linux man pages, /proc filesystem 文件
- **原創性說明**: 本專案為原創作品，非複製他人程式碼

## 簡介

一個類似 `top` 的即時行程監控工具，透過讀取 `/proc` 檔案系統取得系統與行程資訊，以 ANSI escape code 實作即時更新介面。

## 功能

- 顯示所有執行中行程（PID、名稱、狀態、CPU%、MEM%、RSS）
- 即時更新（每秒更新一次）
- 多種排序方式：CPU、MEM、PID、名稱
- 狀態著色：Running(綠)、Sleep(青)、Zombie(紅)
- CPU 使用率計算（透過兩次取樣差值）

## 編譯

```bash
gcc -o hw7-top src/main.c src/procfs.c src/display.c
./hw7-top
```

## 操作

| 按鍵 | 功能 |
|------|------|
| `1` | 依 PID 排序 |
| `2` | 依 CPU% 排序 |
| `3` | 依 MEM% 排序 |
| `4` | 依名稱排序 |
| `q` | 離開 |

## 目錄結構

```
HW7/
├── README.md
├── Makefile
├── src/
│   ├── main.c       # 主程式
│   ├── procfs.c/h   # 讀取 /proc 檔案系統
│   └── display.c/h  # 終端機顯示
└── REPORT.md        # 學習報告
```
