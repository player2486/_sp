# 第十三章：虛擬化與容器

## 虛擬化技術

### 什麼是虛拟化？

虛擬化是在單一實體機器上執行多個虛擬機器（VM）的技術，每個 VM 執行自己的作業系統。

### Type 1 vs Type 2 Hypervisor

```
Type 1（裸機）：
+--------+ +--------+
|  VM 1  | |  VM 2  |
+--------+ +--------+
|    Hypervisor      |
|      硬體          |
+--------------------+

Type 2（託管）：
+--------+ +--------+
|  VM 1  | |  VM 2  |
+--------+ +--------+
|    Hypervisor      |
|    主機作業系統    |
|      硬體          |
+--------------------+
```

## CPU 虛擬化

### 完全虛擬化

軟體模擬所有硬體指令，不需修改 Guest OS。

### 硬體輔助虛擬化

Intel VT-x 和 AMD-V 提供硬體支援：

```c
// VMX 指令流程
VMXON         // 進入 VMX 模式
VMLAUNCH      // 啟動 VM
// VM 執行中...
VMEXIT        // VM 退出（中斷、系統呼叫等）
// Hypervisor 處理
VMRESUME      // 恢復 VM
VMXOFF        // 離開 VMX 模式
```

## 記憶體虛擬化

### 影子頁表（Shadow Page Table）

Hypervisor 維護 Guest 虛擬位址 → 實體位址的對映。

### 第二層位址轉譯（SLAT）

Intel EPT（Extended Page Tables）或 AMD NPT 直接在硬體中處理兩層轉譯：

```
Guest VA → Guest PA（Guest 頁表）
                ↓
Guest PA → Host PA（EPT 頁表）
```

## 容器技術

### 容器 vs 虛擬機器

| 特性 | 虛擬機器 | 容器 |
|-----|---------|------|
| 啟動時間 | 分鐘級 | 秒級 |
| 映像大小 | GB 級 | MB 級 |
| 隔離程度 | 完全隔離 | 行程層級 |
| 核心 | 各自核心 | 共用主機核心 |
| 效能損耗 | 5-15% | 接近原生 |

### Linux 容器核心技術

#### Namespace

隔離行程的視野：

```c
// 可隔離的 namespace
CLONE_NEWPID    // PID 隔離
CLONE_NEWNET   // 網路隔離
CLONE_NEWNS    // 掛載隔離
CLONE_NEWUTS   // hostname 隔離
CLONE_NEWIPC   // IPC 隔離
CLONE_NEWUSER  // 使用者隔離

// 使用 unshare 建立新 namespace
#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>

int main() {
    // 建立新的 PID 和掛載 namespace
    unshare(CLONE_NEWPID | CLONE_NEWNS);

    // 現在這個行程在新 namespace 中
    // 它的 PID 會是 1
    return 0;
}
```

#### Cgroup

限制與監控資源使用：

```c
// 使用 cgroup v2 限制記憶體
// echo "100M" > /sys/fs/cgroup/mygroup/memory.max
// echo $$ > /sys/fs/cgroup/mygroup/cgroup.procs
```

### 建立簡易容器

```c
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mount.h>

#define STACK_SIZE (1024 * 1024)

int container_main(void *arg) {
    char **args = (char **)arg;

    // 設定 hostname
    sethostname("container", 9);

    // 掛載 proc
    mount("proc", "/proc", "proc", 0, NULL);

    // 執行命令
    execvp(args[0], args);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command>\n", argv[0]);
        return 1;
    }

    char *stack = malloc(STACK_SIZE);

    pid_t pid = clone(
        container_main,
        stack + STACK_SIZE,
        CLONE_NEWPID | CLONE_NEWNS |
        CLONE_NEWUTS | SIGCHLD,
        argv + 1
    );

    waitpid(pid, NULL, 0);
    free(stack);
    return 0;
}
```

## Docker 架構

```
+----------------------------------+
|           Container              |
|  +--------+  +--------+         |
|  | App 1  |  | App 2  |         |
|  +--------+  +--------+         |
|  |   Libraries/Dependencies     |
|  +----------------------------+ |
+----------------------------------+
|         Container Engine         |
|         (Docker daemon)          |
+----------------------------------+
|          Host OS                 |
+----------------------------------+
|          Hardware                |
+----------------------------------+
```

## 常見操作

```bash
# Docker
docker run -it ubuntu:22.04 /bin/bash
docker ps
docker images
docker exec -it container_id bash

# Podman（無 daemon 替代方案）
podman run -it alpine:latest sh

# 查看 cgroup
ls /sys/fs/cgroup/
systemd-cgtop
```

---

**上一章**：[第十二章：網路程式設計](12-networking.md)
**下一章**：[第十四章：效能優化與分析](14-performance.md)
