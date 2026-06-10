# m0 語言編譯器與虛擬機

一個用 Python 實現的簡易程式語言，包含編譯器（編譯為 bytecode）和 Stack-based VM。

## 語言規格

| 項目 | 說明 |
|------|------|
| **型態系統** | 動態型態 (Dynamic Typing) |
| **執行方式** | 編譯為 bytecode 後由 VM 直譯執行 |
| **目標架構** | Stack-based 虛擬機 |
| **垃圾蒐集** | 參考計數（簡化版，VM 自動清理 frame） |

## EBNF 語法

```
program       ::= { statement | function_def }
function_def  ::= "func" IDENT "(" [ param_list ] ")" block
param_list    ::= IDENT { "," IDENT }

statement     ::= print_stmt | if_stmt | while_stmt | for_stmt
                | return_stmt | break_stmt | continue_stmt
                | assignment ";" | expr ";"

print_stmt    ::= "print" "(" [ expr { "," expr } ] ")" ";"
if_stmt       ::= "if" "(" expr ")" block [ "else" (if_stmt | block) ]
while_stmt    ::= "while" "(" expr ")" block
for_stmt      ::= "for" "(" [ assignment ] ";" expr ";" [ assignment ] ")" block
return_stmt   ::= "return" [ expr ] ";"
break_stmt    ::= "break" ";"
continue_stmt ::= "continue" ";"
block         ::= "{" { statement } "}"

assignment    ::= IDENT [ "[" expr "]" | "." IDENT ] "=" expr

expr          ::= or_expr
or_expr       ::= and_expr { "or" and_expr }
and_expr      ::= equality_expr { "and" equality_expr }
equality_expr ::= comparison_expr { ("==" | "!=") comparison_expr }
comparison_expr ::= add_expr { ("<" | ">" | "<=" | ">=") add_expr }
add_expr      ::= mul_expr { ("+" | "-") mul_expr }
mul_expr      ::= unary_expr { ("*" | "/" | "%") unary_expr }
unary_expr    ::= [ "-" | "not" ] postfix_expr
postfix_expr  ::= primary { "(" [ arg_list ] ")" | "[" expr "]" | "." IDENT }

primary       ::= NUMBER | STRING | "true" | "false" | "nil"
                | IDENT
                | "[" [ expr { "," expr } ] "]"
                | "{" [ string_key_pair { "," string_key_pair } ] "}"
                | "(" expr ")"

string_key_pair ::= (IDENT | STRING) ":" expr
arg_list      ::= expr { "," expr }
```

## 內建函式

| 函式 | 說明 | 範例 |
|------|------|------|
| `print(...)` | 印出值 | `print("Hello", x)` |
| `len(x)` | 取得長度 | `len([1,2,3])` → 3 |
| `type(x)` | 取得型態名稱 | `type(42)` → "int" |
| `int(x)` | 轉整數 | `int("42")` → 42 |
| `float(x)` | 轉浮點數 | `float("3.14")` → 3.14 |
| `str(x)` | 轉字串 | `str(123)` → "123" |
| `abs(x)` | 絕對值 | `abs(-5)` → 5 |
| `sqrt(x)` | 平方根 | `sqrt(16)` → 4.0 |
| `floor(x)` | 無條件捨去 | `floor(3.7)` → 3 |
| `ceil(x)` | 無條件進位 | `ceil(3.2)` → 4 |
| `input(prompt)` | 讀取輸入 | `input("Enter: ")` |
| `rand()` / `rand(n)` / `rand(a,b)` | 亂數 | `rand(1, 10)` |

## 使用方式

```bash
# 執行程式
python m0.py <source.m0>

# 顯示 bytecode 除錯
python m0.py <source.m0> --dump
```

## 範例程式

### Hello World
```
print("Hello, m0 World!");
```

### 函式與遞迴 (Fibonacci)
```
func fib(n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

for (i = 0; i < 15; i = i + 1) {
    print("fib(", i, ") = ", fib(i));
}
```

### 陣列與字典
```
arr = [10, 20, 30, 40, 50];
print("arr[0] = ", arr[0]);
print("len = ", len(arr));

person = {name: "Alice", age: 25};
print("name: ", person.name);
person.age = 26;
print("new age: ", person.age);
```

### FizzBuzz
```
for (i = 1; i <= 30; i = i + 1) {
    if (i % 15 == 0) {
        print("FizzBuzz");
    } else if (i % 3 == 0) {
        print("Fizz");
    } else if (i % 5 == 0) {
        print("Buzz");
    } else {
        print(i);
    }
}
```

## 專案結構

```
m0/
├── m0.py              # 編譯器 + VM 原始碼
├── examples/          # 範例程式
│   ├── 01_hello.m0
│   ├── 02_arith.m0
│   ├── 03_if.m0
│   ├── 04_while.m0
│   ├── 05_for.m0
│   ├── 06_func.m0     # 遞迴函式
│   ├── 07_array.m0
│   ├── 08_dict.m0
│   ├── 09_logic.m0
│   ├── 10_fizzbuzz.m0
│   ├── 11_sort.m0     # 泡沫排序
│   ├── 12_builtins.m0
│   ├── 13_string_ops.m0
│   └── 14_prime.m0    # 質數判斷
└── README.md
```

## Bytecode 指令集

| 指令 | 說明 |
|------|------|
| `PUSH_INT n` | 推入整數/浮點數 |
| `PUSH_STR idx` | 推入字串 |
| `PUSH_TRUE/FALSE/NIL` | 推入布林/空值 |
| `LOAD name` | 載入變數 |
| `STORE name` | 儲存變數 |
| `ADD/SUB/MUL/DIV/MOD` | 算術運算 |
| `EQ/NEQ/LT/GT/LTE/GTE` | 比較運算 |
| `OP_NOT` | 邏輯非 |
| `JMP addr` | 無條件跳轉 |
| `JMP_FALSE addr` | 條件為假時跳轉 |
| `NEW_ARRAY n` | 建立陣列 |
| `NEW_DICT n` | 建立字典 |
| `GET_ITEM` | 取得元素 |
| `SET_ITEM` | 設定元素 |
| `CALL argc` | 呼叫函式 |
| `RET` | 從函式返回 |
| `PRINT n` | 印出 n 個值 |
| `FUNC_DEF` | 函式定義（跳過） |
