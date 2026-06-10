#!/usr/bin/env python3
"""
W25Q64 串口烧录工具
用法: 按住 KEY4 → 给 STM32 上电/复位 → OLED 显示 "FLASH BURN Waiting PC..."
      → 运行: python serial_flash.py COM5

协议:
  STM32 反复发 "READY"
  PC 收到后发 hex_size
  STM32 回 "OK" → 擦除 → 发 "GO"
  PC 发 raw binary
  STM32 回 "DONE"
"""

import serial
import serial.tools.list_ports
import sys
import os
import time
from pathlib import Path

def list_ports():
    print("COM ports:")
    for p in serial.tools.list_ports.comports():
        print(f"  {p.device} - {p.description}")

def flash_image(port_name, image_path):
    image = Path(image_path).read_bytes()
    size = len(image)
    size_hex = f"{size:08X}"
    print(f"Image: {image_path} ({size} bytes = {size/1024:.1f} KB)")

    ser = serial.Serial(port_name, 115200, timeout=5)
    print(f"Port {port_name} opened, waiting for STM32 READY...")

    # Wait for "READY" from STM32
    while True:
        line = ser.readline()
        if b"READY" in line:
            print("Got READY, sending size...")
            break
        if line:
            print(f"  (ignored: {line.strip()})")

    # Send image size
    ser.write(f"{size_hex}\n".encode())
    ser.flush()

    # Wait for "OK"
    resp = ser.readline().strip()
    print(f"STM32: {resp}")
    if resp != b"OK":
        print("ERROR: STM32 rejected size")
        ser.close()
        return False

    # Wait for "GO" (erase done)
    resp = ser.readline().strip()
    print(f"STM32: {resp}")
    if resp != b"GO":
        print("ERROR: erase may have failed")
        ser.close()
        return False

    # Small delay for STM32 to enter receive loop
    time.sleep(0.3)

    # Send data in 4KB chunks, wait for ACK between chunks
    chunk = 4096
    offset = 0
    while offset < size:
        end = min(offset + chunk, size)
        ser.write(image[offset:end])
        ser.flush()

        # Wait for ACK
        resp = ser.readline().strip()
        if not resp:
            time.sleep(0.2)
            resp = ser.readline().strip()

        pct = min(offset * 100 // size, 99)
        print(f"  [{pct:3d}%] {resp}")
        if resp == b'DONE':
            break

        offset = end

    # Wait for final DONE
    ser.timeout = 2
    resp = ser.readline().strip()
    ser.close()

    if b"DONE" in resp or offset >= size:
        print(f"\n[OK] Flash programming complete! ({offset}/{size} bytes)")
        print("Reset STM32 to boot normally.")
        return True
    else:
        print(f"\n[WARN] {offset}/{size} bytes sent. Check OLED.")
        if resp:
            print(f"  Last response: {resp}")
        return False

if __name__ == "__main__":
    if len(sys.argv) < 2 or sys.argv[1] == "--list":
        list_ports()
        sys.exit(0)

    port = sys.argv[1]
    if len(sys.argv) >= 3:
        image = sys.argv[2]
    else:
        script_dir = Path(__file__).resolve().parent
        image = script_dir.parent / "w25q64_image.bin"
        if not image.exists():
            image = script_dir / "w25q64_image.bin"

    if not Path(image).exists():
        print(f"ERROR: Image not found: {image}")
        sys.exit(1)

    flash_image(port, str(image))
