# 🚀 Spacecraft Game

以 **C++、OpenGL 與 GLUT** 開發的 3D 太空射擊遊戲。

玩家需要操控太空船在宇宙場景中移動，閃避迎面而來的障礙物，並使用子彈攻擊敵方物件，在限定時間內取得足夠分數。

---

## 🎮 Game Objective

玩家初始擁有 **8 點生命值**，遊戲時間為 **40 秒**。

遊戲過程中需要：

- 操控太空船閃避迎面而來的障礙物
- 發射子彈攻擊 UFO 與 Alien 類型敵人
- 擊破可攻擊敵人以獲得分數與生命值
- 避免生命值降至 0

### 勝利條件

在 40 秒倒數結束時：

- `Score >= 50` → **YOU WIN**
- `Score < 50` → **GAME OVER**
- 若生命值提前降至 0，遊戲會立即結束

每擊破一個可攻擊敵人可獲得：

- `+10` 分
- `+2` 生命值
- 生命值最高維持在 8

---

## ✨ Features

- 3D 太空船控制
- `.3ds` 3D 模型載入與繪製
- BMP Texture Mapping
- 六面 Skybox 太空背景
- 隨機生成多種類型障礙物
- 遊戲開始時產生最多 80 個障礙物
- 障礙物自動向玩家方向移動
- 障礙物離開場景後重新生成
- 球體碰撞偵測（Sphere Collision Detection）
- 子彈射擊系統
- 射擊冷卻機制
- 即時 Score 顯示
- 40 秒 Countdown Timer
- 8 格 Heart 生命值系統
- 碰撞傷害與生命回復機制
- Explosion Particle Effect
- Spacecraft Thruster Particle Effect
- Win / Game Over 判定
- Fill / Wireframe 顯示模式切換

---

## ☄️ Game Objects

遊戲會隨機產生以下類型的物件：

- Asteroid / Rock
- UFO
- Alien
- Sun-like Object

場景中最多維持 80 個障礙物，障礙物會持續向玩家方向移動。

當障礙物離開有效範圍，或被子彈擊破時，會重新隨機生成位置與類型。

其中：

- UFO 與 Alien 屬於主要可攻擊敵人
- Rock 類型屬於一般障礙物
- Sun-like Object 屬於不可獲得分數的障礙物

成功擊破 UFO 或 Alien 後，可以獲得分數與生命值。

---

## 🕹️ Controls

| Key   | Action |
| ----- | ------ |
| `W`   | 向上移動 |
| `S`   | 向下移動 |
| `A`   | 向左移動 |
| `D`   | 向右移動 |
| `Z`   | 向前移動 |
| `X`   | 向後移動 |
| `G`   | 發射子彈 |
| `R`   | 切換 Fill / Wireframe 模式 |
| `Esc` | 離開遊戲 |

---

## ❤️ Health System

玩家初始生命值為：

```text
8 Hearts
```

與不同物件碰撞時會造成不同傷害：

- 一般障礙物：`-1 Heart`
- UFO / Alien 類型敵人：`-2 Hearts`

若使用子彈成功擊破 UFO 或 Alien：

```text
Score  +10
Health +2
```

生命值最高為 8。

如果子彈擊中 Rock 或 Sun-like Object，子彈會消失，但不會增加分數或生命值。

---

## 💥 Particle Effects

遊戲加入兩種主要粒子效果。

### Thruster Effect

太空船飛行過程中會持續產生推進器粒子，用來模擬引擎噴射效果。

### Explosion Effect

當以下事件發生時，會產生爆炸粒子：

- 太空船撞擊障礙物
- 子彈擊中敵人

不同事件會使用不同的粒子數量、速度、大小與生命週期，形成不同的爆炸效果。

---

## 🛠️ Tech Stack

| Technology | Usage |
| ---------- | ----- |
| C++ | 遊戲主要邏輯 |
| OpenGL | 3D 場景與物件渲染 |
| GLUT | 視窗建立、鍵盤事件與遊戲迴圈 |
| GLU | Perspective 與 Camera 設定 |
| Win32 API | 字型顯示與 Windows 相關功能 |
| `.3ds` | 3D Model 格式 |
| `.bmp` | Texture 與 Skybox 圖片 |
| Visual Studio 2022 | 專案建置與開發 |

