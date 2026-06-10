# HW5：執行緒、同步與並行程式設計

**本作業使用 AI 輔助**

- **使用 AI**: Claude (opencode)
- **使用方式**: 由 AI 協助產生程式碼架構與文件，本人審閱並修改每一行程式碼與文字
- **參考來源**: 無直接複製他人程式碼，參考 POSIX Threads 文件
- **原創性說明**: 本專案為原創作品，非複製他人程式碼

# HW5：執行緒、同步與並行程式設計

本專案包含以下主題的程式範例與文件說明：

## 目錄結構

```
HW5/
├── README.md                           # 本文件（總覽）
├── doc/
│   └── thread-concepts.md              # 執行緒概念說明文件
├── bank/
│   ├── bank.c                          # 銀行存提款模擬程式
│   └── README.md                       # 程式說明文件
├── producer-consumer/
│   ├── producer_consumer.c             # 生產者消費者問題
│   └── README.md                       # 程式說明文件
└── dining-philosophers/
    ├── dining.c                        # 哲學家用餐問題
    └── README.md                       # 程式說明文件
```

## 主題說明

| 主題 | 說明 |
|------|------|
| **Thread (執行緒)** | 行程內的獨立執行單元，共用相同位址空間 |
| **Race Condition (競爭條件)** | 多執行緒同時存取共享資源導致結果不確定 |
| **Mutex (互斥鎖)** | 確保同一時間只有一個執行緒進入臨界區 |
| **Deadlock (死結)** | 多執行緒互相等待對方持有的資源而無法繼續 |

## 編譯方式

所有 C 程式使用 POSIX Threads（pthread），編譯時需連結 pthread 函式庫：

```bash
# 銀行存提款
gcc -pthread -o bank bank/bank.c && ./bank

# 生產者消費者
gcc -pthread -o producer_consumer producer-consumer/producer_consumer.c && ./producer_consumer

# 哲學家用餐
gcc -pthread -o dining dining-philosophers/dining.c && ./dining
```

或一次編譯全部：

```bash
gcc -pthread -o bank         bank/bank.c
gcc -pthread -o producer_consumer producer-consumer/producer_consumer.c
gcc -pthread -o dining       dining-philosophers/dining.c
```

## 參考資料

- Linux 系統程式 / 02-thread
