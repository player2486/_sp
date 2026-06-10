"""
m0 語言編譯器與虛擬機
=======================

語言規格
--------
型態系統：動態型態 (Dynamic Typing)
執行方式：編譯為 bytecodes 後由 Stack-based VM 直譯執行
垃圾蒐集：參考計數 (簡化版，VM 自動清理 frame)

EBNF 語法
---------
program       ::= { statement | function_def }
function_def  ::= "func" IDENT "(" [ param_list ] ")" block
param_list    ::= IDENT { "," IDENT }
statement     ::= print_stmt
                | if_stmt
                | while_stmt
                | for_stmt
                | return_stmt
                | break_stmt
                | continue_stmt
                | assignment ";"
                | expr ";"

print_stmt    ::= "print" "(" [ expr { "," expr } ] ")" ";"
if_stmt       ::= "if" "(" expr ")" block [ "else" (if_stmt | block) ]
while_stmt    ::= "while" "(" expr ")" block
for_stmt      ::= "for" "(" [ assignment ] ";" expr ";" [ assignment ] ")" block
return_stmt   ::= "return" [ expr ] ";"
break_stmt    ::= "break" ";"
continue_stmt ::= "continue" ";"
block         ::= "{" { statement } "}"

assignment    ::= IDENT [ "[" expr "]" ] "=" expr
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

詞彙
----
NUMBER   : [0-9]+
STRING   : "..."
IDENT    : [a-zA-Z_][a-zA-Z0-9_]*
"""

import sys
import math
from enum import Enum, auto
from typing import Any, Optional

# ============================================================
# Token Types
# ============================================================
class TokenType(Enum):
    # Keywords
    FUNC = auto(); PRINT = auto(); IF = auto(); ELSE = auto()
    WHILE = auto(); FOR = auto(); RETURN = auto()
    BREAK = auto(); CONTINUE = auto()
    TRUE = auto(); FALSE = auto(); NIL = auto()
    AND = auto(); OR = auto(); NOT = auto()
    # Literals
    IDENT = auto(); NUMBER = auto(); STRING = auto()
    # Symbols
    LPAREN = auto(); RPAREN = auto()
    LBRACE = auto(); RBRACE = auto()
    LBRACKET = auto(); RBRACKET = auto()
    DOT = auto(); COLON = auto(); COMMA = auto(); SEMICOLON = auto()
    # Operators
    ASSIGN = auto(); PLUS = auto(); MINUS = auto(); MUL = auto()
    DIV = auto(); MOD = auto(); EQ = auto(); NEQ = auto()
    LT = auto(); GT = auto(); LTE = auto(); GTE = auto()
    EOF = auto()

class Token:
    __slots__ = ('type', 'text', 'line', 'col')
    def __init__(self, t: TokenType, text: str, line: int, col: int):
        self.type = t; self.text = text; self.line = line; self.col = col
    def __repr__(self):
        return f"Token({self.type.name}, {self.text!r}, L{self.line})"

# ============================================================
# Bytecode Instructions
# ============================================================
class OpCode(Enum):
    # Stack ops
    PUSH_INT = auto(); PUSH_STR = auto()
    PUSH_TRUE = auto(); PUSH_FALSE = auto(); PUSH_NIL = auto()
    LOAD = auto(); STORE = auto()
    DUP = auto(); POP = auto()
    # Arithmetic
    ADD = auto(); SUB = auto(); MUL = auto(); DIV = auto(); MOD = auto()
    NEG = auto()
    # Compare
    EQ = auto(); NEQ = auto(); LT = auto(); GT = auto(); LTE = auto(); GTE = auto()
    # Logic
    OP_NOT = auto(); OP_AND = auto(); OP_OR = auto()
    # Control flow
    JMP = auto(); JMP_FALSE = auto()
    # Collection
    NEW_ARRAY = auto(); NEW_DICT = auto()
    GET_ITEM = auto(); SET_ITEM = auto()
    # Function
    CALL = auto(); RET = auto()
    FUNC_DEF = auto()
    # I/O
    PRINT = auto(); INPUT = auto()

# ============================================================
# Error reporting
# ============================================================
class CompileError(Exception):
    def __init__(self, msg: str, line: int, col: int):
        self.msg = msg; self.line = line; self.col = col
        super().__init__(f"L{line}:{col} {msg}")

def error(msg: str, tok: Token):
    raise CompileError(msg, tok.line, tok.col)

# ============================================================
# Lexer
# ============================================================
KEYWORDS = {
    'func': TokenType.FUNC, 'print': TokenType.PRINT, 'if': TokenType.IF,
    'else': TokenType.ELSE, 'while': TokenType.WHILE, 'for': TokenType.FOR,
    'return': TokenType.RETURN, 'break': TokenType.BREAK, 'continue': TokenType.CONTINUE,
    'true': TokenType.TRUE, 'false': TokenType.FALSE, 'nil': TokenType.NIL,
    'and': TokenType.AND, 'or': TokenType.OR, 'not': TokenType.NOT,
}

