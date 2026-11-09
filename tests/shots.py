#!/usr/bin/env python3
"""Screenshots of the Windows app, taken off screen without taking the focus.
usage: python tests/shots.py [exe] [outdir]   Each shot starts the app with a test save and a script, waits, and captures the window."""
import ctypes, ctypes.wintypes as wt, io, os, subprocess, sys, tempfile, time, shutil, urllib.parse
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "desktop", "target", "release", "yomigo.exe")
OUT = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, "build", "shots")
os.makedirs(OUT, exist_ok=True)
u32, g32 = ctypes.windll.user32, ctypes.windll.gdi32
u32.SetProcessDPIAware()

def windows_of(pid):
    found = []
    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(h, _):
        p = wt.DWORD(); u32.GetWindowThreadProcessId(h, ctypes.byref(p))
        if p.value == pid and u32.IsWindowVisible(h): found.append(h)
        return True
    u32.EnumWindows(cb, 0)
    return found

def grab(hwnd, path):
    r = wt.RECT(); u32.GetWindowRect(hwnd, ctypes.byref(r))
    w, h = r.right - r.left, r.bottom - r.top
    dc = u32.GetWindowDC(hwnd); mdc = g32.CreateCompatibleDC(dc); bmp = g32.CreateCompatibleBitmap(dc, w, h)
    g32.SelectObject(mdc, bmp)
    u32.PrintWindow(hwnd, mdc, 2)  # PW_RENDERFULLCONTENT, so the web view is drawn too
    class BIH(ctypes.Structure):
        _fields_ = [("biSize", wt.DWORD), ("biWidth", wt.LONG), ("biHeight", wt.LONG), ("biPlanes", wt.WORD), ("biBitCount", wt.WORD), ("biCompression", wt.DWORD),
                    ("biSizeImage", wt.DWORD), ("biXPelsPerMeter", wt.LONG), ("biYPelsPerMeter", wt.LONG), ("biClrUsed", wt.DWORD), ("biClrImportant", wt.DWORD)]
    bi = BIH(40, w, -h, 1, 32, 0, 0, 0, 0, 0, 0)
    buf = ctypes.create_string_buffer(w * h * 4)
    g32.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bi), 0)
    g32.DeleteObject(bmp); g32.DeleteDC(mdc); u32.ReleaseDC(hwnd, dc)
    im = Image.frombuffer("RGBA", (w, h), buf, "raw", "BGRA", 0, 1).convert("RGB")
    # Crop to what's visible: the window's resize border around it is invisible on screen
    vis = wt.RECT()
    if ctypes.windll.dwmapi.DwmGetWindowAttribute(hwnd, 9, ctypes.byref(vis), ctypes.sizeof(vis)) == 0:
        im = im.crop((vis.left - r.left, vis.top - r.top, vis.right - r.left, vis.bottom - r.top))
    im.save(path)

def save_text(day=12, xp=420, streak=6):
    return f"YOMIGO-SAVE 1\nname=Hana\nxp={xp}\nstreak={streak}\nbest=9\nlast=0\ntoday=0\nplaced=1\nday={day}\nstep=3\npace=1\nen=0\ngenki=1\ngender=1\nreviews=80\nperfects=3\nchats=5\nhw=8\nach=0\nreads=4\nlistens=2\n" + "".join(f"card h.{k} 1 1 0 2500 1\n" for k in "aiueo")

def shot(name, js, size="1180x760", tv=False, wait=4.5, theme=None):
    root = tempfile.mkdtemp()
    io.open(os.path.join(root, "Hana.sav"), "w", newline="\n").write(save_text())
    pre = f"try{{localStorage.setItem('yg.theme','{theme or 'light'}');localStorage.setItem('yg.mute','1')}}catch(e){{}}; applyTheme();"
    if name.startswith("phone"): pre += "root.classList.remove('desk','no-side');"  # the phone layout, drawn by the same page
    # Just before the capture, the page reports whether any "[object ...]" text leaked onto it, by saving a check file
    # It's set up before the shot's own script, so an error there is reported by name instead of silencing the check
    check = "setTimeout(()=>__TAURI__.core.invoke('save',{name:'pagecheck',text:'YOMIGO-SAVE 1\\nobject='+(document.body.innerText.includes('[object ')?1:0)+'\\nerr='+encodeURIComponent(__errs.join(' | '))+'\\n'}),%d);" % int((wait - 2.2) * 1000)  # the script itself starts 1.5 s after launch
    errs = "window.__errs=[];addEventListener('error',(e)=>__errs.push(e.message));addEventListener('unhandledrejection',(e)=>__errs.push(String(e.reason)));"
    io.open(os.path.join(root, "go.js"), "w", encoding="utf-8").write(errs + check + "try{" + pre + js + "}catch(e){__errs.push(String(e))}")
    # Its own web profile, so a test's settings (theme, sounds, look) never touch the learner's real app
    env = dict(os.environ, WEBVIEW2_USER_DATA_FOLDER=os.path.join(root, "webview"), WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS="--disable-features=CalculateNativeWinOcclusion --disable-backgrounding-occluded-windows")
    args = [EXE, "--root", root, "--eval", os.path.join(root, "go.js"), "--size", size] + (["--tv"] if tv else [])
    p = subprocess.Popen(args, env=env)
    try:
        time.sleep(wait)
        pc = os.path.join(root, "pagecheck.sav")
        for _ in range(30):  # a cold first start on a busy PC can run late; give it up to 15 s more before trying again
            if os.path.exists(pc): time.sleep(0.7); break
            time.sleep(0.5)
        hs = windows_of(p.pid)
        if not hs: raise SystemExit("no window for " + name)
        r = wt.RECT(); u32.GetWindowRect(hs[0], ctypes.byref(r))
        # On a real PC the window must stay out of sight; CI machines have no one watching and place it at 0,0
        if not os.environ.get("CI"): assert r.right < 0 or r.left > u32.GetSystemMetrics(0), f"test window on screen at {r.left},{r.top}"
        grab(hs[0], os.path.join(OUT, name + ".png"))
        if not os.path.exists(pc): return False
        report = open(pc).read()
        if "object=1" in report: raise SystemExit(f"{name}: the page shows [object ...] text")
        err = report.split("err=")[1].split("\n")[0] if "err=" in report else ""
        if err: raise SystemExit(f"{name}: the page hit an error: {urllib.parse.unquote(err)}")
        print("shot", name)
        return True
    finally:
        p.kill(); p.wait(); shutil.rmtree(root, ignore_errors=True)

