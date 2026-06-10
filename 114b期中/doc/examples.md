# 執行範例

## 基本命令

```
msh> ls -la
total 60
drwxr-xr-x 2 user user  4096 Jun 10 16:45 .
drwxr-xr-x 3 user user  4096 Jun 10 16:44 ..
-rw-r--r-- 1 user user  1286 Jun 10 16:45 Makefile
-rw-r--r-- 1 user user 10562 Jun 10 16:45 msh.c
```

## 管線

```
msh> ls -l | grep .c | wc -l
3

msh> ps aux | grep bash | head -3
user       310  0.0  0.0  10336  3728 ?        S    16:00   0:00 bash
user      1422  0.0  0.0  10336  3492 pts/0    Ss   16:30   0:00 bash
```

## 重新導向

```
msh> echo "hello world" > test.txt
msh> cat < test.txt
hello world

msh> ls nonexistent 2> error.log
msh> cat error.log
ls: cannot access 'nonexistent': No such file or directory
```

## 背景執行

```
msh> sleep 10 &
[1] 1567
msh> jobs
[1] Running    sleep 10 &
msh> kill 1567
[1] Terminated    sleep 10 &
```

## 內建指令

```
msh> pwd
/home/user
msh> cd /tmp
msh> pwd
/tmp
msh> cd -
/home/user
msh> exit
```
