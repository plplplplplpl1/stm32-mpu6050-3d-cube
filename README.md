# STM32-MPU6050-3D-Cube / 体感魔方

**EN** — A real-time 3D attitude display project based on **STM32F103C8T6**, **MPU6050** (6-axis gyroscope + accelerometer), and **OLED** (128x64). When you rotate the board, a 3D wireframe cube on the OLED rotates accordingly.

**CN** — 基于 STM32F103C8T6 和 MPU6050（六轴陀螺仪+加速度计）的 3D 姿态实时显示项目。转动开发板，OLED 上的 3D 线框魔方随之同步旋转。

---

## Features / 功能

| English | 中文 |
|---------|------|
| **Hardware I2C**（I2C1）驱动 OLED + **Software I2C** 驱动 MPU6050 | **硬件 I2C1**（PB6/PB7）驱动 OLED（100kHz）+ **软件 I2C**（PB10/PB11）驱动 MPU6050 |
| **Complementary filter** for real-time Pitch / Roll / Yaw | **互补滤波** 实时解算姿态角 |
| **31 wireframe polyhedra** — 5 Platonic, 3 Archimedean/stellated, 12 hyperbolic tilings {p,q}, 5 convex 4D polytopes (5-cell, tesseract, 16/24/600-cell), 4 Schläfli-Hess star 4D polytopes — via perspective projection on OLED 128×64 | **31种线框多面体** — 5种柏拉图立体、3种阿基米德/星形、12种双曲镶嵌{p,q}、5种凸4D正多胞体（5-cell/超立方体/16/24/600胞体）、4种Schläfli-Hess 4D星形多胞体 — 透视投影到 OLED 128×64 |
| **Hardware timer TIM2** for accurate frame time measurement | **硬件定时器 TIM2** 精确测量帧时间，姿态积分与真实时间同步 |
| **4D auto-rotation** — continuous rotation in XW and YZ planes for 4D polytopes | **4D 自动旋转** — 4D 多胞体在 XW、YZ 平面持续旋转 |
| **Auto-scale** — 4D shapes automatically sized to fit the screen | **自动缩放** — 4D 图形自动适配屏幕大小 |
| **Menu system** — 4-item main menu with sub-menu navigation | **菜单系统** — 4项主菜单 + 子菜单导航 |
| **3D mode** — 31 wireframe shapes, KEY1 toggle direction, KEY3/4 cycle shapes | **3D模式** — 31种线框图形，KEY1切换方向，KEY3/4切换图形 |
| **2D graph** — y=sin(t) / y=tan(t) real-time plotting with auto-scroll, speed multiplier | **2D绘图** — y=sin(t)/y=tan(t) 实时图像，自动滚动 + 点按加速 |
| **Rotary encoder** — single knob + button replaces 4 keys, EXTI-driven | **旋转编码器** — 单旋钮 + 按钮替代 4 按键，中断驱动 |
| **Animation** — cat + cockroach OLED animations from W25Q64 flash | **动画** — 月薪猫 + 蟑螂动画，从W25Q64实时播放 |
| **Power-on self-test** for MPU6050 connection | **上电自检**，检测 MPU6050 连接状态 |

## Hardware Requirements / 硬件需求

| Component / 组件 | Specification / 说明 |
|------------------|----------------------|
| MCU | STM32F103C8T6 |
| Sensor / 传感器 | MPU6050 (6-axis / 六轴) |
| Display / 显示屏 | 0.96" OLED 128×64 |
| Buttons / 按键 | 4x (KEY1, KEY2, KEY3-PA6, KEY4-PA4) |

## Pin Connections / 引脚连接

| STM32 | MPU6050 | OLED | Encoder |
|-------|---------|------|---------|
| PB10 | SCL | - | - |
| PB11 | SDA | - | - |
| PB6  | -   | SCL | - |
| PB7  | -   | SDA | - |
| PB0  | -   | -   | Enc A |
| PB1  | -   | -   | Enc B |
| PA7  | -   | -   | Button |
| —    | -   | -   | Enc C → GND |

> If your wiring differs, adjust the pin definitions in the source code.
> 如果接线不同，请在源码中修改引脚定义。

## Software Architecture / 软件架构

| Module / 模块 | Role (EN) | 作用 (CN) |
|---------------|-----------|-----------|
| **MPU6050** | Sensor register read/write & data acquisition via **Software I2C** (PB10/PB11) | 传感器寄存器读写与数据获取，**软件 I2C**（PB10/PB11） |
| **Attitude** | Complementary filter + gyro calibration | 互补滤波姿态解算 + 陀螺零偏标定 |
| **Cube3D** | 3D rotation matrix + perspective projection → OLED | 三维旋转矩阵 + 透视投影 → OLED 线框绘制 |
| **OLED** | SSD1306 driver via **Hardware I2C1** (PB6/PB7, 100kHz) | SSD1306 驱动，**硬件 I2C1**（PB6/PB7, 100kHz） |
| **MyI2C** | Software-bitbanged I2C protocol (for MPU6050) | 软件模拟 I2C 时序（驱动 MPU6050） |

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

