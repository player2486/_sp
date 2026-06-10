# 第八章：並行與同步

## 並行 vs 平行

- **並行（Concurrency）**：多個任務在時間上交錯執行（單核心也可）
- **平行（Parallelism）**：多個任務在同一時刻執行（需多核心）

## 競爭條件（Race Condition）

當多個執行緒同時存取共享資料，且至少一個執行緒在寫入，可能導致不可預期的結果：

```c
int counter = 0;

void *thread_func(void *arg) {
    for (int i = 0; i < 1000000; i++) {
        counter++;  // 非原子操作！
    }
    return NULL;
}
```

`counter++` 實際上對應三條指令：
1. 從記憶體載入 counter 到暫存器
2. 將暫存器加 1
3. 將暫存器存回記憶體

## 互斥鎖（Mutex）

```c
#include <pthread.h>

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

## 號誌（Semaphore）

由 Dijkstra 提出的同步原語：

```c
#include <semaphore.h>

sem_t sem;
sem_init(&sem, 0, 1);  // 初始值 = 1

sem_wait(&sem);   // P 操作：如果值 > 0 則減 1，否則等待
// 臨界區段
sem_post(&sem);   // V 操作：值加 1，喚醒等待者
```

### 生產者-消費者問題

```c
#define BUFFER_SIZE 10
sem_t empty, full, mutex;
int buffer[BUFFER_SIZE];

void *producer(void *arg) {
    for (int i = 0; ; i++) {
        sem_wait(&empty);     // 確認有空位
        sem_wait(&mutex);     // 進入臨界區
        buffer[in] = i;
        in = (in + 1) % BUFFER_SIZE;
        sem_post(&mutex);     // 離開臨界區
        sem_post(&full);      // 通知消費者
    }
}

void *consumer(void *arg) {
    for (;;) {
        sem_wait(&full);      // 確認有資料
        sem_wait(&mutex);     // 進入臨界區
        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        sem_post(&mutex);     // 離開臨界區
        sem_post(&empty);     // 通知生產者
    }
}
```

## 條件變數（Condition Variable）

```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int ready = 0;

void *waiter(void *arg) {
    pthread_mutex_lock(&mutex);
    while (!ready)
        pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void *signaler(void *arg) {
    pthread_mutex_lock(&mutex);
    ready = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return NULL;
}
```

## 原子操作

硬體層級支援的不可分割操作：

```c
#include <stdatomic.h>

atomic_int counter = 0;

void *thread_func(void *arg) {
    for (int i = 0; i < 1000000; i++) {
        atomic_fetch_add(&counter, 1);
    }
    return NULL;
}
```

## 死結（Deadlock）

四個必要條件：
1. **互斥**：資源一次只能被一個執行緒持有
2. **持有並等待**：執行緒持有資源同時等待其他資源
3. **不可搶佔**：資源不能被強制取走
4. **循環等待**：存在執行緒間的循環等待鏈

### 避免死結

```c
// 固定資源取得順序
void transfer(account *a, account *b, int amount) {
    if (a < b) {
        lock(a->lock);
        lock(b->lock);
    } else {
        lock(b->lock);
        lock(a->lock);
    }
    a->balance -= amount;
    b->balance += amount;
    unlock(a->lock);
    unlock(b->lock);
}
```

## 讀寫鎖

```c
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

void reader() {
    pthread_rwlock_rdlock(&rwlock);
    // 讀取共享資料（多個讀者可以同時）
    pthread_rwlock_unlock(&rwlock);
}

void writer() {
    pthread_rwlock_wrlock(&rwlock);
    // 寫入共享資料（獨佔）
    pthread_rwlock_unlock(&rwlock);
}
```

---

**上一章**：[第七章：輸入輸出系統](07-io-systems.md)
**下一章**：[第九章：編譯器概論](09-compiler.md)
