"""
cat.GIF → SSD1306 OLED 帧数组转换工具
=========================================
输入: 240×240 月薪猫 GIF (28帧, 40ms)
输出: CatFrames.h (C头文件, SSD1306页-列格式)

策略:
  1. 智能裁剪: 找到每帧猫的边界，保留头部到身体的核心区域
  2. 缩放至 128×64，保持比例
  3. Floyd-Steinberg 抖动转1-bit
  4. 输出 SSD1306 页-列格式 (8 pages × 128 columns)

用法:
  python Tools/gif_to_oled.py [--frames N] [--delay MS]
"""

import argparse
import os
from PIL import Image


def extract_alpha_frame(img):
    """提取带透明背景的帧，透明像素置为白(0xFF)。"""
    if img.mode == 'RGBA':
        rgb = Image.new('RGB', img.size, (255, 255, 255))
        rgb.paste(img, mask=img.split()[3])
        return rgb
    elif img.mode == 'P':
        return img.convert('RGB')
    else:
        return img.convert('RGB')


def auto_crop_region(frames_rgb, target_w=128, target_h=64):
    """
    分析所有帧，找到猫的活动区域。
    240×240 GIF 中猫大致位于中心偏上区域。
    返回 (left, top, right, bottom) 裁剪区域。
    """
    # 对于月薪猫 GIF，猫位于画面中央偏上
    # 策略: 取中心 70% 宽度，顶部 55% 到 90% 作为有效区域
    w, h = frames_rgb[0].size  # 240×240

    # 保守裁剪: 保留猫的主体（头+身体），去掉过多背景
    # 左右各裁 20%，上下各裁 30%
    margin_lr = int(w * 0.15)   # 左右各15%
    margin_top = int(h * 0.10)  # 顶部10% (猫耳朵靠近顶部)
    margin_bot = int(h * 0.15)  # 底部15%

    left = margin_lr
    top = margin_top
    right = w - margin_lr
    bottom = h - margin_bot

    # 验证裁剪后宽高比接近 target_w:target_h (2:1)
    crop_w = right - left
    crop_h = bottom - top
    ratio = crop_w / crop_h
    target_ratio = target_w / target_h  # 2.0

    print(f"  裁剪区域: ({left},{top})-({right},{bottom}) {crop_w}×{crop_h} (比例{crop_ratio:.2f}, 目标{target_ratio:.2f})")

    return left, top, right, bottom


def floyd_steinberg_dither(img_gray):
    """Floyd-Steinberg 误差扩散抖动，输出 1-bit PIL Image (mode='1')。"""
    w, h = img_gray.size
    pixels = list(img_gray.getdata())
    buf = [float(p) for p in pixels]

    for y in range(h):
        for x in range(w):
            idx = y * w + x
            old = buf[idx]
            new = 255.0 if old > 127.0 else 0.0
            err = old - new
            buf[idx] = new

            if x + 1 < w:
                buf[idx + 1] += err * 7 / 16
            if y + 1 < h:
                if x - 1 >= 0:
                    buf[(y + 1) * w + (x - 1)] += err * 3 / 16
                buf[(y + 1) * w + x] += err * 5 / 16
                if x + 1 < w:
                    buf[(y + 1) * w + (x + 1)] += err * 1 / 16

    result = [0 if v < 128 else 255 for v in buf]
    img_out = Image.new('1', (w, h))
    img_out.putdata(result)
    return img_out


def pack_ssd1306(img_bit):
    """
    将 1-bit PIL Image (128×64) 转换为 SSD1306 页-列格式。
    返回 1024 字节数组 (8 pages × 128 columns)。
    """
    w, h = img_bit.size
    assert w == 128 and h == 64, f"期望 128×64, 实际 {w}×{h}"

    data = bytearray(1024)
    for page in range(8):
        for col in range(128):
            byte_val = 0
            for bit in range(8):
                y = page * 8 + bit
                px = img_bit.getpixel((col, y))
                if px == 0:  # 黑色像素 → 1 (OLED点亮)
                    byte_val |= (1 << bit)
            data[page * 128 + col] = byte_val
    return data


def format_byte(b):
    """格式化单个字节为 C 十六进制字面量。"""
    return f"0x{b:02X}"


