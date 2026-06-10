# 第五章：記憶體管理

## 實體記憶體 vs 虛擬記憶體

### 實體記憶體

實際安裝在電腦中的 RAM，是有限的資源。

### 虛擬記憶體

提供每個行程獨立的位址空間，讓行程認為自己擁有完整的記憶體。

```
虛擬位址空間（行程 A）：
+------------------+ 0xFFFFFFFF
|     核心空間      |
+------------------+ 0xC0000000（典型分割）
|     堆疊          |
|        ↓          |
|        ↑          |
|     堆積          |
|     資料段        |
|     程式碼段      |
+------------------+ 0x00000000
```

## 分頁（Paging）

虛擬記憶體的核心機制是分頁：

- **頁面（Page）**：虛擬記憶體的最小單位（典型 4KB）
- **頁框（Frame）**：實體記憶體的最小單位（與頁面等大小）
- **頁表（Page Table）**：虛擬位址到實體位址的對映

```
虛擬位址：
+--------+-----------+
| 頁號   | 偏移量     |
+--------+-----------+

透過頁表轉換：
頁表[頁號] → 頁框號碼

實體位址：
+--------+-----------+
| 頁框號 | 偏移量     |
+--------+-----------+
```

### 多層分頁

現代架構使用多層分頁減少頁表大小：

```c
// x86-64 4 層分頁
// VPN[0..3] 分別索引 PGD → PUD → PMD → PTE
```

## 頁面置換演算法

當記憶體不足時，需要將部分頁面換出到磁碟：

- **FIFO**：先換入的先換出
- **LRU**（Least Recently Used）：換出最久未使用的
- **時鐘演算法**：近似 LRU，效率較高
- **最佳演算法**：換出未來最久才用到的（理論上，無法實現）

## 記憶體對映（mmap）

```c
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    int fd = open("test.txt", O_RDWR);
    struct stat sb;

    fstat(fd, &sb);
    off_t size = sb.st_size;

    // 將檔案對映到記憶體
    char *addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, 0);

    if (addr == MAP_FAILED) {
        perror("mmap 失敗");
        return 1;
    }

    printf("檔案內容：%s\n", addr);

    // 修改記憶體內容（同步到檔案）
    addr[0] = 'H';
    msync(addr, size, MS_SYNC);
    munmap(addr, size);
    close(fd);

    return 0;
}
```

## 記憶體階層與效能

### 參考局部性（Locality of Reference）

- **空間局部性**：存取某個位址後，附近位址也可能被存取
- **時間局部性**：存取某個位址後，同一位址可能再次被存取

### 快取友善程式碼

```c
// 不好的方式（逐行存取，無局部性）
for (int j = 0; j < COLS; j++)
    for (int i = 0; i < ROWS; i++)
        sum += matrix[i][j];  // 跳躍存取

// 好的方式（逐列存取，空間局部性）
for (int i = 0; i < ROWS; i++)
    for (int j = 0; j < COLS; j++)
        sum += matrix[i][j];  // 連續存取
```

## 範例：分析記憶體使用

```bash
# 查看行程記憶體對映
cat /proc/self/maps

# 查看系統記憶體使用
free -h
vmstat

# 使用 Valgrind 偵測記憶體洩漏
valgrind --leak-check=full ./my_program
```

---

**上一章**：[第四章：行程管理](04-process-management.md)  
**下一章**：[第六章：檔案系統](06-file-systems.md)
