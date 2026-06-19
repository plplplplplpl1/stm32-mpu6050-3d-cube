"""
Generate 16x16 Chinese font bitmaps for SSD1306 page-column format.
Usage: python gen_font.py
"""
from PIL import Image, ImageDraw, ImageFont
import os

# Characters to generate: (Unicode codepoint, variable name suffix)
CHARS = [
    (0x52A8, "52A8"),  # 动
    (0x753B, "753B"),  # 画
]

FONT_PATH = "C:/Windows/Fonts/simsun.ttc"
FONT_SIZE = 14

try:
    font = ImageFont.truetype(FONT_PATH, FONT_SIZE)
except:
    # Fallback: try common paths
    for p in ["/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
              "C:/Windows/Fonts/simhei.ttf",
              "C:/Windows/Fonts/msyh.ttc"]:
        if os.path.exists(p):
            font = ImageFont.truetype(p, FONT_SIZE)
            break
    else:
        font = ImageFont.load_default()
        print("WARNING: using default font, results may be poor")

for codepoint, name in CHARS:
    char = chr(codepoint)

    # Render to 16x16 image
    img = Image.new('1', (16, 16), 0)
    draw = ImageDraw.Draw(img)
    draw.text((0, -1), char, font=font, fill=1)

    # Convert to SSD1306 page-column format
    # SSD1306: 8 pages * 16 columns for 16x16 char
    # page 0 = top 8 rows, page 1 = bottom 8 rows
    # Each page byte: bit 0 = top row of page, bit 7 = bottom row of page
    result = bytearray(32)

    for row in range(16):
        page = row // 8
        bit_in_page = row % 8
        for col in range(16):
            if img.getpixel((col, row)):
                result[page * 16 + col] |= (1 << bit_in_page)

    print(f'/* {char} U+{codepoint:04X} */')
    print(f'const uint8_t HZK_{name}[32] = {{')
    for i in range(2):
        line = '    '
        for j in range(16):
            line += f'0x{result[i * 16 + j]:02X}, '
        print(line)
    print('};')
    print()