OPEN = "document.querySelector('.item').click();"
SHOTS = {
    "profiles": "",
    "home": OPEN,
    "home-dark": OPEN,
    "cards": OPEN + "setTimeout(()=>cards(),300);",
    "quiz": OPEN + "setTimeout(()=>{runStep()},300);setTimeout(()=>{const b=document.querySelector('.choices .btn');if(b)b.click();},900);",
    "missed": OPEN + "setTimeout(()=>runQuiz([Y.makeQ(Y.byId['v.konnichiwa'],2,true),Y.makeQ(Y.byId['v.konnichiwa'],1,false)],'Practice',()=>{}),300);setTimeout(()=>[...document.querySelectorAll('.btn.quiet')].pop().click(),900);",
    "listen": OPEN + "setTimeout(()=>{const q=Y.makeQ(Y.byId['v.konnichiwa'],0,false);q.say='こんにちは';runQuiz([q],'Listening',()=>{})},300);",
    "notes": OPEN + "setTimeout(()=>{P.day=C.days.findIndex(d=>d.notes.some(n=>n.includes('すみません')));teach(C.days[P.day].notes.findIndex(n=>n.includes('すみません')))},300);",
    "shadow": OPEN + "setTimeout(()=>{P.day=C.days.findIndex(d=>d.new.some(id=>Y.byId[id].f==='倍'));const l=Y.shadowLines(P.day);shadow(l,l.findIndex(x=>x.jp==='倍'))},300);",
    "goal": OPEN + "setTimeout(()=>{P.gate=0;P.exams={};for(const e of C.exams)P.exams[e.id]=90;P.pace=2;P.last=Y.today();P.today=1;home()},300);",
    "classic": "try{localStorage.setItem('yg.look','classic')}catch(e){};applyTheme();" + OPEN,
    "lost": "Host.store.write('Hana', Host.store.text('Hana').replace('last=0','last=' + (Y.today() - 3)));" + OPEN,
    "classic-quiz": "try{localStorage.setItem('yg.look','classic')}catch(e){};applyTheme();" + OPEN + "setTimeout(()=>{runStep()},300);setTimeout(()=>{const b=document.querySelector('.choices .btn');if(b)b.click();},900);",
    "mine": OPEN + "setTimeout(()=>{readerLines=['今日は天気がいいので、公園で散歩しました。','公園には桜の木がたくさんあって、とても綺麗でした。','散歩のあと、喫茶店でコーヒーを飲みました。'];textReader();setTimeout(()=>[...document.querySelectorAll('.btn.primary')].find(b=>b.textContent.includes('Find')).click(),400)},300);",
    "typing": OPEN + "setTimeout(()=>typing([Y.byId['v.konnichiwa']],0,0,false),300);",
    "exam-listen": OPEN + "setTimeout(()=>{globalThis.YG_TEST_VOICE=1;placement(3,3)},300);",
    "library-guide": OPEN + "setTimeout(()=>{libTab=1;library()},300);",
    "library-media": OPEN + "setTimeout(()=>{libTab=2;library()},300);",
    "teach": OPEN + "setTimeout(()=>teach(4),300);",
    "settings": OPEN + "setTimeout(()=>settings(),300);",
    "settings-look": OPEN + "setTimeout(()=>{settings();setTimeout(()=>[...document.querySelectorAll('*')].filter(e=>e.scrollHeight>e.clientHeight+20).forEach(e=>e.scrollTop=e.scrollHeight),300)},300);",
    "library": OPEN + "setTimeout(()=>library(),300);",
    "quest": OPEN + "setTimeout(()=>quest(),300);",
    "phone-home": OPEN,
    "phone-settings": OPEN + "setTimeout(()=>settings(),300);",
    "phone-quiz": OPEN + "setTimeout(()=>{runStep()},300);",
    "tv-home": OPEN,
    "tv-exams": OPEN + "setTimeout(()=>exams(),300);",
    "desktop-home": OPEN,
    "desktop-lesson": OPEN + "setTimeout(()=>teach(4),300);",
    "desktop-quest": OPEN + "setTimeout(()=>quest(),300);",
    "tv-quiz": OPEN + "setTimeout(()=>{runStep()},300);setTimeout(()=>{const e=new KeyboardEvent('keydown',{key:'ArrowRight',bubbles:true});document.dispatchEvent(e)},1200);",
}
want = sys.argv[3:] if len(sys.argv) > 3 else list(SHOTS)
for n in want:
    args = dict(size="420x860" if n.startswith("phone") else "1280x720" if n.startswith("tv") else "1180x760", tv=n.startswith("tv"), theme="dark" if n.endswith("dark") else None)
    # A shared CI machine sometimes starts the page late; one more try tells that apart from a page that's really broken
    if not shot(n, SHOTS[n], **args) and not shot(n, SHOTS[n], wait=9, **args): raise SystemExit(f"{n}: the page never reported back, twice")
