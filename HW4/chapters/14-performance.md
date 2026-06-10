# 第十四章：效能優化與分析

## 效能分析工具

### 基本工具

```bash
# CPU 使用率
top / htop
mpstat -P ALL 1

# 記憶體
free -h
vmstat 1

# I/O
iostat -x 1
iotop

# 網路
netstat -s
sar -n DEV 1
```

### 進階分析工具

#### perf（Linux Performance Events）

```bash
# 統計 CPU 事件
perf stat ./my_program

# 取樣分析
perf record ./my_program
perf report

# 即時監控
perf top

# 追蹤特定函式
perf probe --add 'my_function'
perf record -e probe:my_function -aR sleep 1
```

#### strace

```bash
# 追蹤系統呼叫
strace -c -p PID
```

#### Valgrind

```bash
# 記憶體錯誤偵測
valgrind --tool=memcheck ./my_program

# 快取分析
valgrind --tool=cachegrind ./my_program

# 呼叫圖
valgrind --tool=callgrind ./my_program
```

## 程式碼層級最佳化

### 減少分支預測失敗

```c
// 不好的方式：不可預測的分支
for (int i = 0; i < n; i++) {
    if (data[i] > threshold) {
        sum += data[i];
    }
}

// 好的方式：移除分支（使用條件移動指令）
for (int i = 0; i < n; i++) {
    int diff = data[i] - threshold;
    int mask = diff >> 31;  // 符號延伸
    sum += data[i] & ~mask;
}
```

### 迴圈展開（Loop Unrolling）

```c
// 展開前
for (int i = 0; i < n; i++) {
    sum += arr[i];
}

// 展開後（減少迴圈控制開銷）
for (int i = 0; i < n; i += 4) {
    sum += arr[i];
    sum += arr[i+1];
    sum += arr[i+2];
    sum += arr[i+3];
}
```

### SIMD 向量化

```c
// 使用 GCC vector extension
typedef int v4si __attribute__((vector_size(16)));

v4si a, b, c;
c = a + b;  // 一次相加 4 個整數

// 使用 Intel Intrinsics
#include <immintrin.h>

__m256 sum = _mm256_setzero_ps();
for (int i = 0; i < n; i += 8) {
    __m256 v = _mm256_loadu_ps(&data[i]);
    sum = _mm256_add_ps(sum, v);
}
```

## 記憶體存取最佳化

### 快取行對齊

```c
// 對齊到快取行（64 bytes）
struct alignas(64) CacheLine {
    int data;
    char padding[60];  // 避免 false sharing
};
```

### 預先擷取（Prefetch）

```c
// 手動預先擷取
for (int i = 0; i < n; i++) {
    __builtin_prefetch(&data[i + 8], 0, 1);
    sum += data[i];
}
```

## 並行最佳化

### 鎖的粒度

```c
// 粗粒度鎖（競爭激烈）
pthread_mutex_lock(&global_lock);
for (int i = 0; i < n; i++)
    result[i] = compute(i);
pthread_mutex_unlock(&global_lock);

// 細粒度鎖（分散競爭）
for (int i = 0; i < n; i++)
    result[i] = compute(i);
// 不需要鎖！每個執行緒處理不同部分
```

### 無鎖程式設計

```c
#include <stdatomic.h>

// Lock-free stack
typedef struct node {
    int value;
    struct node *next;
} Node;

_Atomic(Node *) head = NULL;

void push(int value) {
    Node *new_node = malloc(sizeof(Node));
    new_node->value = value;

    do {
        new_node->next = atomic_load(&head);
    } while (!atomic_compare_exchange_weak(
        &head, &new_node->next, new_node));
}

Node *pop() {
    Node *old_head;
    do {
        old_head = atomic_load(&head);
        if (!old_head) return NULL;
    } while (!atomic_compare_exchange_weak(
        &head, &old_head, old_head->next));
    return old_head;
}
```

## 效能分析流程

1. **定義基準**：設定效能目標
2. **測量**：使用工具找出瓶頸
3. **分析**：確定根本原因
4. **最佳化**：針對瓶頸改善
5. **驗證**：確認改善有效且正確

### 範例

```bash
# 1. 使用 perf 找出熱點
perf record ./myapp
perf report
# → 發現 60% 時間在 strcpy

# 2. 分析原因
# → 大量字串複製操作

# 3. 最佳化
# → 改用指標或 string_view

# 4. 驗證
perf stat ./myapp_optimized
# → 效能提升 3 倍
```

## 結論

系統程式設計是電腦科學的基石。從硬體架構、作業系統到編譯器、網路和虛擬化，理解這些底層機制能讓你寫出更高效、更可靠、更安全的軟體。

本書涵蓋了系統程式的主要面向，但這只是起點。持續學習、動手實作、閱讀原始碼，才是精進的不二法門。

---

**上一章**：[第十三章：虛擬化與容器](13-virtualization.md)
