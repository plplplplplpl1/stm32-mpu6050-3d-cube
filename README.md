# STM32-MPU6050-3D-Cube

A real-time 3D attitude display project based on **STM32F103C8T6**, **MPU6050** (6-axis gyroscope + accelerometer), and **OLED** (128x64).

When you rotate the board, a 3D wireframe cube on the OLED rotates accordingly — like a somatosensory Rubik's cube.

## Features

- **Software I2C** bit-banging to drive MPU6050 (raw acceleration & angular velocity)
- **Complementary filter** fusing gyroscope and accelerometer data for real-time Pitch / Roll / Yaw estimation
- **3D wireframe cube** rendered via perspective projection on a 128×64 OLED
- **Hardware timer TIM2** for accurate frame time measurement — ensures attitude integration stays synchronized with real time
- **Button controls**:
  - KEY1 — toggle rotation direction (normal / reverse)
  - KEY2 — recalibrate gyroscope zero-bias
- **Power-on self-test** — checks MPU6050 connection and displays status

## Hardware Requirements

| Component       | Specification       |
|-----------------|---------------------|
| MCU             | STM32F103C8T6       |
| Sensor          | MPU6050 (6-axis)    |
| Display         | 0.96" OLED 128×64   |
| Buttons         | 2x (KEY1, KEY2)     |
| LED             | 1x (status)         |

## Pin Connections

| STM32 | MPU6050 | OLED  |
|-------|---------|-------|
| PB10  | SCL     | -     |
| PB11  | SDA     | -     |
| PB3   | -       | SCL   |
| PB5   | -       | SDA   |

> Note: Adjust pins in the source code if your wiring differs.

## Software Architecture

| Module      | Role |
|-------------|------|
| **MyI2C**   | Software-bitbanged I2C protocol (GPIO toggling) |
| **MPU6050** | Sensor register read/write and data acquisition |
| **Attitude**| Complementary filter attitude estimation + gyro zero-bias calibration |
| **Cube3D**  | 3D rotation matrix + perspective projection → OLED wireframe drawing |
| **OLED**    | SSD1306 driver (with Chinese character font table) |

## Usage

### 1. Build & Flash

Open the project with **Keil uVision** (`Project.uvprojx`), build, and flash to the STM32F103C8T6 board.

### 2. Power On

Once powered, the OLED will show:

```
MPU ID:68
MPU OK
```

If the display shows `MPU ERR / CHECK WIRE`, check the MPU6050 wiring.

### 3. Auto Calibration

After the self-test, the board enters automatic gyroscope calibration. **Keep the board stationary** on a flat surface during this step. The OLED shows:

```
校准中...
```

Calibration takes about 2 seconds (300 samples). Once finished, it displays `完成` and the cube appears.

### 4. Normal Operation

Rotate the board — the 3D cube on the OLED rotates in real time, matching the physical orientation.

### 5. Button Controls

| Button | Action | OLED Feedback |
|--------|--------|---------------|
| **KEY1** | Toggle rotation direction | Shows `方向:NORMAL / REVERSE` for 400ms |
| **KEY2** | Recalibrate gyroscope | Shows 校准中... → 完成 |

> Recalibrate whenever you notice drift. Place the board flat and still during calibration.

## Key Algorithms

- **Complementary Filter**: `alpha = 0.999` — high-rate gyro integration corrected by low-passed accelerometer data
- **Zero-bias Calibration**: averages 300 stationary samples to cancel gyro offset and temperature drift
- **3D Rotation**: standard right-handed rotation matrices (X → Y → Z)
- **Perspective Projection**: camera-distance-based scaling with automatic centroid centering
