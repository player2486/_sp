# 第二章：電腦硬體架構

## CPU 架構

### 指令集架構（ISA）

指令集架構是處理器與軟體之間的介面合約。主要的 ISA 包括：

- **x86/x86-64**：Intel 與 AMD 使用的複雜指令集（CISC）
- **ARM/AArch64**：廣泛用於行動裝置與伺服器（RISC）
- **RISC-V**：開放原始碼的精簡指令集

### CPU 內部元件

```
+------------------+
|      CPU         |
|  +------------+  |
|  |   控制單元  |  |
|  +------------+  |
|  +------------+  |
|  |   算術邏輯  |  |
|  |   單元 ALU  |  |
|  +------------+  |
|  +------------+  |
|  |   暫存器    |  |
|  |  檔案      |  |
|  +------------+  |
|  +------------+  |
|  |   快取      |  |
|  |   L1/L2/L3 |  |
|  +------------+  |
+------------------+
```

### 管線化（Pipelining）

現代 CPU 將指令執行分為多個階段：

1. **IF**：指令擷取（Instruction Fetch）
2. **ID**：指令解碼（Instruction Decode）
3. **EX**：執行（Execute）
4. **MEM**：記憶體存取（Memory Access）
5. **WB**：寫回（Write Back）

## 記憶體層次結構

```
CPU 暫存器   (1 cycle)     很小   ~1KB
    ↓
L1 快取      (~3 cycles)    ~32KB
    ↓
L2 快取      (~10 cycles)   ~256KB
    ↓
L3 快取      (~40 cycles)   ~幾MB
    ↓
主記憶體     (~100 cycles)  ~幾GB
    ↓
SSD          (~10μs)       ~幾百GB
    ↓
硬碟         (~10ms)       ~幾TB
```

### 快取一致性

在多核心系統中，每個核心有自己的 L1/L2 快取。快取一致性協定（如 MESI）確保所有核心看到相同的資料。

## 中斷機制

中斷是硬體向 CPU 發出事件通知的機制：

- **外部中斷**：來自 I/O 裝置
- **內部中斷（例外）**：除零、頁面錯誤等
- **軟體中斷**：由指令觸發（如 `int 0x80`、`syscall`）

中斷處理流程：

1. CPU 完成當前指令
2. 保存上下文（程式計數器、狀態暫存器）
3. 查詢中斷向量表
4. 跳至中斷處理常式
5. 處理完成後恢復上下文

## DMA（直接記憶體存取）

DMA 允許 I/O 裝置直接與記憶體傳輸資料，無需 CPU 介入，大幅提升效能。

```
傳統方式：
  CPU → 從裝置讀取 → 寫入記憶體 → 重複

DMA 方式：
  CPU → 設定 DMA 控制器 → DMA 直接傳輸 → 中斷通知 CPU
```

## 範例：x86 組合語言

```assembly
section .data
    msg db 'Hello World', 0xa
    len equ $ - msg

section .text
    global _start

_start:
    mov rax, 1      ; sys_write
    mov rdi, 1      ; stdout
    mov rsi, msg    ; 緩衝區
    mov rdx, len    ; 長度
    syscall

    mov rax, 60     ; sys_exit
    xor rdi, rdi    ; exit code 0
    syscall
```

---

**上一章**：[第一章：系統程式概論](01-introduction.md)  
**下一章**：[第三章：作業系統基礎](03-os-basics.md)