class Lexer:
    def __init__(self, src: str):
        self.src = src; self.pos = 0; self.line = 1; self.col = 1
        self.tokens = []; self.tokenize()

    def peek(self) -> Optional[str]:
        return self.src[self.pos] if self.pos < len(self.src) else None

    def advance(self) -> str:
        ch = self.src[self.pos]; self.pos += 1
        if ch == '\n': self.line += 1; self.col = 1
        else: self.col += 1
        return ch

    def skip(self):
        while self.pos < len(self.src):
            ch = self.src[self.pos]
            if ch in ' \t\r': self.advance()
            elif ch == '\n': self.advance()
            elif ch == '/' and self.pos + 1 < len(self.src):
                if self.src[self.pos + 1] == '/':
                    while self.pos < len(self.src) and self.src[self.pos] != '\n': self.advance()
                elif self.src[self.pos + 1] == '*':
                    self.advance(); self.advance()
                    while self.pos + 1 < len(self.src):
                        if self.src[self.pos] == '*' and self.src[self.pos + 1] == '/':
                            self.advance(); self.advance(); break
                        self.advance()
                else: break
            else: break

    def tokenize(self):
        while True:
            self.skip()
            if self.pos >= len(self.src):
                self.tokens.append(Token(TokenType.EOF, '', self.line, self.col)); break

            start_line, start_col = self.line, self.col
            ch = self.peek()

            # Numbers
            if ch.isdigit():
                num = ''
                while self.pos < len(self.src) and self.src[self.pos].isdigit(): num += self.advance()
                if self.pos < len(self.src) and self.src[self.pos] == '.':
                    num += self.advance()
                    while self.pos < len(self.src) and self.src[self.pos].isdigit(): num += self.advance()
                self.tokens.append(Token(TokenType.NUMBER, num, start_line, start_col))
                continue

            # Strings
            if ch == '"':
                self.advance()
                s = ''
                while self.pos < len(self.src) and self.src[self.pos] != '"':
                    if self.src[self.pos] == '\\':
                        self.advance()
                        esc = self.advance()
                        if esc == 'n': s += '\n'
                        elif esc == 't': s += '\t'
                        elif esc == '\\': s += '\\'
                        elif esc == '"': s += '"'
                        else: s += '\\' + esc
                    else: s += self.advance()
                if self.pos >= len(self.src):
                    raise CompileError("未終止的字串", start_line, start_col)
                self.advance()
                self.tokens.append(Token(TokenType.STRING, s, start_line, start_col))
                continue

            # Identifiers / keywords
            if ch.isalpha() or ch == '_':
                ident = ''
                while self.pos < len(self.src) and (self.src[self.pos].isalnum() or self.src[self.pos] == '_'):
                    ident += self.advance()
                tt = KEYWORDS.get(ident, TokenType.IDENT)
                self.tokens.append(Token(tt, ident, start_line, start_col))
                continue

            # Symbols
            self.advance()
            if ch == '(': self.tokens.append(Token(TokenType.LPAREN, '(', start_line, start_col))
            elif ch == ')': self.tokens.append(Token(TokenType.RPAREN, ')', start_line, start_col))
            elif ch == '{': self.tokens.append(Token(TokenType.LBRACE, '{', start_line, start_col))
            elif ch == '}': self.tokens.append(Token(TokenType.RBRACE, '}', start_line, start_col))
            elif ch == '[': self.tokens.append(Token(TokenType.LBRACKET, '[', start_line, start_col))
            elif ch == ']': self.tokens.append(Token(TokenType.RBRACKET, ']', start_line, start_col))
            elif ch == '.': self.tokens.append(Token(TokenType.DOT, '.', start_line, start_col))
            elif ch == ',': self.tokens.append(Token(TokenType.COMMA, ',', start_line, start_col))
            elif ch == ';': self.tokens.append(Token(TokenType.SEMICOLON, ';', start_line, start_col))
            elif ch == ':': self.tokens.append(Token(TokenType.COLON, ':', start_line, start_col))
            elif ch == '+': self.tokens.append(Token(TokenType.PLUS, '+', start_line, start_col))
            elif ch == '-': self.tokens.append(Token(TokenType.MINUS, '-', start_line, start_col))
            elif ch == '*': self.tokens.append(Token(TokenType.MUL, '*', start_line, start_col))
            elif ch == '%': self.tokens.append(Token(TokenType.MOD, '%', start_line, start_col))
            elif ch == '/': self.tokens.append(Token(TokenType.DIV, '/', start_line, start_col))
            elif ch == '=':
                if self.pos < len(self.src) and self.src[self.pos] == '=':
                    self.advance()
                    self.tokens.append(Token(TokenType.EQ, '==', start_line, start_col))
                else: self.tokens.append(Token(TokenType.ASSIGN, '=', start_line, start_col))
            elif ch == '!':
                if self.pos < len(self.src) and self.src[self.pos] == '=':
                    self.advance()
                    self.tokens.append(Token(TokenType.NEQ, '!=', start_line, start_col))
                else: raise CompileError(f"未預期的字元 '!'", start_line, start_col)
            elif ch == '<':
                if self.pos < len(self.src) and self.src[self.pos] == '=':
                    self.advance()
                    self.tokens.append(Token(TokenType.LTE, '<=', start_line, start_col))
                else: self.tokens.append(Token(TokenType.LT, '<', start_line, start_col))
            elif ch == '>':
                if self.pos < len(self.src) and self.src[self.pos] == '=':
                    self.advance()
                    self.tokens.append(Token(TokenType.GTE, '>=', start_line, start_col))
                else: self.tokens.append(Token(TokenType.GT, '>', start_line, start_col))
            else: raise CompileError(f"未預期的字元 '{ch}'", start_line, start_col)

