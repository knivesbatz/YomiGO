#!/usr/bin/env python3
"""Makes the logo, banner, and README screenshots. Screenshots come from the real programs, driven with no window.
Build first. usage: python tools/make_brand.py [path to msedge.exe, for the phone screenshots]"""
import io, math, os, shutil, subprocess, sys, tempfile
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "assets", "brand")
FONT = os.path.join(ROOT, "assets", "fonts", "DotGothic16-Regular.ttf")
PLUM, WINE, PINK, ROSE, CREAM, GOLD = (74, 26, 46), (122, 31, 56), (249, 195, 210), (194, 67, 107), (247, 239, 224), (255, 194, 74)

def pixel_text(text, scale, color):
    """Hard-edged dot type, the same way the apps draw it."""
    f = ImageFont.truetype(FONT, 16)
    w = int(f.getlength(text)) + 2
    im = Image.new("L", (w, 18), 0)
    ImageDraw.Draw(im).text((0, 15), text, font=f, fill=255, anchor="ls")
    mask = im.point(lambda v: 255 if v >= 128 else 0).resize((w * scale, 18 * scale), Image.NEAREST)
    out = Image.new("RGBA", mask.size, color + (255,))
    out.putalpha(mask)
    return out

def sakura(size):
    """An original five-petal blossom, drawn small and enlarged so it stays pixel art."""
    n = 32
    im = Image.new("RGBA", (n * 4, n * 4), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    c = n * 2
    for i in range(5):
        a = -math.pi / 2 + i * 2 * math.pi / 5
        x, y = c + n * 0.95 * math.cos(a), c + n * 0.95 * math.sin(a)
        d.ellipse((x - n * 0.72, y - n * 0.72, x + n * 0.72, y + n * 0.72), fill=PINK + (255,))
        # the notch at the tip of each petal is what makes it read as sakura
        tx, ty = c + n * 1.72 * math.cos(a), c + n * 1.72 * math.sin(a)
        d.polygon([(tx - n * 0.2 * math.sin(a) * -1, ty - n * 0.2 * math.cos(a)), (tx + n * 0.25 * math.sin(a) * -1, ty + n * 0.25 * math.cos(a)), (c + n * 1.25 * math.cos(a), c + n * 1.25 * math.sin(a))], fill=(0, 0, 0, 0))
        d.line((c, c, c + n * 0.9 * math.cos(a), c + n * 0.9 * math.sin(a)), fill=ROSE + (255,), width=4)
    d.ellipse((c - n * 0.28, c - n * 0.28, c + n * 0.28, c + n * 0.28), fill=WINE + (255,))
    small = im.resize((32, 32), Image.BOX)
    px = small.load()
    # three flat colours only, so edges stay hard like the rest of the art
    for y in range(32):
        for x in range(32):
            r, g, b, a = px[x, y]
            if a <= 110: px[x, y] = (0, 0, 0, 0)
            else: px[x, y] = min((PINK, ROSE, WINE), key=lambda c: sum((c[i] - (r, g, b)[i]) ** 2 for i in range(3))) + (255,)
    return small.resize((size, size), Image.NEAREST)

def dither_gradient(w, h, a, b, cell=2):
    bayer = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]
    im = Image.new("RGB", (w // cell + 1, h // cell + 1))
    px = im.load()
    for y in range(im.height):
        t = y / max(1, im.height - 1)
        for x in range(im.width):
            d = bayer[y & 3][x & 3]
            px[x, y] = tuple((int(a[i] * (1 - t) + b[i] * t) + d // 2) & 0xF8 for i in range(3))
    return im.resize((im.width * cell, im.height * cell), Image.NEAREST).crop((0, 0, w, h))

def banner(w, h, name):
    im = dither_gradient(w, h, PLUM, WINE)
    k = h / 320
    for x, y, s in [(.05, .15, 40), (.14, .68, 56), (.87, .14, 64), (.94, .62, 40), (.76, .72, 32), (.27, .1, 24), (.66, .08, 24)]:
        p = sakura(int(s * k)); im.paste(p, (int(x * w), int(y * h)), p)
    title = pixel_text("読みGo", int(6 * k), CREAM)
    im.paste(title, ((w - title.width) // 2, int(h * .17)), title)
    tag = pixel_text("Learn Japanese, one class a day.", max(2, int(2 * k)), PINK)
    im.paste(tag, ((w - tag.width) // 2, int(h * .55)), tag)
    d = ImageDraw.Draw(im)
    for x in range(0, w, 32): d.rectangle((x, h - 8, x + 15, h - 5), fill=PINK)
    im.save(os.path.join(OUT, name))

def logo():
    im = Image.new("RGB", (512, 512), WINE)
    d = ImageDraw.Draw(im)
    d.rectangle((0, 0, 511, 511), outline=PLUM, width=16)
    p = sakura(320); im.paste(p, (96, 40), p)
    t = pixel_text("読みGo", 3, CREAM); im.paste(t, ((512 - t.width) // 2, 372), t)
    im.save(os.path.join(OUT, "logo.png"))
    im.resize((128, 128), Image.LANCZOS).save(os.path.join(ROOT, "extension", "icon128.png"))

# A made-up learner, so screenshots show a course in progress
def save_text(day, step=0):
    from_c = ["YOMIGO-SAVE 1", "name=Hana", "xp=4820", "streak=23", "best=23", "last=1", "today=0", "placed=0", f"day={day}", f"step={step}", "pace=1", "en=0", "genki=1", "gender=3", "reviews=340", "perfects=4", "chats=9", "hw=22", "ach=367"]
    for i, ex in enumerate(["S0", "S1", "S2", "S3", "S4", "S5", "N5"]):
        stage_first = [4, 8, 22, 26, 36, 44, 47][i]
        if day > stage_first: from_c.append(f"exam.{ex}=92")
    from_c += [f"card h.{k} 3 7 0 2500 0" for k in "a i u e o ka ki ku ke ko".split()]
    return "\n".join(from_c) + "\n"

def shot(exe, name, script, day, step=0, scale=2):
    root = tempfile.mkdtemp()
    io.open(os.path.join(root, "Hana.sav"), "w", newline="\n").write(save_text(day, step))
    subprocess.run([os.path.join(ROOT, "dist", exe), "--root", root, "--script", script + "pshot;"], check=True, timeout=60)
    im = Image.open(os.path.join(root, "shot.bmp")).convert("RGB")
    im.resize((im.width * scale, im.height * scale), Image.NEAREST).save(os.path.join(OUT, name), optimize=True)
    shutil.rmtree(root, ignore_errors=True)

def screenshots():
    open_save = "w2;ow2;ow2;"  # past the splash, and pick the learner
    d, h = "YomiGo.exe", "YomiGoHTPC.exe"
    shot(d, "desktop-home.png", open_save, 30, scale=1)
    shot(d, "desktop-lesson.png", open_save + "ow2;ow2;" + "ow1;" * 3, 30, step=1, scale=1)
    shot(d, "desktop-quest.png", open_save + "lluuw2;ow2;", 30, scale=1)
    shot(h, "tv-home.png", open_save + "w12;", 30)
    shot(h, "tv-lesson.png", open_save + "ow2;ow2;" + "ow1;" * 3, 30, step=1)
    shot(h, "tv-exams.png", open_save + "rrw8;ow2;", 30)

PHONE_JS = """<script>
localStorage.setItem('yg.names', JSON.stringify(['Hana']));
localStorage.setItem('yg.save.Hana', %s);
addEventListener('load', () => {
  const by = (t) => [...document.querySelectorAll('button')].find((b) => b.textContent.includes(t));
  by('Hana').click();
  %s
});
</script>"""

def phones(edge):
    www = os.path.join(ROOT, "mobile", "app", "src", "main", "assets", "www")
    import json
    tmp = tempfile.mkdtemp()
    for f in os.listdir(www): shutil.copy(os.path.join(www, f), tmp)
    base = io.open(os.path.join(www, "index.html"), encoding="utf-8").read()
    for name, js in (("phone-home.png", ""), ("phone-lesson.png", "by('はじめる').click();by('つぎ').click();by('つぎ').click();"), ("phone-quest.png", "by('クエスト').click();")):
        page = base.replace("</body>", PHONE_JS % (json.dumps(save_text(30, 1 if "lesson" in name else 0)), js) + "</body>")
        p = os.path.join(tmp, "shot.html")
        io.open(p, "w", encoding="utf-8").write(page)
        out = os.path.join(OUT, name)
        subprocess.run([edge, "--headless=new", "--disable-gpu", "--force-device-scale-factor=2", "--window-size=375,720", "--allow-file-access-from-files", "--virtual-time-budget=2000", f"--screenshot={out}", "file:///" + p.replace(os.sep, "/")], check=True, timeout=90, capture_output=True)
    shutil.rmtree(tmp, ignore_errors=True)

if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    logo()
    banner(1280, 320, "banner.png")
    banner(1280, 640, "social-preview.png")
    screenshots()
    if len(sys.argv) > 1: phones(sys.argv[1])
    print("brand ok:", sorted(os.listdir(OUT)))
