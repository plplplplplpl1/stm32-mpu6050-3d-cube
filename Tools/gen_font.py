"""
Generate 16x16 Chinese font bitmaps in HZK16 row-major format.
Usage: python gen_font.py
"""
from PIL import Image, ImageDraw, ImageFont
import os

# Characters to generate: (Unicode codepoint, variable name suffix)
CHARS = [
    (0x53CD, "53CD"),  # 反
]

FONT_PATH = "C:/Windows/Fonts/simsun.ttc"
FONT_SIZE = 14

try:
    font = ImageFont.truetype(FONT_PATH, FONT_SIZE)
except:
    for p in ["C:/Windows/Fonts/simhei.ttf",
              "C:/Windows/Fonts/msyh.ttc",
              "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc"]:
        if os.path.exists(p):
            font = ImageFont.truetype(p, FONT_SIZE)
            break
    else:
        font = ImageFont.load_default()
        print("WARNING: using default font, results may be poor")

for codepoint, name in CHARS:
    char = chr(codepoint)

    img = Image.new('1', (16, 16), 0)
    draw = ImageDraw.Draw(img)
    draw.text((0, -1), char, font=font, fill=1)

    # HZK16 row-major: 16 rows, each row = 2 bytes (MSB = leftmost pixel)
    result = bytearray(32)

    for row in range(16):
        byte0 = 0  # cols 0-7
        byte1 = 0  # cols 8-15
        for col in range(8):
            if img.getpixel((col, row)):
                byte0 |= (0x80 >> col)
        for col in range(8, 16):
            if img.getpixel((col, row)):
                byte1 |= (0x80 >> (col - 8))
        result[row * 2] = byte0
        result[row * 2 + 1] = byte1

    print(f'/* {char} U+{codepoint:04X} */')
    print(f'const uint8_t HZK_{name}[32] = {{')
    for i in range(16):
        s = f'    0x{result[i*2]:02X}, 0x{result[i*2+1]:02X}'
        print(s + ',  /* row ' + str(i) + ' */')
    print('};')
    print()
