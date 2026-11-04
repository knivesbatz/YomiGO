#!/usr/bin/env python3
"""Makes YomiGo's icon, logo and banner in the macOS Big Sur icon style, and the README screenshots.
Writes: assets/brand/{icon,logo,banner,social-preview}.png, desktop/icons/icon.{png,ico,icns}, the web UI's icon.png,
the extension's icon128.png, and the Android adaptive icon. usage: python tools/make_brand.py [--shots]"""
import math, os, subprocess, sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageChops

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "assets", "brand")
WWW = os.path.join(ROOT, "mobile", "app", "src", "main", "assets", "www")
RES = os.path.join(ROOT, "mobile", "app", "src", "main", "res")
JP = "C:/Windows/Fonts/YuGothB.ttc"
INTER = os.path.join(WWW, "fonts", "InterVariable.woff2")
TOP, BOTTOM, INK, CARD, PETAL, PETAL_DEEP = (255, 143, 192), (226, 46, 112), (74, 22, 46), (255, 251, 245), (255, 183, 209), (240, 98, 150)

def lerp(a, b, t): return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))

def gradient(w, h, a, b):
    g = Image.new("RGB", (1, h))
    for y in range(h): g.putpixel((0, y), lerp(a, b, y / max(1, h - 1)))
    return g.resize((w, h))

def squircle(size, n=5.0):
    """Big Sur's rounded square has continuous corners: a superellipse, not a rounded rectangle."""
    big = size * 2
    m = Image.new("L", (big, big), 0)
    px = m.load(); r = big / 2
    for y in range(big):
        dy = abs((y + .5 - r) / r) ** n
        if dy >= 1: continue
        half = int(r * (1 - dy) ** (1 / n))
        for x in range(int(r - half), int(r + half)): px[x, y] = 255
    return m.resize((size, size), Image.LANCZOS)

def blossom(size, color, deep):
    """An original five-petal sakura with notched tips."""
    k = 4; s = size * k; im = Image.new("RGBA", (s, s), (0, 0, 0, 0)); c = s / 2
    for i in range(5):
        a = -math.pi / 2 + i * 2 * math.pi / 5
        petal = Image.new("RGBA", (s, s), (0, 0, 0, 0)); d = ImageDraw.Draw(petal)
        w, l = s * .17, s * .25
        d.ellipse((c - w, c - s * .46, c + w, c - s * .46 + 2 * l), fill=color + (255,))
        d.polygon([(c - s * .05, c - s * .48), (c + s * .05, c - s * .48), (c, c - s * .38)], fill=(0, 0, 0, 0))  # the notch
        im.alpha_composite(petal.rotate(-math.degrees(a + math.pi / 2), resample=Image.BICUBIC, center=(c, c)))
    d = ImageDraw.Draw(im)
    for i in range(5):
        a = -math.pi / 2 + (i + .5) * 2 * math.pi / 5
        d.line((c, c, c + s * .11 * math.cos(a), c + s * .11 * math.sin(a)), fill=deep + (255,), width=k * 3)
        d.ellipse((c + s * .11 * math.cos(a) - k * 4, c + s * .11 * math.sin(a) - k * 4, c + s * .11 * math.cos(a) + k * 4, c + s * .11 * math.sin(a) + k * 4), fill=(255, 214, 120, 255))
    d.ellipse((c - s * .04, c - s * .04, c + s * .04, c + s * .04), fill=deep + (255,))
    return im.resize((size, size), Image.LANCZOS)

