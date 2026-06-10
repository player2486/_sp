# 系統程式課程 — 平時作業彙總

> 金門大學資訊工程系 114 學年度 系統程式課程

## AI 使用聲明與原創性說明

本課程所有作業均**使用 AI 輔助完成**，聲明如下：

| 項目 | 說明 |
|------|------|
| **使用之 AI 工具** | Claude (Anthropic) + opencode (AI 程式設計助手) |
| **使用方式** | 由 AI 協助產生程式碼架構與文件草稿，本人逐行審閱、修改、測試後才提交。所有程式碼與文字的最終版本均由本人確認負責。 |
| **AI 對話紀錄** | 本作業使用 opencode CLI 工具（命令列介面）進行，採用逐輪對話方式由 AI 產生程式碼，無特定網站版對話分享連結。所有來往提示與修改記錄均保留於 git commit history 中。 |
| **參考來源** | 參考課程教材（陳鍾誠老師系統程式課程）、Linux man pages、Matter.js 官方文件、POSIX Threads 文件。/proc filesystem 文件。無直接複製任何同學或網路上的作品。 |
| **複製他人程式碼** | 無。所有程式碼皆為原創（AI 輔助產生後經本人修改），未複製其他同學或網路上的程式碼片段。 |
| **本人貢獻** | 需求規劃、架構設計審查、每一行程式碼的審閱與修改、除錯測試、文件撰寫與編輯。AI 扮演程式碼草稿生成與建議角色，最終品質由本人把關。 |

## 總覽

| 作業 | 主題 | 核心技術 | 連結 |
|------|------|---------|------|
| **HW1** | p0 語言編譯器 | C/Rust/Python/JS 編譯器 + Stack VM | [HW1/](HW1/) |
| **HW2** | m0 語言編譯器 | Python Bytecode Compiler + VM | [HW2/](HW2/) |
| **HW3** | Angry Birds 遊戲 | Matter.js 物理引擎, HTML5 Canvas | [HW3/](HW3/) |
| **HW4** | 系統程式設計書 | 系統程式全面導論（14 章） | [HW4/](HW4/) |
| **HW5** | 執行緒與同步 | pthread, mutex, 競爭條件, 死結 | [HW5/](HW5/) |
| **HW6** | 行程與檔案操作 | fork, exec, dup2, pipe, fd | [HW6/](HW6/) |
| **HW7** | Process Monitor | /proc 檔案系統, ANSI TUI | [HW7/](HW7/) |
| **114b期中** | Process Monitor（期中專題） | /proc 檔案系統, ANSI TUI | [114b期中/](114b期中/) |

---

## HW1：p0 語言編譯器

一個完整的編譯器專案，支援四種語言實作（C、Rust、Python、JavaScript）。包含詞法分析、語法分析、程式碼產生與 Stack-based 虛擬機。

- 支援：while 迴圈、for 迴圈、if-else、函數呼叫、遞迴
- 測試程式的 p0 原始碼在 `p0/` 目錄中

## HW2：m0 語言編譯器

用 Python 實作的簡易程式語言，將原始碼編譯為 bytecode 後由 Stack-based VM 直譯執行。支援動態型態、函數定義、遞迴呼叫。

## HW3：Angry Birds

使用 Matter.js 物理引擎和 HTML5 Canvas 實作的憤怒鳥遊戲。包含彈弓瞄準發射機制、多種物理材質、關卡設計、碰撞偵測與計分系統。

## HW4：系統程式設計書

以 14 章節的篇幅，從硬體架構到作業系統，從編譯器到虛擬化，全面介紹系統程式設計。每章包含觀念說明與程式碼範例。

## HW5：執行緒、同步與並行程式設計

涵蓋執行緒概念、競爭條件、互斥鎖（Mutex）、死結等主題，搭配三個實作範例：
- 銀行存提款模擬（Mutex 保護）
- 生產者消費者問題（條件變數）
- 哲學家用餐問題（死結預防）

## HW6：行程與檔案操作

深入探討 Linux 行程管理與檔案 I/O 的核心系統呼叫：
- fork 系列：基本 fork、父子關係、exec、殭屍、孤兒
- file 系列：open/read/write/close、stdin/stdout/stderr、dup2 重新導向、pipe 管線

## HW7 / 114b期中：Process Monitor

即時行程監控工具（類似 `top`），讀取 `/proc` 檔案系統取得系統資訊，以 ANSI escape code 實作終端機介面。支援行程列表、CPU%/MEM% 計算、多種排序、狀態著色。
