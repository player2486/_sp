# 第十一章：系統呼叫

## 系統呼叫簡介

系統呼叫是使用者程式請求核心服務的介面。

### 為什麼需要系統呼叫？

- **保護**：使用者程式不能直接存取硬體
- **抽象化**：提供統一的裝置存取介面
- **安全性**：核心可以驗證每個請求

## 系統呼叫 vs 函式庫呼叫

| | 系統呼叫 | 函式庫呼叫 |
|---|---------|-----------|
| 執行空間 | 使用者態 → 核心態 | 使用者態 |
| 開銷 | 高（上下文切換） | 低 |
| 依賴性 | 依賴作業系統 | 跨平台 |
| 範例 | read(), write() | printf(), strlen() |

## Linux 系統呼叫

### 常用系統呼叫

| 系統呼叫 | 用途 | 傳回值 |
|---------|------|-------|
| `read(fd, buf, count)` | 從檔案讀取 | 讀取位元組數 |
| `write(fd, buf, count)` | 寫入檔案 | 寫入位元組數 |
| `open(path, flags, mode)` | 開啟檔案 | 檔案描述子 |
| `close(fd)` | 關閉檔案 | 0 或 -1 |
| `fork()` | 建立子行程 | 子 PID |
| `execve(path, argv, envp)` | 執行程式 | 不返回 |
| `mmap(addr, len, prot, flags, fd, off)` | 記憶體對映 | 對映位址 |
| `sbrk(increment)` | 調整堆積 | 舊的程式斷點 |

### 系統呼叫編號（x86-64）

```c
// /usr/include/asm/unistd_64.h
#define __NR_read          0
#define __NR_write         1
#define __NR_open          2
#define __NR_close         3
#define __NR_stat          4
#define __NR_fstat         5
#define __NR_mmap          9
#define __NR_mprotect     10
#define __NR_brk          12
#define __NR_sigaction    13
#define __NR_exit         60
#define __NR_fork         57
#define __NR_execve       59
```

## 系統呼叫實現細節

### x86-64 系統呼叫約定

- `rax`：系統呼叫編號
- `rdi`：第一個參數
- `rsi`：第二個參數
- `rdx`：第三個參數
- `r10`：第四個參數
- `r8`：第五個參數
- `r9`：第六個參數
- `syscall` 指令觸發切換

### 直接使用系統呼叫

```c
#include <unistd.h>

// 直接使用 syscall() 函式
long syscall(long number, ...);

// 範例：直接呼叫 getpid
#include <sys/syscall.h>

pid_t getpid_direct() {
    return syscall(SYS_getpid);
}
```

### 組合語言版本

```assembly
section .data
    msg db "Hello from syscall!", 10

section .text
    global _start

_start:
    ; sys_write(1, msg, 20)
    mov rax, 1      ; 系統呼叫編號
    mov rdi, 1      ; stdout
    lea rsi, [msg]  ; 緩衝區
    mov rdx, 20     ; 長度
    syscall

    ; sys_exit(0)
    mov rax, 60
    xor rdi, rdi
    syscall
```

## 系統呼叫的開銷

系統呼叫比普通函式呼叫慢得多：

```
普通函式呼叫：~1-5 ns
系統呼叫：    ~100-500 ns（主要是上下文切換）
```

### 減少系統呼叫的技巧

```c
// 不好的方式：每次寫入一個字元
for (int i = 0; str[i]; i++)
    write(fd, &str[i], 1);  // N 次系統呼叫

// 好的方式：一次寫入整個字串
write(fd, str, strlen(str));  // 1 次系統呼叫
```

## strace：追蹤系統呼叫

```bash
# 追蹤 ls 的系統呼叫
strace ls

# 只追蹤特定類別
strace -e trace=open,read,write ls

# 統計系統呼叫次數
strace -c ls

# 追蹤正在執行的程式
strace -p PID
```

## 系統呼叫安全

### 驗證檢查

核心在處理系統呼叫時會檢查：
1. **參數有效性**：指標是否可存取
2. **權限檢查**：是否有足夠權限
3. **資源限制**：是否超過限制

```c
// 核心中的 copy_from_user 會檢查使用者空間指標
// 並安全地複製資料
if (copy_from_user(kernel_buf, user_buf, size)) {
    return -EFAULT;  // 錯誤位址
}
```

---

**上一章**：[第十章：組譯器與連結器](10-assembler-linker.md)
**下一章**：[第十二章：網路程式設計](12-networking.md)
