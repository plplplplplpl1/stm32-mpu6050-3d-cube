"""
Convert cat.gif (240x240, 28 frames, 40ms/frame) to SSD1306 OLED format.
- Scale proportionally to fit 128x64 without cropping
- Output: all 28 frames, 40ms delay, 1-bit packed for SSD1306 page-column
"""
from PIL import Image
import os

GIF_PATH = r"D:\cat.gif"
OLED_W = 128
OLED_H = 64
MARGIN = 0.88       # 猫图占屏幕比例（56×56，比之前53×53稍大）
THRESHOLD = 128     # 1-bit threshold

img = Image.open(GIF_PATH)
total_frames = img.n_frames
print(f"Input: {img.size}, {total_frames} frames")

# Calculate proportional scale: fit within OLED, keep aspect ratio, apply margin
src_w, src_h = img.size
scale = min(OLED_W / src_w, OLED_H / src_h) * MARGIN
new_w = int(src_w * scale)
new_h = int(src_h * scale)
offset_x = (OLED_W - new_w) // 2
offset_y = (OLED_H - new_h) // 2
print(f"Scale: {scale:.4f}, new size: {new_w}x{new_h}, offset: ({offset_x}, {offset_y})")

# 全部28帧，step=1，完整动画周期
FRAME_STEP = 1
MAX_FRAMES = 28
FRAME_DELAY_MS = 30
sampled_indices = list(range(0, min(total_frames, MAX_FRAMES * FRAME_STEP), FRAME_STEP))[:MAX_FRAMES]
print(f"Sampling {len(sampled_indices)}/{total_frames} frames (step={FRAME_STEP})")

# Extract sampled frames as 1-bit packed data
all_frames = []
for i in sampled_indices:
    img.seek(i)
    # Convert to RGBA first, then create a white background to handle transparency
    frame = img.convert("RGBA")
    # Create white background
    bg = Image.new("RGBA", frame.size, (255, 255, 255, 255))
    # Composite frame onto white background (handles transparency)
    composite = Image.alpha_composite(bg, frame).convert("L")

    # Scale proportionally
    composite = composite.resize((new_w, new_h), Image.LANCZOS)

    # Create OLED canvas (1=white=off, 0=black=on for SSD1306)
    canvas = Image.new("1", (OLED_W, OLED_H), 1)  # white background → OLED off
    # Paste the scaled image centered
    canvas.paste(composite, (offset_x, offset_y))

    # Convert to SSD1306 page-column format
    # 8 pages (vertical bytes), each page has 128 bytes (horizontal)
    frame_data = bytearray(OLED_W * OLED_H // 8)
    for y in range(OLED_H):
        page = y // 8
        bit = y % 8
        for x in range(OLED_W):
            pixel = canvas.getpixel((x, y))
            if pixel == 0:  # black pixel = lit on OLED (SSD1306: 1=on, 0=off with normal mode)
                frame_data[page * OLED_W + x] |= (1 << bit)

    all_frames.append(bytes(frame_data))

# Generate C header file
output_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Hardware", "CatFrames.h")
with open(output_path, "w", encoding="utf-8") as f:
    f.write("#ifndef __CATFRAMES_H\n")
    f.write("#define __CATFRAMES_H\n\n")
    f.write("#include \"stm32f10x.h\"\n\n")
    f.write("/* === 月薪猫动画帧数据 (来自 Einswen/SalaryCat cat.GIF) === */\n")
    f.write(f"/* 原始: 240x240 {total_frames}帧 → 等比例缩放适配 {OLED_W}x{OLED_H} OLED，不裁剪 */\n")
    f.write(f"/* 缩放比例: 1:{1/scale:.2f}, 图像区域 {new_w}x{new_h} 居中，采样{len(sampled_indices)}帧(步长{FRAME_STEP})，{FRAME_DELAY_MS}ms/帧 */\n\n")
    f.write(f"#define CAT_FRAME_COUNT  {len(sampled_indices)}\n")
    f.write(f"#define CAT_FRAME_WIDTH  {OLED_W}\n")
    f.write(f"#define CAT_FRAME_HEIGHT {OLED_H}\n")
    f.write(f"#define CAT_FRAME_SIZE   {OLED_W * OLED_H // 8}\n")
    f.write(f"#define CAT_FRAME_DELAY_MS {FRAME_DELAY_MS}\n\n")
    f.write(f"static const uint8_t g_catFrames[{len(sampled_indices)}][CAT_FRAME_SIZE] = {{\n")

    for i, data in enumerate(all_frames):
        f.write(f"    /* Frame {i} */\n    {{\n")
        # Write 16 bytes per line
        for offset in range(0, len(data), 16):
            chunk = data[offset:offset+16]
            hex_str = ", ".join(f"0x{b:02X}" for b in chunk)
            f.write(f"        {hex_str},\n")
        f.write("    },\n")

    f.write("};\n\n")
    f.write("#endif /* __CATFRAMES_H */\n")

print(f"\nGenerated: {output_path}")
print(f"Frames: {len(all_frames)}, each {OLED_W * OLED_H // 8} bytes")
print(f"Frame delay: 40ms (25 FPS)")