def shadow(mask, blur, alpha, dy, size):
    sh = Image.new("RGBA", size, (0, 0, 0, 0))
    a = mask.point(lambda v: v * alpha // 255)
    sh.putalpha(a)
    sh = sh.filter(ImageFilter.GaussianBlur(blur))
    out = Image.new("RGBA", size, (0, 0, 0, 0)); out.alpha_composite(sh, (0, dy))
    return out

def study_card(w, h):
    """The cream flash card with 読 on it, ruled like a study card."""
    k = 2; card = Image.new("RGBA", (w * k, h * k), (0, 0, 0, 0)); d = ImageDraw.Draw(card)
    d.rounded_rectangle((0, 0, w * k - 1, h * k - 1), radius=int(w * k * .09), fill=CARD + (255,))
    for i, y in enumerate((.8, .88)): d.line((w * k * .14, h * k * y, w * k * .86, h * k * y), fill=(247, 190, 212, 255), width=int(4 * k))
    f = ImageFont.truetype(JP, int(h * k * .52))
    d.text((w * k / 2, h * k * .43), "読", font=f, fill=INK + (255,), anchor="mm")
    return card.resize((w, h), Image.LANCZOS)

def icon(size=1024):
    S = 2048; im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    box = int(S * 824 / 1024); off = (S - box) // 2
    m = squircle(box); mask = Image.new("L", (S, S), 0); mask.paste(m, (off, off))
    im.alpha_composite(shadow(mask, 28, 90, 20, (S, S)))
    bg = gradient(S, S, TOP, BOTTOM).convert("RGBA")
    # a soft light from above, as Big Sur icons are lit
    glow = Image.new("L", (S, S), 0); ImageDraw.Draw(glow).ellipse((S * .1, -S * .35, S * .9, S * .5), fill=70)
    bg = Image.composite(Image.new("RGBA", (S, S), (255, 255, 255, 255)), bg, glow.filter(ImageFilter.GaussianBlur(120)))
    tile = Image.new("RGBA", (S, S), (0, 0, 0, 0)); tile.paste(bg, (0, 0), mask)
    # the card, tilted, casting a shadow onto the tile
    cw, ch = int(box * .56), int(box * .66)
    card = study_card(cw, ch).rotate(7, resample=Image.BICUBIC, expand=True)
    cx, cy = (S - card.width) // 2 - int(box * .02), (S - card.height) // 2 + int(box * .03)
    cs = Image.new("L", (S, S), 0); cs.paste(card.getchannel("A"), (cx, cy))
    layer = shadow(cs, 36, 110, 26, (S, S)); layer.alpha_composite(card, (cx, cy))
    b = blossom(int(box * .36), PETAL, PETAL_DEEP)
    bm = Image.new("L", (S, S), 0); bx, by = cx + card.width - int(b.width * .62), cy - int(b.height * .32); bm.paste(b.getchannel("A"), (bx, by))
    layer.alpha_composite(shadow(bm, 16, 90, 10, (S, S))); layer.alpha_composite(b, (bx, by))
    layer.putalpha(ImageChops.multiply(layer.getchannel("A"), mask))
    tile.alpha_composite(layer)
    # a faint edge, so the tile reads on white
    edge = Image.new("RGBA", (S, S), (0, 0, 0, 0)); em = mask.filter(ImageFilter.MaxFilter(3))
    edge.putalpha(ImageChops.subtract(em, mask.filter(ImageFilter.MinFilter(5))).point(lambda v: v * 30 // 255))
    im.alpha_composite(tile); im.alpha_composite(edge)
    return im.resize((size, size), Image.LANCZOS)

def text_img(text, font, size, color):
    f = ImageFont.truetype(font, size)
    if font == INTER:
        f.set_variation_by_axes([32 if "ptical" in str(ax.get("name")) else 700 for ax in f.get_variation_axes()])
    l, t, r, b = f.getbbox(text)
    im = Image.new("RGBA", (r - l + 4, b - t + 4), (0, 0, 0, 0)); ImageDraw.Draw(im).text((2 - l, 2 - t), text, font=f, fill=color)
    return im

def wallpaper(w, h):
    """A soft Big Sur-like backdrop: layered colour hills, in YomiGo's pinks."""
    im = gradient(w, h, (255, 214, 228), (252, 156, 196)).convert("RGBA")
    for i, (col, base, amp, ph) in enumerate([((250, 120, 172), .62, .10, 0.4), ((233, 78, 140), .74, .08, 2.1), ((190, 52, 112), .86, .06, 4.0)]):
        m = Image.new("L", (w, h), 0); d = ImageDraw.Draw(m)
        pts = [(x, h * (base + amp * math.sin(x / w * math.pi * 2 + ph) + amp * .5 * math.sin(x / w * math.pi * 5 + ph * 2))) for x in range(0, w + 8, 8)]
        d.polygon(pts + [(w, h), (0, h)], fill=235)
        # each hill darkens towards its foot, so the layers read as depth rather than flat bands
        fill = gradient(w, h, lerp(col, (255, 255, 255), .25), col).convert("RGBA"); fill.putalpha(m.filter(ImageFilter.GaussianBlur(1.5)))
        im.alpha_composite(fill)
    return im

def banner(w, h, name):
    im = wallpaper(w, h); k = h / 320
    ic = icon(int(170 * k)); title = text_img("YomiGo", INTER, int(84 * k), (74, 22, 46, 255)); tag = text_img("Learn Japanese, one class a day.", INTER, int(26 * k), (110, 40, 72, 255))
    total = ic.width + int(28 * k) + max(title.width, tag.width); x = (w - total) // 2; y = int(h * .18)
    im.alpha_composite(ic, (x, y)); tx = x + ic.width + int(28 * k)
    im.alpha_composite(title, (tx, y + int(34 * k))); im.alpha_composite(tag, (tx + int(4 * k), y + int(34 * k) + title.height + int(10 * k)))
    im.convert("RGB").save(os.path.join(OUT, name), optimize=True)

def android(big):
    """An adaptive icon: the gradient as the background layer, the card and blossom as the foreground."""
    fg = big.resize((432, 432), Image.LANCZOS)
    # Android masks the shape itself, so the foreground is the art without the tile, kept inside the 66% safe zone
    S = 2048; box = int(S * 824 / 1024); cw, ch = int(box * .56), int(box * .66)
    art = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    card = study_card(cw, ch).rotate(7, resample=Image.BICUBIC, expand=True)
    cx, cy = (S - card.width) // 2, (S - card.height) // 2 + 30
    cs = Image.new("L", (S, S), 0); cs.paste(card.getchannel("A"), (cx, cy))
    art.alpha_composite(shadow(cs, 36, 110, 26, (S, S))); art.alpha_composite(card, (cx, cy))
    b = blossom(int(box * .36), PETAL, PETAL_DEEP); bx, by = cx + card.width - int(b.width * .62), cy - int(b.height * .32)
    art.alpha_composite(b, (bx, by))
    pad = Image.new("RGBA", (S, S), (0, 0, 0, 0)); small = art.resize((int(S * .74), int(S * .74)), Image.LANCZOS); pad.alpha_composite(small, ((S - small.width) // 2, (S - small.height) // 2))
    os.makedirs(os.path.join(RES, "drawable-nodpi"), exist_ok=True); os.makedirs(os.path.join(RES, "mipmap-anydpi-v26"), exist_ok=True)
    pad.resize((432, 432), Image.LANCZOS).save(os.path.join(RES, "drawable-nodpi", "icon_fg.png"), optimize=True)
    open(os.path.join(RES, "drawable", "icon_bg.xml"), "w", newline="\n").write(
        '<?xml version="1.0" encoding="utf-8"?>\n<shape xmlns:android="http://schemas.android.com/apk/res/android">\n'
        f'    <gradient android:angle="270" android:startColor="#{TOP[0]:02X}{TOP[1]:02X}{TOP[2]:02X}" android:endColor="#{BOTTOM[0]:02X}{BOTTOM[1]:02X}{BOTTOM[2]:02X}"/>\n</shape>\n')
    open(os.path.join(RES, "mipmap-anydpi-v26", "icon.xml"), "w", newline="\n").write(
        '<?xml version="1.0" encoding="utf-8"?>\n<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">\n'
        '    <background android:drawable="@drawable/icon_bg"/>\n    <foreground android:drawable="@drawable/icon_fg"/>\n    <monochrome android:drawable="@drawable/icon_fg"/>\n</adaptive-icon>\n')
    old = os.path.join(RES, "drawable", "icon.xml")
    if os.path.exists(old): os.remove(old)

def main():
    os.makedirs(OUT, exist_ok=True)
    big = icon(1024)
    big.save(os.path.join(OUT, "icon.png"), optimize=True)
    big.resize((512, 512), Image.LANCZOS).save(os.path.join(OUT, "logo.png"), optimize=True)
    big.resize((256, 256), Image.LANCZOS).save(os.path.join(WWW, "icon.png"), optimize=True)
    big.resize((512, 512), Image.LANCZOS).save(os.path.join(ROOT, "desktop", "icons", "icon.png"))
    big.save(os.path.join(ROOT, "desktop", "icons", "icon.ico"), sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    big.save(os.path.join(ROOT, "desktop", "icons", "icon.icns"))  # for the Mac app
    big.resize((128, 128), Image.LANCZOS).save(os.path.join(ROOT, "extension", "icon128.png"), optimize=True)
    banner(1280, 320, "banner.png"); banner(1280, 640, "social-preview.png")
    android(big)
    if "--shots" in sys.argv:
        subprocess.run([sys.executable, os.path.join(ROOT, "tests", "shots.py"), os.path.join(ROOT, "desktop", "target", "release", "yomigo.exe"), OUT, "desktop-home", "desktop-lesson", "desktop-quest", "tv-home", "tv-exams"], check=True)

if __name__ == "__main__":
    main()