# ============================================================
# Bytecode Instruction
# ============================================================
class Instruction:
    __slots__ = ('op', 'arg', 'line')
    def __init__(self, op: OpCode, arg=None, line=0):
        self.op = op; self.arg = arg; self.line = line
    def __repr__(self):
        a = f" {self.arg}" if self.arg is not None else ""
        return f"{self.op.name}{a}"

# ============================================================
# Parser / Compiler
# ============================================================
class Compiler:
    def __init__(self, tokens: list):
        self.tokens = tokens; self.pos = 0
        self.code = []
        self.strings = []
        self.func_map = {}
        self.loop_stack = []
        self.local_count = 0

    def cur(self) -> Token: return self.tokens[self.pos]

    def advance(self) -> Token:
        t = self.tokens[self.pos]; self.pos += 1; return t

    def expect(self, tt: TokenType) -> Token:
        t = self.cur()
        if t.type != tt: error(f"預期 {tt.name} 但得到 '{t.text}'", t)
        return self.advance()

    def emit(self, op: OpCode, arg=None) -> int:
        idx = len(self.code)
        self.code.append(Instruction(op, arg, self.cur().line))
        return idx

    def patch(self, addr: int, target: int):
        self.code[addr].arg = target

    # ---- program ----
    def compile(self) -> tuple:
        while self.cur().type != TokenType.EOF:
            if self.cur().type == TokenType.FUNC: self.compile_func()
            else: self.compile_stmt()
        self.emit(OpCode.RET)
        return self.code, self.strings

    # ---- function ----
    def compile_func(self):
        self.advance()
        name = self.expect(TokenType.IDENT).text
        self.expect(TokenType.LPAREN)
        params = []
        if self.cur().type != TokenType.RPAREN:
            while True:
                params.append(self.expect(TokenType.IDENT).text)
                if self.cur().type != TokenType.COMMA: break
                self.advance()
        self.expect(TokenType.RPAREN)

        func_start = len(self.code)
        self.func_map[name] = func_start
        func_def_idx = self.emit(OpCode.FUNC_DEF, (name, params, -1))

        old_locals = self.local_count
        self.local_count = len(params)
        self.compile_block()
        if len(self.code) == 0 or self.code[-1].op != OpCode.RET:
            self.emit(OpCode.PUSH_NIL); self.emit(OpCode.RET)
        func_end = len(self.code)
        self.local_count = old_locals

        # Back-patch the end address into FUNC_DEF
        self.code[func_def_idx].arg = (name, params, func_end)

    # ---- statements ----
    def compile_stmt(self):
        t = self.cur()
        if t.type == TokenType.PRINT: self.compile_print()
        elif t.type == TokenType.IF: self.compile_if()
        elif t.type == TokenType.WHILE: self.compile_while()
        elif t.type == TokenType.FOR: self.compile_for()
        elif t.type == TokenType.RETURN: self.compile_return()
        elif t.type == TokenType.BREAK: self.compile_break()
        elif t.type == TokenType.CONTINUE: self.compile_continue()
        elif t.type == TokenType.IDENT: self.compile_assign_or_call()
        elif t.type == TokenType.SEMICOLON: self.advance()
        else:
            self.compile_expr()
            if self.cur().type == TokenType.SEMICOLON: self.advance()

    def compile_block(self):
        self.expect(TokenType.LBRACE)
        while self.cur().type not in (TokenType.RBRACE, TokenType.EOF):
            self.compile_stmt()
        self.expect(TokenType.RBRACE)

    def compile_print(self):
        self.advance()
        self.expect(TokenType.LPAREN)
        count = 0
        if self.cur().type != TokenType.RPAREN:
            while True:
                self.compile_expr(); count += 1
                if self.cur().type != TokenType.COMMA: break
                self.advance()
        self.expect(TokenType.RPAREN)
        self.expect(TokenType.SEMICOLON)
        self.emit(OpCode.PRINT, count)

    def compile_if(self):
        self.advance()
        self.expect(TokenType.LPAREN)
        self.compile_expr()
        self.expect(TokenType.RPAREN)
        jmp_false = self.emit(OpCode.JMP_FALSE, -1)
        self.compile_block()
        if self.cur().type == TokenType.ELSE:
            self.advance()
            jmp_end = self.emit(OpCode.JMP, -1)
            self.patch(jmp_false, len(self.code))
            if self.cur().type == TokenType.IF:
                self.compile_if()
            else:
                self.compile_block()
            self.patch(jmp_end, len(self.code))
        else:
            self.patch(jmp_false, len(self.code))

    def compile_while(self):
        self.advance()
        cond_addr = len(self.code)
        self.expect(TokenType.LPAREN)
        self.compile_expr()
        self.expect(TokenType.RPAREN)
        jmp_false = self.emit(OpCode.JMP_FALSE, -1)
        self.loop_stack.append({'break': [], 'continue': cond_addr})
        self.compile_block()
        self.emit(OpCode.JMP, cond_addr)
        end_addr = len(self.code)
        self.patch(jmp_false, end_addr)
        ctx = self.loop_stack.pop()
        for addr in ctx['break']: self.patch(addr, end_addr)

    def compile_for(self):
        self.advance()
        self.expect(TokenType.LPAREN)
        if self.cur().type != TokenType.SEMICOLON:
            self.compile_assign_or_call()
        self.expect(TokenType.SEMICOLON)
        cond_addr = len(self.code)
        if self.cur().type != TokenType.SEMICOLON:
            self.compile_expr()
        else:
            self.emit(OpCode.PUSH_TRUE)
        self.expect(TokenType.SEMICOLON)
        jmp_false = self.emit(OpCode.JMP_FALSE, -1)
        jmp_body = self.emit(OpCode.JMP, -1)
        step_addr = len(self.code)
        if self.cur().type != TokenType.RPAREN:
            self.compile_assign_or_call()
        self.expect(TokenType.RPAREN)
        self.emit(OpCode.JMP, cond_addr)
        self.patch(jmp_body, len(self.code))

        self.loop_stack.append({'break': [], 'continue': step_addr})
        self.compile_block()
        self.emit(OpCode.JMP, step_addr)
        end_addr = len(self.code)
        self.patch(jmp_false, end_addr)
        ctx = self.loop_stack.pop()
        for addr in ctx['break']: self.patch(addr, end_addr)

    def compile_return(self):
        self.advance()
        if self.cur().type != TokenType.SEMICOLON:
            self.compile_expr()
        else:
            self.emit(OpCode.PUSH_NIL)
        self.expect(TokenType.SEMICOLON)
        self.emit(OpCode.RET)

    def compile_break(self):
        self.advance()
        self.expect(TokenType.SEMICOLON)
        if not self.loop_stack: error("break 必須在迴圈內", self.cur())
        self.loop_stack[-1]['break'].append(self.emit(OpCode.JMP, -1))

    def compile_continue(self):
        self.advance()
        self.expect(TokenType.SEMICOLON)
        if not self.loop_stack: error("continue 必須在迴圈內", self.cur())
        self.emit(OpCode.JMP, self.loop_stack[-1]['continue'])

    def compile_assign_or_call(self):
        name_tok = self.advance()
        name = name_tok.text
        
        # Build access chain
        access_chain = []
        
        while True:
            if self.cur().type == TokenType.LBRACKET:
                self.advance()
                idx_result = self.compile_expr()
                self.expect(TokenType.RBRACKET)
                # Store index result in temp
                self.emit(OpCode.STORE, '_ac_idx')
                access_chain.append(('index',))
            elif self.cur().type == TokenType.DOT:
                self.advance()
                prop = self.expect(TokenType.IDENT).text
                sidx = self.get_string_index(prop)
                access_chain.append(('prop', sidx))
            else:
                break

        if self.cur().type == TokenType.ASSIGN:
            self.advance()
            self.compile_expr()  # value on stack
            
            if len(access_chain) == 0:
                self.resolve_store(name)
            elif len(access_chain) == 1:
                atype = access_chain[0][0]
                self.emit(OpCode.STORE, '_tmp_val')
                self.resolve_load(name)
                if atype == 'prop':
                    self.emit(OpCode.PUSH_STR, access_chain[0][1])
                elif atype == 'index':
                    self.emit(OpCode.LOAD, '_ac_idx')
                self.emit(OpCode.LOAD, '_tmp_val')
                self.emit(OpCode.SET_ITEM)
            else:
                raise CompileError("不支援的賦值形式", name_tok)
        elif self.cur().type == TokenType.LPAREN and len(access_chain) == 0:
            sidx = self.get_string_index(name)
            self.emit(OpCode.PUSH_STR, sidx)
            self.advance()
            argc = 0
            if self.cur().type != TokenType.RPAREN:
                while True:
                    self.compile_expr(); argc += 1
                    if self.cur().type != TokenType.COMMA: break
                    self.advance()
            self.expect(TokenType.RPAREN)
            self.emit(OpCode.CALL, argc)
            if self.cur().type == TokenType.SEMICOLON:
                self.emit(OpCode.POP)
                self.advance()
        else:
            self.resolve_load(name)
            for item in access_chain:
                if item[0] == 'index':
                    self.emit(OpCode.LOAD, '_ac_idx')
                    self.emit(OpCode.GET_ITEM)
                elif item[0] == 'prop':
                    self.emit(OpCode.PUSH_STR, item[1])
                    self.emit(OpCode.GET_ITEM)
            if self.cur().type == TokenType.LPAREN:
                self.advance()
                argc = 0
                if self.cur().type != TokenType.RPAREN:
                    while True:
                        self.compile_expr(); argc += 1
                        if self.cur().type != TokenType.COMMA: break
                        self.advance()
                self.expect(TokenType.RPAREN)
                self.emit(OpCode.CALL, argc)
            if self.cur().type == TokenType.SEMICOLON:
                self.emit(OpCode.POP)
                self.advance()

    def resolve_load(self, name: str):
        if name == 'true': self.emit(OpCode.PUSH_TRUE)
        elif name == 'false': self.emit(OpCode.PUSH_FALSE)
        elif name == 'nil': self.emit(OpCode.PUSH_NIL)
        else: self.emit(OpCode.LOAD, name)

    def resolve_store(self, name: str):
        self.emit(OpCode.STORE, name)

    # ---- expressions ----
    def compile_expr(self): return self.compile_or()

    def compile_or(self):
        self.compile_and()
        while self.cur().type == TokenType.OR:
            self.advance()
            # Duplicate the left result for the jump test
            self.emit(OpCode.DUP)
            # If truthy, skip right side (keep the duplicated truthy value)
            jmp_end = self.emit(OpCode.JMP, -1)
            # Pop the duplicate, evaluate right side
            self.emit(OpCode.POP)
            self.compile_and()
            self.patch(jmp_end, len(self.code))

    def compile_and(self):
        self.compile_equality()
        while self.cur().type == TokenType.AND:
            self.advance()
            # Duplicate the left result for the jump test
            self.emit(OpCode.DUP)
            # If falsy, skip right side (keep the duplicated falsy value)
            jmp_end = self.emit(OpCode.JMP_FALSE, -1)
            # Pop the duplicate, evaluate right side
            self.emit(OpCode.POP)
            self.compile_equality()
            self.patch(jmp_end, len(self.code))

    def compile_equality(self):
        self.compile_comparison()
        while self.cur().type in (TokenType.EQ, TokenType.NEQ):
            op = OpCode.EQ if self.cur().type == TokenType.EQ else OpCode.NEQ
            self.advance()
            self.compile_comparison()
            self.emit(op)

    def compile_comparison(self):
        self.compile_add()
        while self.cur().type in (TokenType.LT, TokenType.GT, TokenType.LTE, TokenType.GTE):
            tt = self.cur().type; self.advance()
            self.compile_add()
            ops = {TokenType.LT: OpCode.LT, TokenType.GT: OpCode.GT,
                   TokenType.LTE: OpCode.LTE, TokenType.GTE: OpCode.GTE}
            self.emit(ops[tt])

    def compile_add(self):
        self.compile_mul()
        while self.cur().type in (TokenType.PLUS, TokenType.MINUS):
            op = OpCode.ADD if self.cur().type == TokenType.PLUS else OpCode.SUB
            self.advance()
            self.compile_mul()
            self.emit(op)

    def compile_mul(self):
        self.compile_unary()
        while self.cur().type in (TokenType.MUL, TokenType.DIV, TokenType.MOD):
            tt = self.cur().type; self.advance()
            self.compile_unary()
            ops = {TokenType.MUL: OpCode.MUL, TokenType.DIV: OpCode.DIV,
                   TokenType.MOD: OpCode.MOD}
            self.emit(ops[tt])

    def compile_unary(self):
        if self.cur().type == TokenType.MINUS:
            self.advance()
            self.compile_unary()
            self.emit(OpCode.NEG)
        elif self.cur().type == TokenType.NOT:
            self.advance()
            self.compile_unary()
            self.emit(OpCode.OP_NOT)
        else:
            self.compile_postfix()

    def compile_postfix(self):
        # Check for IDENT( pattern - direct function call
        if self.cur().type == TokenType.IDENT and self.pos + 1 < len(self.tokens) and self.tokens[self.pos + 1].type == TokenType.LPAREN:
            fname = self.advance().text
            sidx = self.get_string_index(fname)
            self.emit(OpCode.PUSH_STR, sidx)
            self.advance()  # consume (
            argc = 0
            if self.cur().type != TokenType.RPAREN:
                while True:
                    self.compile_expr(); argc += 1
                    if self.cur().type != TokenType.COMMA: break
                    self.advance()
            self.expect(TokenType.RPAREN)
            self.emit(OpCode.CALL, argc)
            return

        self.compile_primary()
        while True:
            if self.cur().type == TokenType.LPAREN:
                # Variable holding callable - load variable first (already done by compile_primary)
                self.advance()
                argc = 0
                if self.cur().type != TokenType.RPAREN:
                    while True:
                        self.compile_expr(); argc += 1
                        if self.cur().type != TokenType.COMMA: break
                        self.advance()
                self.expect(TokenType.RPAREN)
                self.emit(OpCode.CALL, argc)
            elif self.cur().type == TokenType.LBRACKET:
                self.advance()
                self.compile_expr()
                self.expect(TokenType.RBRACKET)
                self.emit(OpCode.GET_ITEM)
            elif self.cur().type == TokenType.DOT:
                self.advance()
                prop = self.expect(TokenType.IDENT).text
                sidx = self.get_string_index(prop)
                self.emit(OpCode.PUSH_STR, sidx)
                self.emit(OpCode.GET_ITEM)
            else:
                break

    def compile_primary(self):
        t = self.cur()
        if t.type == TokenType.NUMBER:
            self.advance()
            val = float(t.text) if '.' in t.text else int(t.text)
            self.emit(OpCode.PUSH_INT, val)
        elif t.type == TokenType.STRING:
            self.advance()
            sidx = self.get_string_index(t.text)
            self.emit(OpCode.PUSH_STR, sidx)
        elif t.type in (TokenType.TRUE, TokenType.FALSE, TokenType.NIL, TokenType.IDENT):
            self.advance()
            self.resolve_load(t.text)
        elif t.type == TokenType.LBRACKET:
            self.advance()
            count = 0
            if self.cur().type != TokenType.RBRACKET:
                while True:
                    self.compile_expr(); count += 1
                    if self.cur().type != TokenType.COMMA: break
                    self.advance()
            self.expect(TokenType.RBRACKET)
            self.emit(OpCode.NEW_ARRAY, count)
        elif t.type == TokenType.LBRACE:
            self.advance()
            count = 0
            if self.cur().type != TokenType.RBRACE:
                while True:
                    if self.cur().type == TokenType.IDENT:
                        sidx = self.get_string_index(self.advance().text)
                        self.emit(OpCode.PUSH_STR, sidx)
                    elif self.cur().type == TokenType.STRING:
                        sidx = self.get_string_index(self.advance().text)
                        self.emit(OpCode.PUSH_STR, sidx)
                    else:
                        error("字典鍵必須是字串或識別碼", self.cur())
                    self.expect(TokenType.COLON)
                    self.compile_expr()
                    count += 1
                    if self.cur().type != TokenType.COMMA: break
                    self.advance()
            self.expect(TokenType.RBRACE)
            self.emit(OpCode.NEW_DICT, count)
        elif t.type == TokenType.LPAREN:
            self.advance()
            self.compile_expr()
            self.expect(TokenType.RPAREN)
        else:
            error(f"未預期的 token '{t.text}'", t)

    def get_string_index(self, s: str) -> int:
        if s not in self.strings:
            self.strings.append(s)
        return self.strings.index(s)

