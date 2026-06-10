# HW1：p0 語言編譯器

**本作業使用 AI 輔助**

- **使用 AI**: Claude (opencode)
- **使用方式**: 由 AI 協助產生程式碼架構，本人審閱並修改每一行程式碼
- **參考來源**: 無直接複製他人程式碼，參考 c4/c5 編譯器設計概念
- **原創性說明**: 本專案為原創作品，非複製他人程式碼。四種語言實作（C/Rust/Python/JS）皆為獨立完成。

# Hp0 Compiler - p0 語言編譯器

## 專案結構

```
Hp0/
├── compiler.c      # C 語言版編譯器 + 虛擬機
├── compiler.rs     # Rust 語言版編譯器 + 虛擬機
├── compiler.py     # Python 版編譯器 + 虛擬機
├── compiler.js     # JavaScript 版編譯器 + 虛擬機
├── test.sh         # 測試腳本
├── p0/             # 測試程式
│   ├── while.p0    # while 迴圈測試
│   ├── for.p0      # for 迴圈測試
│   ├── if.p0       # if-else 測試
│   ├── fact.p0     # 遞迴階乘測試
│   └── ...
└── doc/            # 文件
    ├── ebnf.md     # EBNF 語法
    ├── call.md     # 函數呼叫機制說明
    └── 區域變數的尋找.md
```

## 執行方式

```bash
# C 版本
gcc compiler.c -o compiler
./compiler p0/while.p0

# Rust 版本
rustc compiler.rs -o compiler_rs
./compiler_rs p0/while.p0
```

---

## 一、while 語法的處理 — 設計原理

### EBNF 定義

```ebnf
statement ::= "while" "(" expression ")" "{" statement* "}"
            | "if" "(" expression ")" "{" statement* "}" ...
            | ...
```

### 編譯器處理流程 (compiler.c:783-817)

while 迴圈的編譯產生以下四元組結構：

```
        ┌─────────────────────────────────────┐
        │                                     │
cond_idx:  <計算條件式>                         │
        │  JMP_F cond, ?, end_idx  ────┐      │
        │                             │      │
        │  <迴圈本體 statement* >       │      │
        │                             │      │
        │  JMP "", "", cond_idx  ──────┘      │
        │                                     │
end_idx:  <後續程式>                            │
        └─────────────────────────────────────┘
```

#### 步驟分解：

1. **記錄條件位置** (`cond_idx = quad_count`)
   - 在解析條件式之前記錄當前四元組編號，作為迴圈跳回點

2. **解析條件式** (`expression()`)
   - 產生計算條件的四元組，結果存入暫存變數

3. **發出條件失敗跳轉** (`emit("JMP_F", cond, "", "?")`)
   - 條件為 false 時跳出迴圈，目標地址先用 `?` 暫代（back-patching）

4. **推入迴圈上下文** (`loop_stack`)
   - 記錄 `continue_target = cond_idx`（continue 跳轉目標）
   - 初始化 `break_list`（記錄 break 指令位置）

5. **解析迴圈本體** (`statement()` 迴圈)
   - 可能包含 break/continue/if/nested while 等

6. **發出迴圈跳轉** (`emit("JMP", "", "", cond_idx_str)`)
   - 執行完本體後無條件跳回條件檢查處

7. **Back-patching**
   - 將 `JMP_F` 的目標填為 `end_idx`（迴圈結束位置）
   - 將所有 `break` 指令的目標填為 `end_idx`

### 對 break/continue 的支援

```c
// compiler.c:796-798
loop_stack[loop_stack_count].break_count = 0;
loop_stack[loop_stack_count].continue_target = cond_idx;
loop_stack_count++;
```

- **continue**: 發出 `JMP` 到 `cond_idx`（繼續檢查條件）
- **break**: 記錄 `JMP` 指令位置到 `break_list`，在迴圈結束時 back-patch 到 `end_idx`

### Rust 版本對應 (compiler.rs:523-541)

邏輯完全相同，使用 Rust 的 `Vec<LoopCtx>` 取代 C 的陣列：

