# STM32-MPU6050-3D-Cube / 体感魔方

**EN** — A real-time 3D attitude display project based on **STM32F103C8T6**, **MPU6050** (6-axis gyroscope + accelerometer), and **OLED** (128x64). When you rotate the board, a 3D wireframe cube on the OLED rotates accordingly.

**CN** — 基于 STM32F103C8T6 和 MPU6050（六轴陀螺仪+加速度计）的 3D 姿态实时显示项目。转动开发板，OLED 上的 3D 线框魔方随之同步旋转。

---

## Features / 功能

| English | 中文 |
|---------|------|
| **Software I2C** bit-banging to drive MPU6050 | **软件 I2C** 驱动 MPU6050，读取原始加速度和角速度 |
| **Complementary filter** for real-time Pitch / Roll / Yaw | **互补滤波** 实时解算姿态角 |
| **31 wireframe polyhedra** — 5 Platonic, 3 Archimedean/stellated, 12 hyperbolic tilings {p,q}, 5 convex 4D polytopes (5-cell, tesseract, 16/24/600-cell), 4 Schläfli-Hess star 4D polytopes — via perspective projection on OLED 128×64 | **31种线框多面体** — 5种柏拉图立体、3种阿基米德/星形、12种双曲镶嵌{p,q}、5种凸4D正多胞体（5-cell/超立方体/16/24/600胞体）、4种Schläfli-Hess 4D星形多胞体 — 透视投影到 OLED 128×64 |
| **Hardware timer TIM2** for accurate frame time measurement | **硬件定时器 TIM2** 精确测量帧时间，姿态积分与真实时间同步 |
| **4D auto-rotation** — continuous rotation in XW and YZ planes for 4D polytopes | **4D 自动旋转** — 4D 多胞体在 XW、YZ 平面持续旋转 |
| **Auto-scale** — 4D shapes automatically sized to fit the screen | **自动缩放** — 4D 图形自动适配屏幕大小 |
| **KEY1** — toggle rotation direction (normal / reverse) | **KEY1** — 切换旋转方向（正向/反向） |
| **KEY2** — recalibrate gyroscope zero-bias | **KEY2** — 陀螺仪零偏重标定 |
| **KEY3 (PA6)** — cycle shapes forward (up) | **KEY3 (PA6)** — 向上切换显示图形 |
| **KEY4 (PA4)** — cycle shapes backward (down) | **KEY4 (PA4)** — 向下切换显示图形 |
| **Power-on self-test** for MPU6050 connection | **上电自检**，检测 MPU6050 连接状态 |

## Hardware Requirements / 硬件需求

| Component / 组件 | Specification / 说明 |
|------------------|----------------------|
| MCU | STM32F103C8T6 |
| Sensor / 传感器 | MPU6050 (6-axis / 六轴) |
| Display / 显示屏 | 0.96" OLED 128×64 |
| Buttons / 按键 | 4x (KEY1, KEY2, KEY3-PA6, KEY4-PA4) |

## Pin Connections / 引脚连接

| STM32 | MPU6050 | OLED | Button |
|-------|---------|------|--------|
| PB10 | SCL | - | - |
| PB11 | SDA | - | - |
| PB8  | -   | SCL | - |
| PB9  | -   | SDA | - |
| PB1  | -   | -   | KEY1 |
| PB12 | -   | -   | KEY2 |
| PA6  | -   | -   | KEY3 |
| PA4  | -   | -   | KEY4 |

> If your wiring differs, adjust the pin definitions in the source code.
> 如果接线不同，请在源码中修改引脚定义。

## Software Architecture / 软件架构

