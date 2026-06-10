# 第十二章：網路程式設計

## OSI 模型 vs TCP/IP 模型

```
OSI 模型              TCP/IP 模型
+-----------+         +-----------+
| 應用層    |         | 應用層    |
| 表現層    |         |           |
| 會議層    |         |           |
+-----------+         +-----------+
| 傳輸層    |         | 傳輸層    |
+-----------+         +-----------+
| 網路層    |         | 網路層    |
+-----------+         +-----------+
| 資料連結層 |         | 網路介面  |
| 實體層    |         |           |
+-----------+         +-----------+
```

## Socket API

Socket 是網路通訊的端點，提供統一的程式設計介面。

### TCP 伺服器流程

```c
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr = htonl(INADDR_ANY)
    };

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("Server listening on port 8080\n");

    while (1) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client,
                               &client_len);

        char buffer[1024] = {0};
        read(client_fd, buffer, sizeof(buffer));
        printf("Received: %s\n", buffer);

        char *response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, HW4!\n";
        write(client_fd, response, strlen(response));

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
```

### TCP 客戶端

```c
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server = {
        .sin_family = AF_INET,
        .sin_port = htons(8080)
    };
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    connect(sock, (struct sockaddr *)&server, sizeof(server));

    char *msg = "Hello from HW4 client!";
    write(sock, msg, strlen(msg));

    char buffer[1024] = {0};
    read(sock, buffer, sizeof(buffer));
    printf("Server: %s\n", buffer);

    close(sock);
    return 0;
}
```

## UDP 通訊

```c
// UDP 伺服器
int fd = socket(AF_INET, SOCK_DGRAM, 0);

struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(8080),
    .sin_addr = htonl(INADDR_ANY)
};
bind(fd, (struct sockaddr *)&addr, sizeof(addr));

char buf[1024];
struct sockaddr_in client;
socklen_t len = sizeof(client);
recvfrom(fd, buf, sizeof(buf), 0,
         (struct sockaddr *)&client, &len);
sendto(fd, "OK", 2, 0,
       (struct sockaddr *)&client, len);
```

```c
// UDP 客戶端
int fd = socket(AF_INET, SOCK_DGRAM, 0);

struct sockaddr_in server = {
    .sin_family = AF_INET,
    .sin_port = htons(8080)
};
inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

sendto(fd, "Hello", 5, 0,
       (struct sockaddr *)&server, sizeof(server));

char buf[1024];
socklen_t len = sizeof(server);
recvfrom(fd, buf, sizeof(buf), 0,
         (struct sockaddr *)&server, &len);
```

## 非阻塞 I/O 與事件驅動

### 使用 epoll 的高效能伺服器

```c
#include <sys/epoll.h>

#define MAX_EVENTS 10

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // ... bind, listen ...

    int epfd = epoll_create1(0);
    struct epoll_event ev = {
        .events = EPOLLIN,
        .data.fd = server_fd
    };
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

    struct epoll_event events[MAX_EVENTS];

    while (1) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == server_fd) {
                // 接受新連線
                int client = accept(server_fd, NULL, NULL);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client, &ev);
            } else {
                // 處理客戶端資料
                handle_client(events[i].data.fd);
            }
        }
    }
}
```

## 常用網路工具

```bash
# 查看連線狀態
netstat -tlnp
ss -tlnp

# DNS 查詢
nslookup example.com
dig example.com

# HTTP 請求
curl -v http://localhost:8080

# 封包分析
tcpdump -i eth0 port 80
```

---

**上一章**：[第十一章：系統呼叫](11-system-calls.md)
**下一章**：[第十三章：虛擬化與容器](13-virtualization.md)
