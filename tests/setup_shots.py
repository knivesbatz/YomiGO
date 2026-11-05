#!/usr/bin/env python3
"""Screenshots of each installer page, taken off screen. usage: python tests/setup_shots.py [setup.exe] [outdir]"""
import os, shutil, subprocess, sys, tempfile, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "shots.py"), encoding="utf-8").read().split("def save_text")[0]
sys.argv = sys.argv[:1] + ["x", "y"] if len(sys.argv) < 2 else sys.argv
exec(src)  # windows_of and grab
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SETUP = sys.argv[1] if len(sys.argv) > 1 and sys.argv[1].endswith(".exe") else os.path.join(ROOT, "desktop", "target", "release", "yomigo-setup.exe")
OUT = os.path.join(ROOT, "build", "shots")
PAGES = {"setup-1": "", "setup-2": "go(1)", "setup-3": "go(2)", "setup-done": "page(4,[h('div',{cls:'done-mark'},icon('check')),h('h2',{cls:'center'},'The installation was completed successfully.'),h('p',{cls:'center dim'},'YomiGo is in your Start menu and on your desktop. It keeps itself up to date.')],[btn('Close',()=>{}),btn('Open YomiGo',()=>{},'primary')])"}
for name, js in PAGES.items():
    tmp = tempfile.mkdtemp(); f = os.path.join(tmp, "go.js"); open(f, "w", encoding="utf-8").write(js + ";")
    exe = os.path.join(tmp, "YomiGo-Setup.exe"); shutil.copy(SETUP, exe)
    p = subprocess.Popen([exe], env=dict(os.environ, YG_SETUP_EVAL=f, WEBVIEW2_USER_DATA_FOLDER=os.path.join(tmp, "wv"), WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS="--force-device-scale-factor=1"))
    time.sleep(3.5)
    try: grab(windows_of(p.pid)[0], os.path.join(OUT, name + ".png")); print("shot", name)
    finally: p.kill(); p.wait(); shutil.rmtree(tmp, ignore_errors=True)