| Module / 模块 | Role (EN) | 作用 (CN) |
|---------------|-----------|-----------|
| **MyI2C** | Software-bitbanged I2C protocol | 软件模拟 I2C 时序 |
| **MPU6050** | Sensor register read/write & data acquisition | 传感器寄存器读写与数据获取 |
| **Attitude** | Complementary filter + gyro calibration | 互补滤波姿态解算 + 陀螺零偏标定 |
| **Cube3D** | 3D rotation matrix + perspective projection → OLED | 三维旋转矩阵 + 透视投影 → OLED 线框绘制 |
| **OLED** | SSD1306 driver (with CJK font table) | SSD1306 驱动（含汉字字模） |

## Usage / 使用方法

### 1. Build & Flash / 编译与下载

Open the project with **Keil uVision** (`Project.uvprojx`), build, and flash to the STM32F103C8T6 board.
使用 **Keil uVision** 打开 `Project.uvprojx`，编译并下载到 STM32F103C8T6 开发板。

### 2. Power On / 上电启动

Once powered, the board first auto-calibrates the gyroscope (~2 seconds, 300 samples). **Keep the board stationary and flat.**
上电后先自动进行陀螺仪零偏标定（约2秒、采样300次）。**请将板子水平静止放置。**

Then the OLED shows the MPU6050 ID:
然后 OLED 显示 MPU6050 的 ID：

```
MPU ID:68
MPU OK
```

If you see `MPU ERR / CHECK WIRE`, check the wiring between STM32 and MPU6050.
如果显示 `MPU ERR / CHECK WIRE`，请检查 MPU6050 的接线。

### 3. Normal Operation / 正常运行

After self-test, the 3D cube appears on the OLED. Rotate the board — the cube follows in real time.
自检通过后，OLED 显示 3D 魔方。转动开发板，魔方实时同步旋转。

### 4. Button Controls / 按键操作

| Button / 按键 | Action / 功能 | OLED Feedback / 反馈 |
|--------------|--------------|---------------------|
| **KEY1** | Toggle direction / 切换方向 | Shows / 显示 `方向:NORMAL / REVERSE`（400ms） |
| **KEY2** | Recalibrate gyro / 重标定陀螺仪 | Shows / 显示 `校准中...` → `完成` |
| **KEY3 (PA6)** | Cycle shape up / 向上切换图形 | Shows shape label (400ms) / 显示图形名称（400ms） |
| **KEY4 (PA4)** | Cycle shape down / 向下切换图形 | Shows shape label (400ms) / 显示图形名称（400ms） |

> Recalibrate whenever you notice drift. Place the board flat and still during calibration.
> 发现漂移时可随时按 KEY2 重新标定，标定时请保持板子静止水平。

## Key Algorithms / 关键算法

| Algorithm / 算法 | Description (EN) | 说明 (CN) |
|------------------|-------------------|-----------|
| **Complementary Filter** | `alpha = 0.999` — high-rate gyro integration corrected by low-passed accelerometer data | 高置信度陀螺积分 + 加速度计低频修正 |
| **Zero-bias Calibration** | Average 300 stationary samples to cancel gyro offset and drift | 静止采样 300 次取平均，消除零偏与温漂 |
| **3D Rotation** | Right-handed rotation matrices (X → Y → Z) | 右手系标准旋转矩阵依次绕 XYZ 轴旋转 |
| **Perspective Projection** | Camera-distance-based scaling with centroid auto-centering | 基于相机距离的缩放投影，重心自动居中 |
| **4D→3D→2D Pipeline** | 4D rotation (XW+YZ), perspective project to 3D, 3D rotate, then project to screen with auto-scale | 4D旋转、透视投影到3D、3D旋转、再投影到屏幕，自动缩放适配 |
| **4D Auto-rotation** | XW plane at 15°/s, YZ plane at 10°/s, independent of MPU rotation | XW平面15°/s、YZ平面10°/s持续自动旋转，与MPU姿态角相互独立 |
| **Hyperbolic Tessellation {p,q}** | Regular tiling of the hyperbolic plane mapped via stereographic projection to 3D | 双曲正多边形镶嵌{p,q}，经球极投影映射到3D空间 |
