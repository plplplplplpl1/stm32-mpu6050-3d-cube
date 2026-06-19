#!/usr/bin/env python3
"""
W25Q64 镜像打包工具
读取项目中的 C 数据文件，提取 const 数组，打包成二进制镜像。
镜像通过串口一次性烧入 W25Q64。

用法: python flash_image_builder.py [--project-dir <路径>]
输出: w25q64_image.bin
"""

import re
import struct
import os
import sys
from pathlib import Path

# ── 分区地址 (与 Hardware/W25Q64_Layout.h 保持一致) ──
ADDR_CATFRAMES      = 0x000000
ADDR_COCKROACHFRAMES = 0x01D000
ADDR_SHAPES         = 0x007000
ADDR_FONT_CN        = 0x017000
ADDR_FONT_ASCII     = 0x01B000
ADDR_CALIB          = 0x01C000
IMAGE_SIZE          = 0x024000  # 144KB

# ── 工具函数 ────────────────────────────────────

def parse_hex_bytes(text):
    """从 C 源码文本中提取所有 0xNN 字节"""
    return bytes(int(b, 16) for b in re.findall(r'0x([0-9A-Fa-f]{2})', text))

def parse_float_literals(text):
    """从 C 源码文本中提取所有浮点数 (如 -0.79056942f)"""
    values = []
    for m in re.finditer(r'([+-]?\d+\.\d+)f', text):
        values.append(float(m.group(1)))
    return values

def parse_uint8_braces(text):
    """解析 {0, 1}, {2, 3} 格式的 uint8 对，返回所有数字"""
    return [int(x) for x in re.findall(r'\b(\d+)\b', text)]

# ── CatFrames 解析 ──────────────────────────────

def extract_catframes(hw_dir):
    """从 CatFrames.h 提取 28 帧 × 1024 字节"""
    path = hw_dir / "CatFrames.h"
    text = path.read_text(encoding='utf-8', errors='replace')

    # 提取帧数
    count_m = re.search(r'#define\s+CAT_FRAME_COUNT\s+(\d+)', text)
    frame_count = int(count_m.group(1)) if count_m else 28

    # 提取整个数组的十六进制字节
    all_bytes = parse_hex_bytes(text)
    frame_size = 1024
    expected = frame_count * frame_size

    if len(all_bytes) < expected:
        print(f"  WARN CatFrames: 期望 {expected}B, 实际只有 {len(all_bytes)}B")
    else:
        all_bytes = all_bytes[:expected]

    print(f"  CatFrames: {frame_count} 帧 × {frame_size}B = {len(all_bytes)}B")
    return all_bytes

# ── CockroachFrames 解析 ──────────────────────────

def extract_cockroachframes(hw_dir):
    """从 CockroachFrames.h 提取 28 帧 × 1024 字节"""
    path = hw_dir / "CockroachFrames.h"
    if not path.exists():
        print("  CockroachFrames: 未找到, 跳过")
        return b''

    text = path.read_text(encoding='utf-8', errors='replace')
    count_m = re.search(r'#define\s+COCKROACH_FRAME_COUNT\s+(\d+)', text)
    frame_count = int(count_m.group(1)) if count_m else 28

    all_bytes = parse_hex_bytes(text)
    frame_size = 1024
    expected = frame_count * frame_size

    if len(all_bytes) < expected:
        print(f"  WARN CockroachFrames: 期望 {expected}B, 实际只有 {len(all_bytes)}B")
    else:
        all_bytes = all_bytes[:expected]

    print(f"  CockroachFrames: {frame_count} 帧 × {frame_size}B = {len(all_bytes)}B")
    return all_bytes

# ── 图形数据解析 ────────────────────────────────

def extract_shapes(hw_dir):
    """
    从 Cube3D_4D.h 和 Cube3D_Hyperbolic.h 提取图形数据。
    二进制格式:
      [2B: total_shapes] [shapes×4B: offset_table]
      每个 shape:
        [1B: is_4d] [1B: vtx_count] [2B: edge_count]
        [vertices: vtx_count × (is_4d ? 16 : 12)]
        [edges: edge_count × 2]
    """
    records = []

    for fname in ["Cube3D_4D.h", "Cube3D_Hyperbolic.h"]:
        path = hw_dir / fname
        if not path.exists():
            continue
        text = path.read_text(encoding='utf-8', errors='replace')

        # 找每个 shape 的定义块
        # 匹配: #define XXX_VTX_COUNT  N  ... static const Vec{3,4}f_t kXXXVertices[...] = {...}; ... static const uint8_t kXXXEdges[...][2] = {...};
        shapes = re.split(r'(?=/\*.*?\*/\s*\n\s*/\*|^/\*)', text, flags=re.M)

        for block in shapes:
            # 提取顶点数
            vtx_m = re.search(r'_VTX_COUNT\s+(\d+)', block)
            edge_m = re.search(r'_EDGE_COUNT\s+(\d+)', block)
            if not vtx_m or not edge_m:
                continue
            vtx_count = int(vtx_m.group(1))
            edge_count = int(edge_m.group(1))
            is_4d = 'Vec4f_t' in block

            # 提取顶点数据
            vtx_text = re.search(r'(?:Vec4f_t|Vec3f_t)\s+\w+\[\w+\]\s*=\s*\{(.*?)\};', block, re.S)
            edges_text = re.search(r'uint8_t\s+\w+\[\w+\]\[\d+\]\s*=\s*\{(.*?)\};', block, re.S)

            if not vtx_text or not edges_text:
                continue

            # 解析浮点数
            floats = parse_float_literals(vtx_text.group(1))
            edge_nums = parse_uint8_braces(edges_text.group(1))

            expected_vtx_floats = vtx_count * (4 if is_4d else 3)
            expected_edge_nums = edge_count * 2

            if len(floats) < expected_vtx_floats:
                print(f"  WARN shape: 期望 {expected_vtx_floats} floats, 实际 {len(floats)}")
                continue
            if len(edge_nums) < expected_edge_nums:
                print(f"  WARN shape: 期望 {expected_edge_nums} edges, 实际 {len(edge_nums)}")
                continue

            floats = floats[:expected_vtx_floats]
            edge_nums = edge_nums[:expected_edge_nums]

            # 构建二进制记录
            record = bytearray()
            record.append(1 if is_4d else 0)
            record.append(vtx_count & 0xFF)
            record.extend(struct.pack('<H', edge_count))

            for f in floats:
                record.extend(struct.pack('<f', f))
            for e in edge_nums:
                record.append(e & 0xFF)

            records.append(bytes(record))

    total = sum(len(r) for r in records)
    print(f"  Shapes: {len(records)} 个图形, 共 {total}B")

    # 构建带偏移表的数据
    header = struct.pack('<H', len(records))
    offset = 2 + len(records) * 4  # header + offset table
    for r in records:
        header += struct.pack('<I', offset)
        offset += len(r)

    return header + b''.join(records)

