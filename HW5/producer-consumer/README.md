# 生產者消費者問題程式說明

## 問題描述

經典的同步問題，由 Dijkstra 提出：
- **生產者**：生產資料放入有限的緩衝區
- **消費者**：從緩衝區取出資料處理

需滿足的條件：
1. 緩衝區滿時，生產者不能放入資料（需等待）
2. 緩衝區空時，消費者不能取出資料（需等待）
3. 同一時間只有一個執行緒可以操作緩衝區

## 實作方式

### 環形緩衝區

使用陣列模擬環形緩衝區（circular buffer）：

```c
typedef struct {
    int buffer[BUFFER_SIZE];
    int in;                    // 下一個生產位置
    int out;                   // 下一個消費位置
    int count;                 // 目前項目數量
    pthread_mutex_t mutex;     // 互斥鎖
    pthread_cond_t not_full;   // 條件變數：緩衝區未滿
    pthread_cond_t not_empty;  // 條件變數：緩衝區非空
} RingBuffer;
```

### 同步機制

使用 **Mutex + 條件變數**（Condition Variable）：

**生產者流程：**
1. 鎖定 mutex
2. 若緩衝區已滿 → 等待 `not_full` 條件變數
3. 放入資料到 `buffer[in]`，移動 `in` 指標
4. 通知消費者：發送 `not_empty` 信號
5. 解鎖 mutex

**消費者流程：**
1. 鎖定 mutex
2. 若緩衝區為空 → 等待 `not_empty` 條件變數
3. 從 `buffer[out]` 取出資料，移動 `out` 指標
4. 通知生產者：發送 `not_full` 信號
5. 解鎖 mutex

### 為什麼使用條件變數而非 busy waiting？

```c
// 不好的方式：busy waiting（浪費 CPU）
while (rb->count >= BUFFER_SIZE)
    ;  // 空轉！

// 好的方式：條件變數（執行緒休眠，不佔用 CPU）
while (rb->count >= BUFFER_SIZE)
    pthread_cond_wait(&rb->not_full, &rb->mutex);
```

條件變數讓執行緒在等待時休眠，收到信號才被喚醒，節省 CPU 資源。

## 執行結果範例

```
生產者消費者問題模擬
緩衝區大小: 8
生產/消費數量: 20

生產者 → [ 0]  (緩衝區: 1/8)
生產者 → [ 1]  (緩衝區: 2/8)
消費者 ← [ 0]  (緩衝區: 1/8)
生產者 → [ 2]  (緩衝區: 2/8)
...
✓ 生產者消費者模擬完成
```