**硬件：旋转编码器 + 独立按钮**

| 编码器引脚 | STM32 | 功能 |
|-----------|-------|------|
| A | PB0 (EXTI0) | 编码器 A 相 |
| B | PB1 (EXTI1) | 编码器 B 相 |
| C | GND | 公共端 |
| 按钮 | PA7 | 确认/返回 |

**操作映射（全局）：**

| 操作 | 功能 |
|------|------|
| 旋转编码器 CW/CCW | 光标移动 / 选项切换 / 图形切换 |
| 短按按钮（<500ms） | 确认选择（原 KEY1） |
| 长按按钮（≥500ms） | 返回上级（原 KEY2） |

**3D Cube Mode / 3D模式：**

进入 3D&2D → 3D 后显示线框魔方，转动开发板实时跟随。

| 操作 | Action / 功能 |
|------|--------------|
| **短按按钮** | Toggle rotation direction / 切换旋转方向 |
| **长按按钮** | Return to 3D&2D sub-menu / 返回子菜单 |
| **旋转 CW** | Next shape / 下一个图形 |
| **旋转 CCW** | Previous shape / 上一个图形 |

**2D Graph Mode / 2D绘图模式：**

进入 3D&2D → 2D → 三角函数 后选择 y=sin(t) 或 y=tan(t)。

| 操作 | Action / 功能 |
|------|--------------|
| **旋转编码器（不按按钮）** | 手动微调 t（0.05/步），停止自动滚动 |
| **按住按钮 + 转一下** | 启动自动滚动，松手继续 |
| **按住按钮 + 同向转** | 速度翻倍（×2），无上限 |
| **按住按钮 + 反向转** | 速度减半（÷2），到 0 停止 |
| **按住按钮 800ms 不转** | 退出返回 2D 子菜单 |

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

## Changelog / 更新日志

### 2026-06-09
- **Key2 引脚从 PB12 改为 PA2**（PB12 硬件故障）
- **按键消抖改为非阻塞计数方式**，不再使用 `Delay_ms` 阻塞，消除 SysTick 干扰 GPIO 读取的隐患
- **MPU6050 时钟源修复**：`PWR_MGMT_1` 从 `0x01`（X轴陀螺 PLL）改为 `0x00`（内部 8MHz RC 振荡器），启动更稳定
- **新增主菜单界面**（Menu.c），支持光标导航，预留 电子罗盘/温度监测/计步器 入口
- **KEY2 功能变更**：3D 立方体模式下按 KEY2 返回主菜单（原为重标定陀螺仪）
- **PA2 上拉显式置位**：修复标准库 `GPIO_Init` 未正确设置 PA2 ODR 的问题

### 2026-06-10
- **新增 W25Q64 NOR Flash 驱动**（软件 SPI，PB12=CS, PB13=SCK, PB14=MOSI, PB15=MISO）
- **新增月薪猫动画播放**（CatAnimation.c），28帧×1024字节存于 W25Q64，从Flash实时读取播放
- **新增串口烧录系统**（serial.c + Tools/serial_flash.py），USART1/2 配合实现 PC→STM32→Flash 烧录
- **新增镜像打包工具**（Tools/flash_image_builder.py），自动解析 C 头文件打包 w25q64_image.bin
- **新增 GIF 转 OLED 帧工具**（convert_cat.py），支持透明背景处理
- **新增中文菜单系统**（Menu.c），4 项菜单：水平仪 / 月薪猫 / 温度监测(预留) / 计步器(预留)
- **新增中文字库支持**（FontCN.c），54 个常用汉字 16×16 点阵
- **W25Q64 状态寄存器完整诊断**：SR1/SR2/SR3 三个寄存器解码（SRP/WEL/BUSY/BP 位）
- **修复 DO/DI 引脚标注问题**：模块丝印是从主设备视角标注，DO=STM32 MOSI→PB14，DI=STM32 MISO→PB15
- **MPU6050 优化**：新增 burst 连续读取模式，一次 I2C 事务读 14 字节，效率提升 10 倍
- **OLED I2C 增强**：超时保护 + 总线恢复（SWRST + 9 个 SCK 脉冲）+ 自动重试

### 2026-06-14

