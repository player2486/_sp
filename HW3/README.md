# HW3：Angry Birds（憤怒鳥）

**本作業使用 AI 輔助**

- **使用 AI**: Claude (opencode)
- **使用方式**: 由 AI 協助產生遊戲架構與物理引擎互動邏輯，本人審閱並修改每一行程式碼
- **參考來源**: 無直接複製他人程式碼，參考 Matter.js 官方文件
- **原創性說明**: 本專案為原創作品，非複製他人程式碼。關卡設計、遊戲機制為原創。

## 簡介

使用 Matter.js 物理引擎和 HTML5 Canvas 實作的 Angry Birds 仿作。

## 技術

- **物理引擎**: Matter.js（碰撞、重力、彈性）
- **渲染**: HTML5 Canvas API
- **語言**: 純前端 JavaScript（無後端需求）

## 功能

- 彈弓瞄準與發射機制
- 多種物理材質（木頭、石頭、玻璃）
- 關卡設計（多個關卡）
- 碰撞偵測與傷害計算
- 計分系統
- 小鳥數量管理

## 執行

```bash
# 直接開啟 index.html 即可遊玩
open index.html
# 或使用任何 HTTP 伺服器
python3 -m http.server 8080
# 瀏覽器打開 http://localhost:8080
```
