# 執行緒、競爭條件、互斥鎖與死結

## 一、執行緒（Thread）

### 什麼是執行緒？

執行緒是作業系統能夠進行運算排程的最小單位。一個行程（Process）可以包含多個執行緒，這些執行緒共用相同的位址空間（包括程式碼、資料、堆積），但每個執行緒有自己的堆疊和一組暫存器。

```
行程（Process）
+------------------------------------------+
| 程式碼段（Code）                          |
| 資料段（Data）                            |
| 堆積（Heap）                              |
+------------------------------------------+
| 執行緒 1      | 執行緒 2      | 執行緒 3  |
| 堆疊          | 堆疊          | 堆疊      |
| 暫存器        | 暫存器        | 暫存器    |
+------------------------------------------+
```

### 行程 vs 執行緒

| 特性 | 行程 | 執行緒 |
|------|------|--------|
| 位址空間 | 獨立 | 共用 |
| 資源開銷 | 大 | 小 |
| 切換速度 | 慢 | 快 |
| 通訊方式 | IPC（pipe、socket） | 直接讀寫記憶體 |
| 獨立性 | 完全隔離 | 同一行程內互相影響 |

### POSIX 執行緒 API

```c
#include <pthread.h>

pthread_t tid;
pthread_create(&tid, NULL, thread_func, arg);   // 建立執行緒
pthread_join(tid, NULL);                         // 等待執行緒結束
pthread_detach(tid);                             // 分離執行緒（自動釋放資源）
```

## 二、競爭條件（Race Condition）

### 定義

當多個執行緒同時存取共享資料，且至少有一個執行緒在寫入資料時，若沒有適當的同步機制，程式的執行結果將取決於執行緒的執行順序，這種不確定性稱為競爭條件。

### 範例

```c
int counter = 0;  // 共享變數

void *thread_func(void *arg) {
    for (int i = 0; i < 1000000; i++) {
        counter++;  // 非原子操作！
    }
    return NULL;
}
```

`counter++` 在機器碼層級實際包含三個步驟：

```
1. MOV  R1, counter     ← 從記憶體讀取 counter 到暫存器
2. ADD  R1, 1           ← 暫存器加 1
3. MOV  counter, R1     ← 將結果存回記憶體
```

當兩個執行緒同時執行時，可能發生以下交錯：

```
執行緒 A                    執行緒 B
MOV R1, counter (=0)
ADD R1, 1         (=1)
                             MOV R1, counter (=0) ← 讀到舊值！
                             ADD R1, 1         (=1)
                             MOV counter, R1   (=1)
MOV counter, R1   (=1)       ← counter 應為 2，結果卻是 1！
```

### 競爭條件的解決

- **互斥鎖（Mutex）**：保護臨界區
- **號誌（Semaphore）**：控制存取數量
- **原子操作（Atomic Operation）**：硬體層級的不可分割操作

## 三、互斥鎖（Mutex）

### 定義

Mutex（Mutual Exclusion）是最基本的同步原語，確保同一時間只有一個執行緒可以進入臨界區段（Critical Section）。

### API

```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // 靜態初始化

pthread_mutex_lock(&mutex);    // 鎖定（若已被鎖則阻塞等待）
// 臨界區段（Critical Section）
pthread_mutex_unlock(&mutex);  // 解鎖

pthread_mutex_destroy(&mutex); // 銷毀
```

### 使用原則

1. **最小化臨界區**：只在必要時才持有鎖
2. **一致鎖定順序**：避免死結
3. **異常安全**：確保解鎖一定會被執行
4. **不可重入**：同一個執行緒不能對未解鎖的 mutex 再次鎖定（普通 mutex）

### 範例（修正後的 counter）

```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;

void *thread_func(void *arg) {
    for (int i = 0; i < 1000000; i++) {
        pthread_mutex_lock(&mutex);
        counter++;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}
```

## 四、死結（Deadlock）

### 定義

死結是指兩個以上的執行緒互相等待對方持有的資源，導致所有相關執行緒永遠無法繼續執行。

### 四個必要條件

| 條件 | 說明 |
|------|------|
| 1. 互斥（Mutual Exclusion） | 資源一次只能被一個執行緒持有 |
| 2. 持有並等待（Hold and Wait） | 執行緒持有資源的同時等待其他資源 |
| 3. 不可搶佔（No Preemption） | 資源不能被強制從執行緒手中取走 |
| 4. 循環等待（Circular Wait） | 存在執行緒間的循環等待鏈 |

### 示意圖

```
執行緒 A                   執行緒 B
   │                         │
   ├── 持有鎖 1              ├── 持有鎖 2
   │                         │
   ├── 等待鎖 2 ──────┬──────┼── 等待鎖 1
   │                   循環   │
   └─────────────────────────┘
```

### 死結預防策略

1. **固定鎖定順序**：所有執行緒以相同順序取得鎖

```c
// 錯誤方式（可能死結）
// 執行緒 A：lock(a) → lock(b)
// 執行緒 B：lock(b) → lock(a)

// 正確方式（固定順序）
// 執行緒 A：lock(a) → lock(b)
// 執行緒 B：lock(a) → lock(b)  ← 與 A 順序一致
```

2. **嘗試鎖定（Trylock）**：使用 `pthread_mutex_trylock()`，若無法取得則釋放已持有的鎖

3. **一次取得所有資源**：要求執行緒必須一次取得所有需要的資源才能開始

### 死結避免 vs 死結預防

- **預防**：透過破壞四個必要條件之一來避免死結
- **避免**：使用銀行家演算法等動態判斷資源分配是否安全
- **偵測與恢復**：允許死結發生，偵測到後強制終止或釋放資源

## 總結

| 概念 | 關鍵重點 |
|------|---------|
| Thread | 輕量級執行單元，共用位址空間 |
| Race Condition | 同步不當導致結果不確定 |
| Mutex | 保護臨界區，確保互斥存取 |
| Deadlock | 循環等待資源，需預防設計 |

正確使用這些同步機制是撰寫可靠並行程式的關鍵。
