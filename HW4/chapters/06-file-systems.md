# 第六章：檔案系統

## 檔案系統基礎

檔案系統負責組織、儲存、檢索磁碟上的資料。

### 檔案概念

- **檔案**：位元組的集合，具有名稱
- **目錄**：包含檔案和其他目錄的特殊檔案
- **路徑**：從根目錄到檔案的定位字串

### 常見檔案系統

| 檔案系統 | 作業系統 | 特性 |
|---------|---------|------|
| ext4 | Linux | 日誌式、廣泛使用 |
| NTFS | Windows | ACL、加密、壓縮 |
| APFS | macOS | 快照、加密、空間共享 |
| FAT32 | 通用 | 簡單、最大 4GB 單檔 |
| XFS | Linux | 高效能、大檔案 |

## 磁碟結構

```
+------------------------------------------+
|  MBR/GPT  |  分割區表  |  分割區 1  | ... |
+------------------------------------------+

分割區內部：
+------------------+
| 開機區塊          |
| 超級區塊          |
| inode 表格        |
| 資料區塊          |
+------------------+
```

## inode 結構

Unix 檔案系統中，每個檔案由一個 inode 描述：

```c
struct ext4_inode {
    __le16  i_mode;       // 檔案類型與權限
    __le16  i_uid;        // 擁有者 UID
    __le32  i_size;       // 檔案大小
    __le32  i_atime;      // 最後存取時間
    __le32  i_ctime;      // 最後狀態變更時間
    __le32  i_mtime;      // 最後修改時間
    __le32  i_dtime;      // 刪除時間
    __le16  i_gid;        // 群組 GID
    __le16  i_links_count; // 硬連結數
    __le32  i_blocks;     // 區塊數
    __le32  i_block[15];  // 區塊指標（12 直接 + 1 間接 + 1 雙重 + 1 三重）
    // ...
};
```

### 區塊指標

```c
// 直接區塊：直接指向資料區塊
i_block[0..11] → 資料區塊

// 間接區塊：指向包含區塊指標的區塊
i_block[12] → 間接區塊 → 資料區塊

// 雙重間接
i_block[13] → 雙重間接 → 間接區塊 → 資料區塊

// 三重間接
i_block[14] → 三重間接 → 雙重間接 → 間接區塊 → 資料區塊
```

## VFS（Virtual File System）

Linux VFS 提供統一的檔案系統介面：

```
+------------------------------------------+
|           系統呼叫（open、read、write）     |
+------------------------------------------+
|              VFS 層                       |
+------------------------------------------+
|  ext4  |  NTFS  |  FAT  |  tmpfs  | ...  |
+------------------------------------------+
|              裝置驅動程式                   |
+------------------------------------------+
```

## 範例：自製簡易檔案系統操作

```c
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

void list_directory(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        struct stat st;
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath),
                 "%s/%s", path, entry->d_name);

        if (stat(fullpath, &st) == 0) {
            char type = S_ISDIR(st.st_mode) ? 'd' : '-';
            printf("%c %s (大小: %ld bytes)\n",
                   type, entry->d_name, st.st_size);
        }
    }
    closedir(dir);
}

int main() {
    list_directory(".");
    return 0;
}
```

## 範例：使用系統呼叫操作檔案

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    // open
    int fd = open("hw4.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    // write
    const char *msg = "HW4: System Programming Book\n";
    write(fd, msg, strlen(msg));

    // lseek
    lseek(fd, 0, SEEK_SET);

    // close
    close(fd);

    // read back
    fd = open("hw4.txt", O_RDONLY);
    char buf[128] = {0};
    read(fd, buf, sizeof(buf) - 1);
    printf("讀取內容: %s", buf);
    close(fd);

    return 0;
}
```

---

**上一章**：[第五章：記憶體管理](05-memory-management.md)  
**下一章**：[第七章：輸入輸出系統](07-io-systems.md)