```rust
self.loop_stack.push(LoopCtx { break_list: vec![], continue_idx: cond_idx });
// ... statement parsing ...
if let Some(ctx) = self.loop_stack.pop() {
    for b_idx in ctx.break_list {
        self.quads[b_idx].result = end_idx.to_string();
    }
}
```

---

## 二、函數呼叫機制

### 整體架構

函數呼叫使用 **呼叫堆疊 (Call Stack)** + **堆疊幀 (Stack Frame)** 機制，類似真實 CPU 的運作方式：

```
param_stack:  [arg1, arg2, ...]   ← 參數暫存區

call_stack:   sp=0: [全域變數]
              sp=1: [funcA 區域變數]  ← 當前作用域
              sp=2: [funcB 區域變數]
```

### 編譯階段的四元組生成

以 `sum = add(a, b)` 為例：

```
PARAM    a          →  把 a 放入 param_stack
PARAM    b          →  把 b 放入 param_stack
CALL     add, 2, t4 →  呼叫 add (2個參數)，結果存入 t4
```

函數定義：
```
FUNC_BEG add
FORMAL   a          →  從 incoming_args 取出參數，建立區域變數 a
FORMAL   b          →  從 incoming_args 取出參數，建立區域變數 b
... 函數本體 ...
RET_VAL  result     →  回傳 result 給呼叫者
FUNC_END
```

### VM 執行階段的機制

#### A. CALL 指令 (compiler.c:1357-1406)

```
1. 搜尋 func_map 找到函數起始 PC
2. sp++                    // 建立新堆疊幀
3. 設定 ret_pc = pc + 1   // 儲存返回位址
4. 拷貝參數到 incoming_args
5. pc = target_pc          // 跳轉到函數
```

#### B. FORMAL 指令 (compiler.c:1407-1410)

```
set_var(name, incoming_args[formal_idx++])
// 從參數列取出值，在當前幀建立區域變數
```

#### C. RET_VAL 指令 (compiler.c:1411-1421)

```
1. 取出回傳值 ret_val = get_var(arg1)
2. 取出返回位址 ret_pc
3. sp--                    // 銷毀當前堆疊幀，回到呼叫者
4. set_var(ret_var, ret_val)  // 將回傳值寫入呼叫者的目標變數
5. pc = ret_pc             // 跳回呼叫點繼續執行
```

### D. FUNC_END 指令 (compiler.c:1422-1433)

當函數沒有 return 而自然結束時，回傳 0 並返回。

### 區域變數的尋找 (compiler.c:960-993)

```c
Value* get_var(const char* name) {
    // 只搜尋 stack[sp]（當前幀），不跨層尋找
    Frame* frame = &call_stack[sp];
    for (int i = 0; i < frame->var_count; i++) {
        if (strcmp(frame->var_names[i], name) == 0)
            return frame->vars[i];
    }
    return create_int(0);  // 找不到回傳 0
}
```

這造就了「純局部作用域」：
- 每個函數有自己的變數空間
- 函數內看不到全域變數（除非手動加入作用域鏈查找）
- 遞迴呼叫時每層的變數完全隔離

### 遞迴支援的原理

以 `fact(3)` 為例，遞迴呼叫 `fact(2)` → `fact(1)`：

```
sp=1: [n=3]          ← fact(3)
sp=2: [n=2]          ← fact(2)
sp=3: [n=1]          ← fact(1) → 回傳 1, sp--
sp=2: [n=2]          ← 恢復 fact(2)，n 仍然是 2
     ...
sp=0: [result=6]     ← 最終結果
```

每次 `CALL` 都 `sp++` 建立新幀，每次 `RET_VAL` 都 `sp--` 銷毀當前幀。這是遞迴能正確運作的核心。

### Rust 版本的對應

Rust 版使用 `Vec<Frame>` 做為呼叫堆疊，`push`/`pop` 代替 C 的 `sp++`/`sp--`：

```rust
// CALL
self.stack.push(Frame { vars: HashMap::new(), ret_pc: pc + 1, ... });
pc = target_pc;

// RET_VAL
let frame = self.stack.pop().unwrap();
self.set_var(&frame.ret_var, ret_val);
pc = frame.ret_pc;
```
