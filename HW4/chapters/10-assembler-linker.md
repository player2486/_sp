# 第十章：組譯器與連結器

## 組譯器（Assembler）

將組合語言轉換為機器碼。

### 組譯器的工作

1. 將助憶碼轉換為 opcode
2. 將標籤轉換為位址
3. 處理虛擬指令（pseudo-instruction）

### 兩遍組譯（Two-Pass Assembly）

**第一遍**：建立符號表（記錄所有標籤的位址）

```
位址   原始碼
0      LOOP:  mov rax, 0
3             cmp rax, 10
7             je DONE
10            inc rax
13            jmp LOOP
16     DONE:  ret

符號表：
LOOP → 0
DONE → 16
```

**第二遍**：使用符號表產生機器碼

```c
// 簡化版組譯器邏輯
typedef struct {
    char *name;
    int address;
} Symbol;

Symbol symbol_table[256];
int symbol_count = 0;

// 第一遍：收集符號
int current_address = 0;
for each instruction {
    if (has_label(instruction)) {
        symbol_table[symbol_count++] = {
            label_name(instruction), current_address
        };
    }
    current_address += instruction_size(instruction);
}

// 第二遍：產生機器碼
for each instruction {
    if (uses_label(instruction)) {
        int target = lookup_symbol(label_name(instruction));
        // 產生跳躍指令，目標位址為 target
    } else {
        // 產生一般指令
    }
}
```

## 目的檔格式

### ELF（Executable and Linkable Format）

```
+------------------+
| ELF Header       |
+------------------+
| Section Headers  |
| .text            | ← 程式碼
| .data            | ← 已初始化資料
| .bss             | ← 未初始化資料
| .rodata          | ← 唯讀資料
| .symtab          | ← 符號表
| .rel.text        | ← 重定位資訊
| .strtab          | ← 字串表
+------------------+
```

### ELF Header

```c
#define EI_NIDENT 16

typedef struct {
    unsigned char e_ident[EI_NIDENT];  // 魔數、類別、編碼
    uint16_t      e_type;              // 物件檔類型
    uint16_t      e_machine;           // 架構
    uint32_t      e_version;           // 版本
    uint64_t      e_entry;             // 進入點位址
    uint64_t      e_phoff;             // Program header 偏移
    uint64_t      e_shoff;             // Section header 偏移
    uint32_t      e_flags;             // 旗標
    uint16_t      e_ehsize;            // ELF header 大小
    uint16_t      e_phentsize;         // Program header 大小
    uint16_t      e_phnum;             // Program header 數量
    uint16_t      e_shentsize;         // Section header 大小
    uint16_t      e_shnum;             // Section header 數量
    uint16_t      e_shstrndx;          // 字串表索引
} Elf64_Ehdr;
```

## 連結器（Linker）

將多個目的檔合併為單一可執行檔。

### 連結過程

```
main.o   → +---+       +----------------+
utils.o  → | L |  →    |   可執行檔     |
math.o   → | I |  →    |  main          |
           | N |       |  utils         |
           | K |       |  math          |
           | E |       |  函式庫         |
libc.a   → | R |       +----------------+
           +---+
```

### 符號解析（Symbol Resolution）

```c
// main.c
extern int utils_func();  // 外部符號
int global_var = 42;       // 全域符號

int main() {
    return utils_func();
}

// utils.c
int utils_func() {         // 定義
    return 0;
}
```

連結器會將 `main.o` 中的 `utils_func` 引用連結到 `utils.o` 中的定義。

### 重定位（Relocation）

```c
// 重定位表項
typedef struct {
    uint64_t r_offset;  // 需要修改的位置
    uint64_t r_info;    // 符號索引 + 重定位類型
    int64_t  r_addend;  // 加數
} Elf64_Rela;

// 重定位類型範例（x86-64）
#define R_X86_64_PC32   2   // PC 相對定址
#define R_X86_64_PLT32  4   // PLT 相對定址
```

### 動態連結

```c
// 使用 dlopen 動態載入共用函式庫
#include <dlfcn.h>

int main() {
    void *handle = dlopen("./mylib.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error: %s\n", dlerror());
        return 1;
    }

    // 取得函式指標
    void (*func)() = dlsym(handle, "my_function");
    if (func) {
        func();
    }

    dlclose(handle);
    return 0;
}
```

編譯：
```bash
gcc -shared -fPIC -o mylib.so mylib.c
gcc -o main main.c -ldl
```

---

**上一章**：[第九章：編譯器概論](09-compiler.md)
**下一章**：[第十一章：系統呼叫](11-system-calls.md)
