# HW6：行程與檔案操作

本專案涵蓋 Linux 系統程式設計中行程管理與檔案操作的核心觀念，包含 fork、exec、檔案描述子、dup2 等。

## 目錄結構

```
HW6/
├── README.md
├── doc/
│   ├── process.md              # 行程觀念 (fork, exec, zombie, orphan)
│   └── file-descriptor.md      # 檔案描述子觀念 (fd, dup2, pipe)
├── fork/
│   ├── 01-fork-basic.c         # fork 基本概念
│   ├── 02-fork-child.c         # 父子行程關係
│   ├── 03-fork-exec.c          # fork + execvp
│   ├── 04-fork-zombie.c        # 殭屍行程
│   ├── 05-fork-orphan.c        # 孤兒行程
│   └── Makefile
└── file/
    ├── 01-basic-rw.c           # open/read/write/close
    ├── 02-stdio.c              # stdin/stdout/stderr
    ├── 03-dup2-redirect.c      # dup2 重新導向
    ├── 04-dup2-pipe.c          # pipe + dup2 模擬 shell pipe
    └── Makefile
```

## 編譯與執行

```bash
# 全部編譯
cd fork && make && cd ..
cd file && make && cd ..

# 或個別編譯
gcc -o fork-basic fork/01-fork-basic.c && ./fork-basic

# 一次編譯全部
gcc -o fork-basic     fork/01-fork-basic.c
gcc -o fork-child     fork/02-fork-child.c
gcc -o fork-exec      fork/03-fork-exec.c
gcc -o fork-zombie    fork/04-fork-zombie.c
gcc -o fork-orphan    fork/05-fork-orphan.c
gcc -o basic-rw       file/01-basic-rw.c
gcc -o stdio          file/02-stdio.c
gcc -o dup2-redirect  file/03-dup2-redirect.c
gcc -o dup2-pipe      file/04-dup2-pipe.c
```

## 參考資料

- https://github.com/ccc114b/cccocw/tree/main/系統程式/06-Linux系統程式/03-fork
- https://github.com/ccc114b/cccocw/tree/main/系統程式/06-Linux系統程式/04-fs