---

## 📁 Project Structure

```text
spacecraft-game/
│
├── tutorial4.cpp
├── tutorial4.h
│
├── 3dsloader.cpp
├── 3dsloader.h
│
├── texture.cpp
├── texture.h
│
├── tutorial4.sln
├── tutorial4.vcxproj
├── tutorial4.vcxproj.filters
│
├── fighter1.3ds
├── UFO.3DS
├── Rock.3ds
├── Rock1.3ds
├── uploads_files_4253924_Style+Sun_v1_001.3DS
├── uploads_files_5347744_Cartoon+Alien_v1_001.3DS
│
├── 1.bmp
├── 2.bmp
├── 3.bmp
├── 4.bmp
├── 5.bmp
├── 6.bmp
├── skull.bmp
├── UFONormal.bmp
├── Rock-Texture-Surface.bmp
├── photo-stone-texture-pattern.bmp
├── Cartoon Alien_v1_001_Diffuse.bmp
├── Style Sun_v1_001_Diffuse.bmp
│
├── glut32.dll
├── HighScoreFile.txt
└── .gitignore
```

### Main Files

#### `tutorial4.cpp`

遊戲主要程式，包含：

- OpenGL 初始化
- 3D 場景繪製
- Skybox
- 太空船控制
- 障礙物生成與重新生成
- 子彈系統
- 球體碰撞偵測
- Score / Health / Timer
- Particle Effect
- Win / Game Over 邏輯

#### `3dsloader.cpp / 3dsloader.h`

負責讀取 `.3ds` 模型中的：

- Vertex
- Polygon
- Texture Mapping Coordinate

並將資料載入遊戲中的 3D Object Structure。

#### `texture.cpp / texture.h`

負責讀取 BMP 圖片並轉換為 OpenGL Texture，用於：

- 3D 模型貼圖
- Skybox
- 場景材質

---

## 🚀 Getting Started

### Requirements

建議使用以下環境：

- Windows
- Visual Studio 2022
- Desktop development with C++
- OpenGL
- GLU
- GLUT

專案使用：

- Visual Studio `v143` Toolset
- `Win32` 平台
- `Debug` 組態

GLUT 開發環境需要提供：

```text
GL/glut.h
glut32.lib
glut32.dll
```

Repository 中已包含：

```text
glut32.dll
```

但 `GL/glut.h` 與 `glut32.lib` 仍需要在本機安裝或設定 GLUT 開發環境。

---

## 🔨 Build

1. Clone 或下載此 repository。

2. 使用 Visual Studio 開啟：

```text
tutorial4.sln
```

3. 選擇以下設定：

```text
Platform: Win32
Configuration: Debug
```

4. 執行：

```text
Build → Build Solution
```

5. 執行產生的程式：

```text
Debug\tutorial4.exe
```

遊戲中的 `.3ds` 與 `.bmp` 資源使用相對路徑載入，因此執行時必須確保相關模型與圖片位於程式可以讀取的工作目錄中。

如果直接執行 `Debug\tutorial4.exe` 後出現找不到圖片或模型的錯誤，可以選擇以下其中一種方式：

- 從 Visual Studio 執行，並將工作目錄設定為專案根目錄
- 將必要的 `.3ds` 與 `.bmp` 檔案複製到 `Debug` 資料夾

目前專案主要以 `Win32 / Debug` 設定進行建置與測試。

---

## 🧩 Implementation Overview

遊戲主要流程如下：

```text
Initialize OpenGL
        ↓
Load 3D Models / Textures
        ↓
Generate 80 Obstacles
        ↓
Game Loop
        ↓
Player Movement / Shooting
        ↓
Update Obstacles & Bullets
        ↓
Collision Detection
        ↓
Update Score / Health / Timer
        ↓
Render Scene & Particle Effects
        ↓
Check Win / Game Over
```

---