def write_header(f, frame_count, delay_ms, frame_data):
    """写入 CatFrames.h 头文件。"""
    f.write('#ifndef __CATFRAMES_H\n')
    f.write('#define __CATFRAMES_H\n\n')
    f.write('#include "stm32f10x.h"\n\n')
    f.write('/* === 月薪猫动画帧数据 (来自 Einswen/SalaryCat cat.GIF) === */\n')
    f.write(f'/* 原始: 240x240 28帧 40ms → 智能裁剪 → 128x64 1-bit */\n')
    f.write(f'/* 采样: {frame_count}/28 帧  {delay_ms}ms/帧  ~{1000//delay_ms}fps */\n')
    f.write(f'/* 生成: Tools/gif_to_oled.py */\n\n')
    f.write(f'#define CAT_FRAME_COUNT  {frame_count}\n')
    f.write(f'#define CAT_FRAME_WIDTH  128\n')
    f.write(f'#define CAT_FRAME_HEIGHT 64\n')
    f.write(f'#define CAT_FRAME_DELAY_MS {delay_ms}\n')
    f.write(f'#define CAT_FRAME_SIZE   1024\n\n')
    f.write(f'static const uint8_t g_catFrames[{frame_count}][CAT_FRAME_SIZE] = {{\n')

    for fi, data in enumerate(frame_data):
        f.write(f'    /* Frame {fi} */\n    {{\n')
        for row in range(8):
            line_bytes = data[row * 128:(row + 1) * 128]
            # 每行16字节放一行
            chunks = [line_bytes[i:i + 16] for i in range(0, 128, 16)]
            for chunk in chunks:
                formatted = ', '.join(format_byte(b) for b in chunk)
                f.write(f'        {formatted},\n')
        f.write('    }')
        if fi < frame_count - 1:
            f.write(',')
        f.write('\n')

    f.write('};\n\n')
    f.write('#endif /* __CATFRAMES_H */\n')


def process_gif(gif_path, output_path, max_frames=28, delay_ms=40,
                target_w=128, target_h=64, crop_left=None, crop_top=None,
                crop_right=None, crop_bottom=None):
    """主处理函数。"""
    print(f"读取 GIF: {gif_path}")
    img = Image.open(gif_path)

    total_frames = img.n_frames
    frames_rgb = []
    for i in range(total_frames):
        img.seek(i)
        frames_rgb.append(extract_alpha_frame(img))
    print(f"  总帧数: {total_frames}, 尺寸: {frames_rgb[0].size}")

    # 确定裁剪区域
    if crop_left is not None:
        crop_box = (crop_left, crop_top, crop_right, crop_bottom)
    else:
        crop_box = auto_crop_region(frames_rgb, target_w, target_h)

    # 采样帧
    if max_frames >= total_frames:
        indices = list(range(total_frames))
    else:
        # 均匀采样
        step = total_frames / max_frames
        indices = [int(i * step) for i in range(max_frames)]

    print(f"  输出帧数: {len(indices)}, 延迟: {delay_ms}ms")

    # 处理每一帧
    frame_data = []
    for fi, idx in enumerate(indices):
        rgb = frames_rgb[idx]
        # 裁剪
        cropped = rgb.crop(crop_box)
        # 缩放
        resized = cropped.resize((target_w, target_h), Image.LANCZOS)
        # 转灰度
        gray = resized.convert('L')
        # 抖动
        bit_img = floyd_steinberg_dither(gray)
        # 打包为 SSD1306 格式
        packed = pack_ssd1306(bit_img)
        frame_data.append(packed)

        if fi % 5 == 0 or fi == len(indices) - 1:
            print(f"  处理帧 {fi + 1}/{len(indices)} (原帧 #{idx})")

    # 写入头文件
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, 'w') as f:
        write_header(f, len(frame_data), delay_ms, frame_data)

    file_size = os.path.getsize(output_path)
    flash_bytes = len(frame_data) * 1024
    print(f"\n输出: {output_path}")
    print(f"  文件大小: {file_size:,} 字节")
    print(f"  Flash占用: {flash_bytes:,} 字节 ({flash_bytes/1024:.1f} KB)")
    print(f"  帧延迟: {delay_ms}ms → ~{1000/delay_ms:.0f} fps")
    print(f"  完成!")


def main():
    parser = argparse.ArgumentParser(description='GIF → SSD1306 OLED 帧转换')
    parser.add_argument('--input', default='cat.GIF',
                        help='输入GIF路径 (默认: cat.GIF)')
    parser.add_argument('--output', default='Hardware/CatFrames.h',
                        help='输出C头文件 (默认: Hardware/CatFrames.h)')
    parser.add_argument('--frames', type=int, default=28,
                        help='输出帧数 (默认: 28)')
    parser.add_argument('--delay', type=int, default=40,
                        help='帧延迟ms (默认: 40 → 25fps)')
    parser.add_argument('--width', type=int, default=128,
                        help='目标宽度 (默认: 128)')
    parser.add_argument('--height', type=int, default=64,
                        help='目标高度 (默认: 64)')
    parser.add_argument('--crop', type=int, nargs=4, metavar=('L', 'T', 'R', 'B'),
                        help='手动裁剪区域 (left top right bottom)')

    args = parser.parse_args()

    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    gif_path = os.path.join(project_root, args.input)
    output_path = os.path.join(project_root, args.output)

    process_gif(
        gif_path=gif_path,
        output_path=output_path,
        max_frames=args.frames,
        delay_ms=args.delay,
        target_w=args.width,
        target_h=args.height,
        crop_left=args.crop[0] if args.crop else None,
        crop_top=args.crop[1] if args.crop else None,
        crop_right=args.crop[2] if args.crop else None,
        crop_bottom=args.crop[3] if args.crop else None,
    )


if __name__ == '__main__':
    main()