# ============================================================
# Stack-based VM
# ============================================================
class VMError(Exception): pass

class Frame:
    """Call stack frame for function calls"""
    __slots__ = ('ret_pc', 'env', 'param_names')
    def __init__(self, ret_pc=0, env=None, param_names=None):
        self.ret_pc = ret_pc
        self.env = env if env is not None else {}
        self.param_names = param_names if param_names is not None else []

class VM:
    def __init__(self, code: list, strings: list):
        self.code = code
        self.strings = strings
        self.stack = []
        self.global_env = {}
        self.frames = [Frame()]  # Bottom frame for global scope
        self.pc = 0
        self.func_table = {}  # name -> (start_pc, param_count)
        self._build_func_table()

    def _build_func_table(self):
        for i, instr in enumerate(self.code):
            if instr.op == OpCode.FUNC_DEF and isinstance(instr.arg, tuple):
                name, params, _ = instr.arg
                self.func_table[name] = (i + 1, params)  # (start_pc, param_names)

    def current_env(self):
        return self.frames[-1].env

    def load_var(self, name):
        for frame in reversed(self.frames):
            if name in frame.env:
                return frame.env[name]
        return self.global_env.get(name, 0)

    def store_var(self, name, value):
        self.frames[-1].env[name] = value

    def run(self):
        self.pc = 0
        while self.pc < len(self.code):
            instr = self.code[self.pc]
            op = instr.op
            arg = instr.arg

            if op == OpCode.PUSH_INT:
                self.stack.append(arg); self.pc += 1
            elif op == OpCode.PUSH_STR:
                self.stack.append(self.strings[arg]); self.pc += 1
            elif op == OpCode.PUSH_TRUE:
                self.stack.append(True); self.pc += 1
            elif op == OpCode.PUSH_FALSE:
                self.stack.append(False); self.pc += 1
            elif op == OpCode.PUSH_NIL:
                self.stack.append(None); self.pc += 1
            elif op == OpCode.LOAD:
                self.stack.append(self.load_var(arg)); self.pc += 1
            elif op == OpCode.STORE:
                self.store_var(arg, self.stack.pop()); self.pc += 1
            elif op == OpCode.DUP:
                self.stack.append(self.stack[-1]); self.pc += 1
            elif op == OpCode.POP:
                self.stack.pop(); self.pc += 1
            elif op == OpCode.ADD:
                b = self.stack.pop(); a = self.stack.pop()
                if isinstance(a, str) and isinstance(b, str):
                    self.stack.append(a + b)
                else:
                    self.stack.append(self.to_num(a) + self.to_num(b))
                self.pc += 1
            elif op == OpCode.SUB:
                b = self.stack.pop(); a = self.stack.pop()
                self.stack.append(self.to_num(a) - self.to_num(b)); self.pc += 1
            elif op == OpCode.MUL:
                b = self.stack.pop(); a = self.stack.pop()
                self.stack.append(self.to_num(a) * self.to_num(b)); self.pc += 1
            elif op == OpCode.DIV:
                b = self.stack.pop(); a = self.stack.pop()
                bn = self.to_num(b)
                if bn == 0: raise VMError("除以零")
                self.stack.append(self.to_num(a) / bn); self.pc += 1
            elif op == OpCode.MOD:
                b = self.stack.pop(); a = self.stack.pop()
                self.stack.append(self.to_num(a) % self.to_num(b)); self.pc += 1
            elif op == OpCode.NEG:
                self.stack.append(-self.to_num(self.stack.pop())); self.pc += 1
            elif op == OpCode.EQ:
                b = self.stack.pop(); a = self.stack.pop()
                self.stack.append(a == b); self.pc += 1
            elif op == OpCode.NEQ:
                b = self.stack.pop(); a = self.stack.pop()
                self.stack.append(a != b); self.pc += 1
            elif op == OpCode.LT:
                b = self.stack.pop(); a = self.stack.pop()
                self.stack.append(self.to_num(a) < self.to_num(b)); self.pc += 1
            elif op == OpCode.GT:
                b = self.stack.pop(); a = self.stack.pop()
                self.stack.append(self.to_num(a) > self.to_num(b)); self.pc += 1
            elif op == OpCode.LTE:
                b = self.stack.pop(); a = self.stack.pop()
                self.stack.append(self.to_num(a) <= self.to_num(b)); self.pc += 1
            elif op == OpCode.GTE:
                b = self.stack.pop(); a = self.stack.pop()
                self.stack.append(self.to_num(a) >= self.to_num(b)); self.pc += 1
            elif op == OpCode.OP_NOT:
                self.stack.append(not self.is_truthy(self.stack.pop())); self.pc += 1
            elif op == OpCode.OP_AND:
                b = self.stack.pop(); a = self.stack.pop()
                self.stack.append(self.is_truthy(a) and self.is_truthy(b)); self.pc += 1
            elif op == OpCode.OP_OR:
                b = self.stack.pop(); a = self.stack.pop()
                self.stack.append(self.is_truthy(a) or self.is_truthy(b)); self.pc += 1
            elif op == OpCode.JMP:
                self.pc = arg
            elif op == OpCode.JMP_FALSE:
                if not self.is_truthy(self.stack.pop()):
                    self.pc = arg
                else:
                    self.pc += 1
            elif op == OpCode.NEW_ARRAY:
                arr = []
                for _ in range(arg): arr.insert(0, self.stack.pop())
                self.stack.append(arr); self.pc += 1
            elif op == OpCode.NEW_DICT:
                d = {}
                for _ in range(arg):
                    val = self.stack.pop(); key = self.stack.pop()
                    d[key] = val
                self.stack.append(d); self.pc += 1
            elif op == OpCode.GET_ITEM:
                key = self.stack.pop(); obj = self.stack.pop()
                if isinstance(obj, list):
                    self.stack.append(obj[self.to_num(key)])
                elif isinstance(obj, dict):
                    self.stack.append(obj.get(key, None))
                elif isinstance(obj, str):
                    self.stack.append(obj[self.to_num(key)])
                else: raise VMError(f"無法索引 {type(obj).__name__}")
                self.pc += 1
            elif op == OpCode.SET_ITEM:
                val = self.stack.pop(); key = self.stack.pop(); obj = self.stack[-1]
                if isinstance(obj, list):
                    idx = self.to_num(key)
                    while len(obj) <= idx: obj.append(0)
                    obj[idx] = val
                elif isinstance(obj, dict):
                    obj[key] = val
                else: raise VMError(f"無法設定 {type(obj).__name__}")
                self.pc += 1
            elif op == OpCode.PRINT:
                args = []
                for _ in range(arg): args.insert(0, self.stack.pop())
                parts = []
                for a in args:
                    if isinstance(a, list): parts.append(self._arr_str(a))
                    elif isinstance(a, dict): parts.append(self._dict_str(a))
                    elif a is None: parts.append("nil")
                    elif isinstance(a, bool): parts.append("true" if a else "false")
                    else: parts.append(str(a))
                print(" ".join(parts))
                self.pc += 1
            elif op == OpCode.INPUT:
                prompt = self.stack.pop() if self.stack else ""
                self.stack.append(input(str(prompt)))
                self.pc += 1
            elif op == OpCode.CALL:
                argc = arg
                # Arguments are on top of the stack, function name is below them
                args = []
                for _ in range(argc):
                    args.insert(0, self.stack.pop())
                fname = self.stack.pop()

                # Try built-in functions first
                if self._builtin_call(fname, args):
                    self.pc += 1
                elif fname in self.func_table:
                    func_pc, param_names = self.func_table[fname]
                    new_env = {}
                    for i, pname in enumerate(param_names):
                        if i < len(args):
                            new_env[pname] = args[i]
                        else:
                            new_env[pname] = None
                    frame = Frame(ret_pc=self.pc + 1, env=new_env)
                    self.frames.append(frame)
                    self.pc = func_pc
                else:
                    raise VMError(f"未知函數: {fname}")
            elif op == OpCode.RET:
                ret_val = self.stack.pop() if self.stack else None
                if len(self.frames) > 1:
                    frame = self.frames.pop()
                    self.stack.append(ret_val)
                    self.pc = frame.ret_pc
                else:
                    return ret_val
            elif op == OpCode.FUNC_DEF:
                # Skip function body during global execution
                if isinstance(arg, tuple) and len(arg) == 3:
                    name, params, func_end = arg
                    self.pc = func_end
                else:
                    self.pc += 1
            else:
                raise VMError(f"未知 opcode: {op}")

    def _builtin_call(self, fname, args):
        if fname == "len":
            val = args[0] if args else None
            self.stack.append(len(val) if isinstance(val, (list, dict, str)) else 0)
            return True
        elif fname == "type":
            val = args[0] if args else None
            if isinstance(val, int) and not isinstance(val, bool): t = "int"
            elif isinstance(val, float): t = "float"
            elif isinstance(val, str): t = "string"
            elif isinstance(val, list): t = "array"
            elif isinstance(val, dict): t = "dict"
            elif isinstance(val, bool): t = "bool"
            elif val is None: t = "nil"
            else: t = "unknown"
            self.stack.append(t); return True
        elif fname == "input":
            prompt = args[0] if args else ""
            self.stack.append(input(str(prompt))); return True
        elif fname == "int":
            self.stack.append(int(args[0])); return True
        elif fname == "float":
            self.stack.append(float(args[0])); return True
        elif fname == "str":
            self.stack.append(str(args[0])); return True
        elif fname == "abs":
            self.stack.append(abs(self.to_num(args[0]))); return True
        elif fname == "sqrt":
            self.stack.append(math.sqrt(self.to_num(args[0]))); return True
        elif fname == "floor":
            self.stack.append(math.floor(self.to_num(args[0]))); return True
        elif fname == "ceil":
            self.stack.append(math.ceil(self.to_num(args[0]))); return True
        elif fname == "rand":
            import random
            if len(args) == 0: self.stack.append(random.random())
            elif len(args) == 1: self.stack.append(random.randint(0, self.to_num(args[0]) - 1))
            elif len(args) == 2: self.stack.append(random.randint(self.to_num(args[0]), self.to_num(args[1])))
            else: raise VMError("rand 最多 2 個參數")
            return True
        return False

    @staticmethod
    def to_num(v):
        if isinstance(v, bool): return 1 if v else 0
        if isinstance(v, (int, float)): return v
        return 0

    @staticmethod
    def is_truthy(v):
        if v is None: return False
        if isinstance(v, bool): return v
        if isinstance(v, (int, float)): return v != 0
        if isinstance(v, str): return len(v) > 0
        if isinstance(v, list): return len(v) > 0
        if isinstance(v, dict): return len(v) > 0
        return True

    @staticmethod
    def _arr_str(arr, seen=None):
        if seen is None: seen = set()
        if id(arr) in seen: return "[...]"
        seen.add(id(arr))
        items = []
        for v in arr:
            if isinstance(v, list): items.append(VM._arr_str(v, seen))
            elif isinstance(v, dict): items.append(VM._dict_str(v, seen))
            elif v is None: items.append("nil")
            elif isinstance(v, bool): items.append("true" if v else "false")
            else: items.append(str(v))
        return "[" + ", ".join(items) + "]"

    @staticmethod
    def _dict_str(d, seen=None):
        if seen is None: seen = set()
        if id(d) in seen: return "{...}"
        seen.add(id(d))
        items = []
        for k, v in d.items():
            ks = str(k)
            if isinstance(v, list): vs = VM._arr_str(v, seen)
            elif isinstance(v, dict): vs = VM._dict_str(v, seen)
            elif v is None: vs = "nil"
            elif isinstance(v, bool): vs = "true" if v else "false"
            else: vs = str(v)
            items.append(f"{ks}: {vs}")
        return "{" + ", ".join(items) + "}"

