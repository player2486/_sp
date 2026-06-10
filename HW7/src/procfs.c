#include "procfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>

/*
 * 解析 /proc/[pid]/stat
 *
 * 格式範例：
 *   1234 (process name) R 0 0 0 ...
 *
 * 注意：process name 用括號包圍，名稱內可能包含空格！
 * 所以不能直接用 sscanf 依賴空格分割。
 */
static int parse_proc_stat(const char *path, ProcessInfo *info)
{
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    char line[1024];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    /*
     * 手動解析：跳過 pid，找到 '(', 跳過 ')' 前的名稱
     */
    char *p = line;

    /* PID */
    info->pid = atol(p);
    p = strchr(p, ' ');

    /* 跳到 '(' 後面 */
    p = strchr(p, '(') + 1;

    /* 找到 ')'，複製名稱 */
    char *end = strrchr(p, ')');
    if (!end) return -1;

    int name_len = end - p;
    if (name_len >= NAME_LEN) name_len = NAME_LEN - 1;
    strncpy(info->name, p, name_len);
    info->name[name_len] = '\0';

    /* 跳過 ") " */
    p = end + 2;

    /*
     * 使用 sscanf 解析後面的數值欄位
     * fields after name (0-indexed):
     *   0: pid (already parsed)
     *   1: comm (already parsed)
     *   2: state
     *   3: ppid
     *   ...
     *   11: utime
     *   12: stime
     *   23: vsize
     *   24: rss
     */
    char state;
    long ppid, utime, stime, vsize, rss;

    if (sscanf(p, "%c %ld %*d %*d %*d %*d %*d %*d %*d %*d %*d %ld %ld %*d %*d %*d %*d %*d %*d %*d %ld %ld",
               &state, &ppid,
               &utime, &stime,
               &vsize, &rss) < 6) {
        return -1;
    }

    info->state = state;
    info->ppid = ppid;
    info->utime = utime;
    info->stime = stime;
    info->total_time = utime + stime;
    info->vsize_kb = vsize / 1024;
    info->rss_pages = rss;
    info->cpu_percent = 0.0f;
    info->mem_percent = 0.0f;

    return 0;
}

/* 讀取 /proc/stat */
CpuTime read_cpu_time(void)
{
    CpuTime ct = {0};
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return ct;

    char line[256];
    if (fgets(line, sizeof(line), fp)) {
        sscanf(line,
            "cpu %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
            &ct.user, &ct.nice, &ct.system, &ct.idle,
            &ct.iowait, &ct.irq, &ct.softirq, &ct.steal,
            &ct.guest, &ct.guest_nice);
        ct.total = ct.user + ct.nice + ct.system + ct.idle
                 + ct.iowait + ct.irq + ct.softirq + ct.steal;
    }
    fclose(fp);
    return ct;
}

/* 讀取 /proc/meminfo */
MemInfo read_mem_info(void)
{
    MemInfo mi = {0};
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return mi;

    char line[128];
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %ld kB", &mi.mem_total_kb) == 1) continue;
        if (sscanf(line, "MemFree: %ld kB", &mi.mem_free_kb) == 1) continue;
        if (sscanf(line, "MemAvailable: %ld kB", &mi.mem_avail_kb) == 1) continue;
    }
    fclose(fp);
    return mi;
}

/* 讀取所有行程 */
int read_processes(ProcessInfo *procs, int max_count)
{
    DIR *dir = opendir("/proc");
    if (!dir) return -1;

    MemInfo mi = read_mem_info();
    int count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL && count < max_count) {
        /* 只處理數字目錄（行程 ID） */
        if (entry->d_type != DT_DIR) continue;
        if (!isdigit(entry->d_name[0])) continue;

        char path[512];
        snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name);

        ProcessInfo info;
        if (parse_proc_stat(path, &info) == 0) {
            if (mi.mem_total_kb > 0) {
                long rss_bytes = info.rss_pages * sysconf(_SC_PAGESIZE);
                info.mem_percent = (float)rss_bytes / (mi.mem_total_kb * 1024) * 100.0f;
            }
            procs[count++] = info;
        }
    }

    closedir(dir);
    return count;
}

/* 計算 CPU 使用率（需兩次取樣）*/
void calc_cpu_percent(ProcessInfo *procs, int count,
                      CpuTime *prev_cpu, CpuTime *curr_cpu,
                      ProcessInfo *prev_procs, ProcessInfo *curr_procs)
{
    (void)procs;
    long total_diff = curr_cpu->total - prev_cpu->total;
    if (total_diff <= 0) return;

    for (int i = 0; i < count; i++) {
        pid_t pid = curr_procs[i].pid;

        /* 找到該行程的上一次資料 */
        for (int j = 0; j < count; j++) {
            if (prev_procs[j].pid == pid) {
                long time_diff = curr_procs[i].total_time
                               - prev_procs[j].total_time;
                curr_procs[i].cpu_percent =
                    (float)time_diff / total_diff * 100.0f;
                break;
            }
        }
    }
}

/* 比較函式 for qsort */
static int cmp_pid(const void *a, const void *b)
{
    return ((ProcessInfo *)a)->pid - ((ProcessInfo *)b)->pid;
}

static int cmp_mem(const void *a, const void *b)
{
    float diff = ((ProcessInfo *)b)->rss_pages - ((ProcessInfo *)a)->rss_pages;
    return (diff > 0) - (diff < 0);
}

static int cmp_cpu(const void *a, const void *b)
{
    float diff = ((ProcessInfo *)b)->cpu_percent - ((ProcessInfo *)a)->cpu_percent;
    return (diff > 0) - (diff < 0);
}

static int cmp_name(const void *a, const void *b)
{
    return strcmp(((ProcessInfo *)a)->name, ((ProcessInfo *)b)->name);
}

void sort_by_pid(ProcessInfo *procs, int count)  { qsort(procs, count, sizeof(ProcessInfo), cmp_pid); }
void sort_by_mem(ProcessInfo *procs, int count)  { qsort(procs, count, sizeof(ProcessInfo), cmp_mem); }
void sort_by_cpu(ProcessInfo *procs, int count)  { qsort(procs, count, sizeof(ProcessInfo), cmp_cpu); }
void sort_by_name(ProcessInfo *procs, int count) { qsort(procs, count, sizeof(ProcessInfo), cmp_name); }
