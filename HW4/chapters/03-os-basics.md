# 第三章：作業系統基礎

## 作業系統的角色

作業系統是管理電腦硬體與軟體資源的系統軟體，提供：

1. **資源管理**：CPU、記憶體、I/O 裝置
2. **抽象化**：隱藏硬體細節，提供一致的介面
3. **保護機制**：隔離不同程式，防止互相干擾
4. **使用者介面**：Shell、GUI 等

## 核心態 vs 使用者態

現代 CPU 支援至少兩個特權層級：

| | 使用者態 | 核心態 |
|---|---------|--------|
| 特權指令 | 不能執行 | 可執行 |
| 記憶體存取 | 受限 | 完全存取 |
| I/O 存取 | 不能直接存取 | 可直接存取 |
| 切換方式 | 系統呼叫 / 中斷 | 硬體機制 |

### 系統呼叫流程

```
使用者程式
    ↓
函式庫呼叫（libc）
    ↓
trap/syscall 指令 → 切換至核心態
    ↓
核心處理系統呼叫
    ↓
返回使用者態
    ↓
繼續執行使用者程式
```

## 作業系統類型

### 批次處理系統

早期系統，一次執行一個工作，沒有互動。

### 分時系統

透過時間分片（time slicing）讓多個使用者同時使用系統。

### 即時系統

保證在特定時間內完成處理：
- **硬即時**：錯過截止時間 = 系統失敗（飛行控制）
- **軟即時**：錯過截止時間 = 效能下降（影音串流）

### 嵌入式系統

資源受限的專用系統（IoT、微控制器）。

### 分散式系統

多台電腦協同工作，對使用者呈現單一系統。

## 作業系統元件

```
+------------------------------------------+
|            使用者程式                       |
+------------------------------------------+
|   Shell    |    GUI    |   工具程式        |
+------------------------------------------+
|           系統呼叫介面                      |
+------------------------------------------+
|  行程管理  |  記憶體管理 |  檔案系統        |
|  中斷處理  |  裝置驅動程式 |  網路協定       |
+------------------------------------------+
|              硬體層                        |
+------------------------------------------+
```

## 範例：Linux 核心模組

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init hello_init(void)
{
    printk(KERN_INFO "HW4: Hello from kernel module!\n");
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "HW4: Goodbye from kernel module!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HW4 Example Kernel Module");
```

編譯與載入：
```bash
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
sudo insmod hello.ko
sudo rmmod hello.ko
dmesg | tail
```

---

**上一章**：[第二章：電腦硬體架構](02-hardware.md)  
**下一章**：[第四章：行程管理](04-process-management.md)