# ============================================================
# Disassembler
# ============================================================
def disassemble(code: list, strings: list):
    for i, instr in enumerate(code):
        arg = instr.arg
        if instr.op == OpCode.PUSH_STR and arg is not None:
            arg = strings[arg]
        arg_s = f" {arg}" if arg is not None else ""
        print(f"  {i:03d}  {instr.op.name}{arg_s}")

# ============================================================
# Main
# ============================================================
def main():
    if len(sys.argv) < 2:
        print(f"用法: python {sys.argv[0]} <source.m0> [--dump]")
        print("  --dump  顯示 bytecode")
        sys.exit(1)

    filepath = sys.argv[1]
    do_dump = '--dump' in sys.argv

    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            source = f.read()
    except Exception as e:
        print(f"錯誤: 無法開啟 {filepath}: {e}"); sys.exit(1)

    try:
        lexer = Lexer(source)
        compiler = Compiler(lexer.tokens)
        code, strings = compiler.compile()

        if do_dump:
            print("=== Bytecode ===")
            disassemble(code, strings)
            print()

        vm = VM(code, strings)
        result = vm.run()

    except CompileError as e:
        print(f"\n編譯錯誤: {e.msg} (第 {e.line} 行, 第 {e.col} 列)")
        sys.exit(1)
    except VMError as e:
        print(f"\n執行錯誤: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"\n未預期錯誤: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
