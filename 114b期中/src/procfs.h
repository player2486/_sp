#ifndef PROCFS_H
#define PROCFS_H

#include <sys/types.h>
#include <stdio.h>

#define MAX_PROCESSES 4096
#define NAME_LEN 256

typedef struct {
    long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
    long total;  /* 所有項目加總 */
} CpuTime;

typedef struct {
    pid_t pid;
    pid_t ppid;
    char name[NAME_LEN];
    char state;            /* R, S, D, Z, T */
    long utime;            /* user mode jiffies */
    long stime;            /* kernel mode jiffies */
    long total_time;       /* utime + stime */
    long rss_pages;        /* RSS in pages */
    long vsize_kb;         /* virtual memory in KB */
    float cpu_percent;     /* CPU 使用率 % */
    float mem_percent;     /* 記憶體使用率 % */
} ProcessInfo;

typedef struct {
    long mem_total_kb;
    long mem_free_kb;
    long mem_avail_kb;
} MemInfo;

/* 讀取 /proc/stat 取得 CPU 時間 */
CpuTime read_cpu_time(void);

/* 讀取 /proc/meminfo 取得記憶體資訊 */
MemInfo read_mem_info(void);

/* 讀取所有行程資訊 */
int read_processes(ProcessInfo *procs, int max_count);

/* 計算 CPU 使用率（填入 procs 的 cpu_percent）*/
void calc_cpu_percent(ProcessInfo *procs, int count,
                      CpuTime *prev_cpu, CpuTime *curr_cpu,
                      ProcessInfo *prev_procs, ProcessInfo *curr_procs);

/* 排序 */
void sort_by_pid(ProcessInfo *procs, int count);
void sort_by_mem(ProcessInfo *procs, int count);
void sort_by_cpu(ProcessInfo *procs, int count);
void sort_by_name(ProcessInfo *procs, int count);

#endif
