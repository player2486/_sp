# 第九章：編譯器概論

## 編譯器的工作

編譯器將高階語言（C、C++、Rust 等）轉換為機器碼。

```
原始碼 → 詞法分析 → 語法分析 → 語意分析 →
  中間碼產生 → 最佳化 → 目的碼產生 → 目的碼
```

## 詞法分析（Lexical Analysis）

將原始碼分割為記號（Token）：

```c
// 原始碼：int x = 42;
// 記號序列：
// KEYWORD(int) IDENTIFIER(x) OPERATOR(=) NUMBER(42) SEMICOLON

typedef enum {
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_OPERATOR,
    TOKEN_SEMICOLON,
    // ...
} TokenType;

typedef struct {
    TokenType type;
    char *lexeme;
    int line;
} Token;
```

## 語法分析（Syntax Analysis）

將記號序列轉換為抽象語法樹（AST）：

```
         =
       /   \
      x     42
```

```c
typedef struct ast_node {
    NodeType type;
    struct ast_node *left;
    struct ast_node *right;
    char *value;
} ASTNode;

// 範例：建立二元運算子節點
ASTNode *create_binary_op(NodeType type,
                          ASTNode *left, ASTNode *right) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = type;
    node->left = left;
    node->right = right;
    return node;
}
```

## 語意分析（Semantic Analysis）

檢查程式的語意正確性：
- 型別檢查
- 變數是否宣告
- 作用域規則
- 函式參數數量與型別

## 中間碼產生（IR Generation）

將 AST 轉換為與機器無關的中間表示：

```c
// 三地址碼（Three-Address Code）
t1 = 42
x = t1

// 靜態單賦值形式（SSA）
x1 = 42
```

## 最佳化（Optimization）

### 常數摺疊（Constant Folding）

```c
// 最佳化前
int x = 42 * 3 + 1;

// 最佳化後
int x = 127;
```

### 死碼刪除（Dead Code Elimination）

```c
// 最佳化前
int x = 42;
return 0;
// x 從未被使用，可刪除

// 最佳化後
return 0;
```

### 內聯展開（Inline Expansion）

```c
// 最佳化前
int add(int a, int b) { return a + b; }
int x = add(3, 4);

// 最佳化後
int x = 3 + 4;
```

## 目的碼產生（Code Generation）

將 IR 轉換為目標機器的組合語言或機器碼：

```c
// x86-64 組合語言輸出範例
// x = 42;
mov $42, x(%rip)

// y = x + 10
mov x(%rip), %rax
add $10, %rax
mov %rax, y(%rip)
```

## 練習：簡易直譯器

```python
def eval_ast(node, env):
    if node.type == "NUMBER":
        return node.value
    elif node.type == "IDENTIFIER":
        return env[node.value]
    elif node.type == "BINARY_OP":
        left = eval_ast(node.left, env)
        right = eval_ast(node.right, env)
        if node.op == "+": return left + right
        if node.op == "-": return left - right
        if node.op == "*": return left * right
        if node.op == "/": return left / right
    elif node.type == "ASSIGN":
        value = eval_ast(node.right, env)
        env[node.left.value] = value
        return value
```

---

**上一章**：[第八章：並行與同步](08-concurrency.md)
**下一章**：[第十章：組譯器與連結器](10-assembler-linker.md)
