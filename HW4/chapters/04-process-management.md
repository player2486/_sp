# 第四章：行程管理

## 行程 vs 執行緒

| | 行程（Process） | 執行緒（Thread） |
|---|---------------|----------------|
| 定義 | 執行中的程式實例 | 行程內的執行單元 |
| 資源 | 獨立的位址空間 | 共用行程資源 |
| 切換成本 | 高（需切換頁表） | 低 |
| 通訊方式 | IPC（pipe、socket 等） | 直接共用記憶體 |
| 獨立性 | 完全隔離 | 共用相同位址空間 |

## 行程狀態

```
   ┌─── 新建 ──→ 就緒 ──→ 執行 ──→ 終止
   │              ↑        │
   │              │        ↓
   │              └── 等待 ←─┘
   │                         │
   └─────────────────────────┘
```

- **新建**（New）：行程被建立
- **就緒**（Ready）：等待 CPU 排程
- **執行**（Running）：正在使用 CPU
- **等待**（Waiting）：等待 I/O 或事件
- **終止**（Terminated）：執行完畢

## 行程控制區塊（PCB）

核心為每個行程維護的資料結構：

```c
struct task_struct {
    pid_t pid;                    // 行程 ID
    long state;                   // 狀態
    struct mm_struct *mm;         // 記憶體資訊
    struct files_struct *files;   // 開啟的檔案
    struct list_head tasks;       // 行程串列
    struct task_struct *parent;   // 父行程
    struct list_head children;    // 子行程
    // ... 更多欄位
};
```

## 排程演算法

### FCFS（First-Come, First-Served）

先到的先執行，簡單但平均等待時間長。

### SJF（Shortest Job First）

最短的任務先執行，理論上最佳但無法預知執行時間。

### 優先權排程

每個行程有優先權，優先權高的先執行。

### Round Robin（RR）

每個行程獲得固定時間片（time quantum），依序輪流。

### 多層佇列（MLQ）

將行程分為不同類別（前景、背景），各有不同排程策略。

## Linux 排程器：CFS

Linux 使用完全公平排程器（Completely Fair Scheduler）：

- 以紅黑樹（red-black tree）管理就緒行程
- 追蹤每個行程的 `vruntime`（虛擬執行時間）
- 每次選取 `vruntime` 最小的行程執行

## 範例：行程建立

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        // 子行程
        printf("子行程：PID = %d, 父行程 PID = %d\n",
               getpid(), getppid());
    } else if (pid > 0) {
        // 父行程
        printf("父行程：PID = %d, 子行程 PID = %d\n",
               getpid(), pid);
        wait(NULL);  // 等待子行程結束
    } else {
        perror("fork 失敗");
        return 1;
    }
    return 0;
}
```

## 範例：執行緒建立（POSIX）

```c
#include <stdio.h>
#include <pthread.h>

void *thread_func(void *arg) {
    int id = *(int *)arg;
    printf("執行緒 %d 執行中\n", id);
    return NULL;
}

int main() {
    pthread_t t1, t2;
    int id1 = 1, id2 = 2;

    pthread_create(&t1, NULL, thread_func, &id1);
    pthread_create(&t2, NULL, thread_func, &id2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("所有執行緒完成\n");
    return 0;
}
```

編譯：
```bash
gcc -pthread thread_example.c -o thread_example
```

---

**上一章**：[第三章：作業系統基礎](03-os-basics.md)  
**下一章**：[第五章：記憶體管理](05-memory-management.md)