- **3D旋转方向统一**：交换 `Cube3D_Render` 前两个参数（PitchDeg↔RollDeg），修正物理 MPU6050 朝向与屏幕渲染的轴映射；三轴符号统一同向（Y 取正、Z 取负）
- **菜单重构**：
  - 主菜单"水平仪"→"3D&2D"（ASCII 显示），新增 `Menu_Show3D2D()` 子菜单（3D/2D选择）
  - 2D 层新增三角函数/指数函数子菜单（中文标签），KEY3/4 切换，KEY1 确认，KEY2 返回
  - 子菜单支持返回上层（循环 while 嵌套），3D 模式 KEY2 回到子菜单而非主菜单
- **旋转编码器替代 4 按键**：
  - 编码器 A(PB0)+B(PB1) 双边沿 EXTI 中断驱动，Gray 码状态机累加解码
  - 独立按钮(PA7) 状态机区分短按确认 / 长按返回（500ms）
  - `Key_GetEncRaw()` 暴露中断累加值，支持细粒度连续调节（1/4 detent = 0.05 单位）
  - 释放原 KEY1~KEY4 引脚（PA2/PA4/PA6/PB1）
- **三角函数 y=sin(t) / y=tan(t) 实时绘图**：
  - 直角坐标系：y轴(竖)+t轴(横)，原点(32,32)，10px/单位，箭头 + 字母标签(y/t)
  - **脱手自动滚动**：按住按钮 + 转一下设方向 → 松手继续自动滚动
  - **按住按钮调速**：同向转 ×2 加速（无上限），反向转 ÷2 减速（至 0 停止）
  - **长按 800ms 退出**，退出后自动等待按钮释放防连跳
  - 不按按钮旋转 → 手动微调 + 停止自动滚动
  - sin/tan 二级子菜单，共用同一套交互逻辑
  - 正切函数渐近线跳变检测（|Δy| ≥ 40px 断开），`tanf` 值钳制 ±100 防止溢出/毛刺
  - 曲线使用逐段 `OLED_DrawLine` 连线，Bresenham 消除断点
- **新增 `OLED_ShowCharBuf`**：ASCII 字符写入 OLED_GRAM 缓冲区（`|=` 保留背景），随 `OLED_Refresh` 统一刷新，不闪烁
- **新增中文字模**：三(U+4E09)、角(U+89D2)、函(U+51FD)、数(U+6570)、指(U+6307)、切(U+5207)，HZK16 行优先格式
- **蟑螂动画**（小猫从兜里掏出蟑螂 GIF → OLED 128×64 28帧）：`CockroachAnimation.c/h`，W25Q64 分区 0x01D000
- **串口烧录优化**：擦除上限→W25Q_RESERVED_ADDR，大小校验 128KB→256KB，上电自动检测改为 `#if 0` 默认关闭

### 2026-06-11

- **OLED 刷新优化**—消除摄像头横纹：
  - `OLED_Refresh()` 改为临时切水平寻址模式（`0x20,0x00`），1024 字节单次 I2C 突发传输，完成后切回页寻址模式（`0x20,0x02`）
  - 8 条固定页边界撕裂线 → 1~2 条随机位置撕裂线，相机拍摄观感大幅提升
  - 其他所有代码保持页寻址模式不变，文本/Menu/动画兼容无影响
- **新增多动画系统**（`Animation.c/h`）：
  - 菜单"月薪猫"改为"动画"，预留多 GIF 扩展入口
  - 动画切换逻辑对标水平仪 3D 图形切换：`OLED_Clear` + 直接写 GDDRAM + 400ms 延时，无黑屏闪烁
  - KEY3（PA6）下一个动画、KEY4（PA4）上一个动画、KEY2（PA2）返回菜单
  - `CatAnimation_Play()` 增加 KEY3/KEY4 检测，返回触发退出的按键码
- **字模新增与修正**：
  - 新增 `HZK_52A8`（动）、`HZK_753B`（画）— SimSun 12pt，行优先 HZK16 格式
  - 重新生成 `HZK_YUE`（月）— 修复旧字模顶部横线缺失（原为 2 像素点，现为完整横线）
  - 新增字体生成工具 `Tools/gen_font.py`，标准 HZK16 行优先格式输出
- **动画切换黑屏修复**：
  - v1 方案在动画切换时调用 `stop(清屏)→init→标签`，中途 `OLED_Clear` 造成可感知黑屏
  - v2 直接对标形状切换：`OLED_Clear` + 直接 GDDRAM 写标签 + 400ms → 进入播放，无 init/stop 开销
- **Bug 修复记录**：
  - 字模格式错误（曾误用 SSD1306 页-列格式写入 HZK16 行优先数组，导致显示乱码）
  - I2C 总线恢复函数中 I2C 时钟为 400kHz（非 CLAUDE.md 旧版所述的 100kHz）
  - 水平寻址模式与 `0xB0`/`0x00`/`0x10` 页寻址命令不兼容（SSD1306 克隆芯片差异）
