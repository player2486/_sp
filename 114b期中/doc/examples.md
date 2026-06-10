# 執行範例

## 啟動

```
$ make
gcc -Wall -Wextra -o hw7-top src/main.c src/procfs.c src/display.c

$ ./hw7-top
```

## 畫面說明

```
HW7 Process Monitor     16:30:31     更新中... (按 q 離開)
行程: 26     CPU: user=938  sys=644  idle=61206  Mem: 1385 MB / 7851 MB (18%)

  PID     %CPU     %MEM     NAME                  STATE     RSS
     1    0.0      0.0     systemd                SLEEP     20M
    39    0.0      0.1     systemd-journal        SLEEP     10M
    92    0.0      0.0     systemd-udevd          SLEEP     5M
   304    0.0      0.0     SessionLeader          SLEEP     6M
   310    0.0      0.0     bash                   SLEEP     4M
    ...
```

## 排序操作

預設依 CPU% 排序。按 `2` 切換到 CPU、`3` 切換到 MEM：

```
排序: %CPU  |  1:PID  2:CPU  3:MEM  4:NAME  q:離開
```

## 狀態著色

- 綠色：Running（執行中）
- 青色：Sleep（休眠中）
- 紅色：Zombie（殭屍）
