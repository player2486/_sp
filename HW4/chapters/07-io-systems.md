# 第七章：輸入輸出系統

## I/O 硬體基礎

### I/O 裝置類型

- **區塊裝置**：以固定大小的區塊存取（硬碟、SSD）
- **字元裝置**：以位元組串流存取（鍵盤、序列埠）
- **網路裝置**：以封包方式傳輸（網卡）

### I/O 連接方式

```
CPU ──→ 系統匯流排 ──→ I/O 控制器 ──→ 裝置
                        ↓
                    連接埠 / MMIO / DMA
```

## I/O 控制方式

### Programmed I/O（PIO）

CPU 直接讀寫 I/O 暫存器，每次傳送一個位元組。

```c
// 假設使用 PIO 讀取鍵盤狀態
while (!(inb(KEYBOARD_STATUS) & READY))
    ;  // 等待
char c = inb(KEYBOARD_DATA);
```

### 中斷驅動 I/O

裝置完成操作後發出中斷，CPU 暫停當前工作進行處理。

```
1. CPU 發起 I/O 操作
2. CPU 繼續執行其他工作
3. 裝置完成操作
4. 裝置發出中斷信號
5. CPU 保存上下文
6. CPU 執行中斷處理常式
7. CPU 恢復上下文
8. CPU 繼續原工作
```

### DMA（Direct Memory Access）

DMA 控制器直接在裝置與記憶體間傳輸資料。

```c
// DMA 設定流程（簡化）
struct dma_descriptor desc;

desc.source      = device_addr;
desc.destination = memory_buffer;
desc.size        = 4096;
desc.interrupt   = 1;  // 完成後中斷

dma_start(&desc);
// CPU 可以在此期間做其他事
```

## 裝置驅動程式

驅動程式是核心中與特定硬體溝通的模組：

```c
// Linux 字元裝置驅動程式框架
#include <linux/fs.h>
#include <linux/cdev.h>

static int hw4_open(struct inode *inode, struct file *file)
{
    // 初始化裝置
    return 0;
}

static ssize_t hw4_read(struct file *file, char __user *buf,
                         size_t len, loff_t *off)
{
    // 從裝置讀取資料
    // 使用 copy_to_user() 將資料複製到使用者空間
    return 0;
}

static ssize_t hw4_write(struct file *file, const char __user *buf,
                          size_t len, loff_t *off)
{
    // 寫入資料到裝置
    // 使用 copy_from_user() 接收使用者資料
    return 0;
}

static struct file_operations hw4_fops = {
    .owner = THIS_MODULE,
    .open  = hw4_open,
    .read  = hw4_read,
    .write = hw4_write,
};
```

## I/O 多工

高效處理多個 I/O 來源的方式：

### select / poll

```c
fd_set read_fds;
FD_ZERO(&read_fds);
FD_SET(fd1, &read_fds);
FD_SET(fd2, &read_fds);

select(max_fd + 1, &read_fds, NULL, NULL, NULL);

if (FD_ISSET(fd1, &read_fds))
    handle_fd1();
if (FD_ISSET(fd2, &read_fds))
    handle_fd2();
```

### epoll（Linux）

```c
int epfd = epoll_create1(0);

struct epoll_event ev;
ev.events = EPOLLIN;
ev.data.fd = fd;
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

struct epoll_event events[10];
int n = epoll_wait(epfd, events, 10, -1);

for (int i = 0; i < n; i++) {
    handle_event(events[i].data.fd);
}
```

### io_uring（現代 Linux）

```c
// io_uring 使用共用 ring buffer 減少系統呼叫
struct io_uring ring;
io_uring_queue_init(32, &ring, 0);

struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
io_uring_prep_read(sqe, fd, buf, size, offset);
io_uring_submit(&ring);

// 等待完成
struct io_uring_cqe *cqe;
io_uring_wait_cqe(&ring, &cqe);
// 處理結果
io_uring_cqe_seen(&ring, cqe);
```

## I/O 效能比較

| 方式 | 系統呼叫次數 | CPU 使用率 | 適合場景 |
|-----|------------|-----------|---------|
| 同步 read/write | 每次 I/O 一次 | 低 | 簡單程式 |
| select/poll | 每次 I/O 一次 | 中 | 中等並發 |
| epoll | 批次 | 低 | 大量連線 |
| io_uring | 接近零 | 極低 | 高效能 I/O |
| mmap | 設定一次 | 低 | 隨機存取 |

---

**上一章**：[第六章：檔案系統](06-file-systems.md)  
**下一章**：[第八章：並行與同步](08-concurrency.md)
