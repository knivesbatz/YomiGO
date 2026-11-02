#!/usr/bin/env python3
"""Bake DotGothic16 into 1-bit 16px glyphs, so the apps never depend on the OS having Japanese fonts."""
import io, os
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONT = ImageFont.truetype(os.path.join(ROOT, "assets", "fonts", "DotGothic16-Regular.ttf"), 16)

def glyph(cp):
    im = Image.new("L", (16, 16), 0)
    ImageDraw.Draw(im).text((0, 14), chr(cp), font=FONT, fill=255, anchor="ls")
    wide = FONT.getlength(chr(cp)) > 8
    rows = []
    for y in range(16):
        v = 0
        for x in range(16):
            if im.getpixel((x, y)) >= 128: v |= 0x8000 >> x
        rows.append(v)
    return rows, wide

def charset():
    s = set(range(0x20, 0x7F)) | {0xE9, 0xD7, 0x301C}
    for hi in list(range(0x81, 0xA0)) + list(range(0xE0, 0xF0)):
        for lo in list(range(0x40, 0x7F)) + list(range(0x80, 0xFD)):
            try: s.add(ord(bytes([hi, lo]).decode("cp932")))
            except (UnicodeDecodeError, TypeError): pass
    return sorted(c for c in s if c < 0x10000 and not 0xE000 <= c <= 0xF8FF)

def main():
    blank, _ = glyph(0x20)
    tofu, _ = glyph(0xFFFF)
    cps, wid, bits = [], [], []
    for cp in charset():
        rows, wide = glyph(cp)
        if rows == tofu or (rows == blank and cp not in (0x20, 0x3000)): continue
        cps.append(cp); wid.append(16 if wide else 8); bits.append(rows)
    gen = os.path.join(ROOT, "build", "gen")
    os.makedirs(gen, exist_ok=True)
    with io.open(os.path.join(gen, "font_gen.h"), "w", newline="\n") as f:
        f.write(f"#define YG_NGLYPHS {len(cps)}\nstatic const unsigned short YG_GLYPH_CP[]={{")
        f.write(",".join(map(str, cps)))
        f.write("};\nstatic const unsigned char YG_GLYPH_W[]={" + ",".join(map(str, wid)))
        f.write("};\nstatic const unsigned short YG_GLYPH_BITS[]={")
        f.write(",".join(str(v) for g in bits for v in g))
        f.write("};\n")
    print(f"font ok: {len(cps)} glyphs")

if __name__ == "__main__":
    main()