# ── 汉字字库解析 ────────────────────────────────

def extract_font_cn(hw_dir):
    """从 FontCN.c 提取所有 32B 汉字点阵，按声明顺序排列"""
    path = hw_dir / "FontCN.c"
    text = path.read_text(encoding='utf-8', errors='replace')

    # 匹配每个 const uint8_t HZK_XXXX[32] = {...};
    chars = re.findall(
        r'const\s+uint8_t\s+HZK_\w+\[32\]\s*=\s*\{(.*?)\};',
        text, re.S
    )

    char_data = bytearray()
    for c in chars:
        b = parse_hex_bytes(c)
        if len(b) >= 32:
            char_data.extend(b[:32])
        else:
            # 补齐
            char_data.extend(b + b'\x00' * (32 - len(b)))

    count = len(char_data) // 32
    result = struct.pack('<H', count) + bytes(char_data)
    print(f"  FontCN: {count} 个汉字, {len(char_data)}B")
    return result

# ── ASCII 字库解析 ──────────────────────────────

def extract_font_ascii(hw_dir):
    """从 OLED_Font.h 提取 8x16 ASCII 字库"""
    path = hw_dir / "OLED_Font.h"
    text = path.read_text(encoding='utf-8', errors='replace')

    # 找 const uint8_t OLED_F8x16[...][16] = {...};
    match = re.search(r'(?:const\s+)?uint8_t\s+OLED_F8x16\[\]\[\d+\]\s*=\s*\{(.*?)\};', text, re.S)
    if not match:
        print("  WARN 未找到 OLED_F8x16 数组")
        return b''

    all_bytes = parse_hex_bytes(match.group(1))
    print(f"  FontASCII: {len(all_bytes)}B ({len(all_bytes)//16} 字符)")
    return bytes(all_bytes)

# ── 镜像组装 ────────────────────────────────────

def build_image(hw_dir, output_path):
    """组装完整镜像"""
    image = bytearray(IMAGE_SIZE)
    for i in range(IMAGE_SIZE):
        image[i] = 0xFF  # 空白区填 0xFF (NOR Flash 擦除态)

    def write_at(addr, data):
        if addr + len(data) > IMAGE_SIZE:
            print(f"  WARN 地址 0x{addr:06X} 写入 {len(data)}B 越界!")
            return
        image[addr:addr+len(data)] = data
        print(f"  写入 0x{addr:06X}: {len(data)}B")

    print("解析数据文件...")

    # CatFrames
    cat = extract_catframes(hw_dir)
    write_at(ADDR_CATFRAMES, cat)

    # CockroachFrames
    cockroach = extract_cockroachframes(hw_dir)
    if cockroach:
        write_at(ADDR_COCKROACHFRAMES, cockroach)

    # Shapes
    shapes = extract_shapes(hw_dir)
    write_at(ADDR_SHAPES, shapes)

    # FontCN
    font_cn = extract_font_cn(hw_dir)
    write_at(ADDR_FONT_CN, font_cn)

    # FontASCII
    font_ascii = extract_font_ascii(hw_dir)
    write_at(ADDR_FONT_ASCII, font_ascii)

    # 校准区保持 0xFF (未写入状态)

    # 写入文件
    output_path.write_bytes(bytes(image))
    size_kb = len(image) / 1024
    print(f"\n[OK] 镜像已生成: {output_path} ({size_kb:.1f} KB)")

# ── main ────────────────────────────────────────

if __name__ == "__main__":
    script_dir = Path(__file__).resolve().parent
    project_dir = script_dir.parent

    if "--project-dir" in sys.argv:
        idx = sys.argv.index("--project-dir")
        project_dir = Path(sys.argv[idx + 1])

    hw_dir = project_dir / "Hardware"
    output = project_dir / "w25q64_image.bin"

    build_image(hw_dir, output)
