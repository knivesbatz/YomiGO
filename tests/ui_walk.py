#!/usr/bin/env python3
"""Plays the whole course through the real screens of both Windows builds, with no window, then checks the save."""
import os, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
START = {"YomiGo.exe": "w2;ow2;ow2;tWalker;ow2;odow2;G;", "YomiGoHTPC.exe": None}
fail = 0
for exe in ("YomiGo.exe", "YomiGoHTPC.exe"):
    root = tempfile.mkdtemp()
    if exe == "YomiGo.exe":
        script = "w2;ow2;ow2;tWalker;w1;ow2;dw1;ow2;dw1;ow2;G;"
    else:
        # the TV build types its name on the on-screen keyboard, then presses Start
        script = "w2;ow2;ow2;ow1;sw2;dw1;ow2;dw1;ow2;G;"
    r = subprocess.run([os.path.join(ROOT, "dist", exe), "--root", root, "--script", script], timeout=600)
    saves = [f for f in os.listdir(root) if f.endswith(".sav")]
    text = open(os.path.join(root, saves[0]), encoding="utf-8").read() if saves else ""
    vals = dict(l.split("=", 1) for l in text.splitlines() if "=" in l)
    cards = sum(1 for l in text.splitlines() if l.startswith("card "))
    ok = r.returncode == 0 and saves and int(vals.get("day", 0)) == 47 and cards > 400 and int(vals.get("xp", 0)) > 5000 and "exam.N5" in vals
    print(exe, "exit", r.returncode, "day", vals.get("day"), "cards", cards, "xp", vals.get("xp"), "chats", vals.get("chats"), "hw", vals.get("hw"), "OK" if ok else "FAILED")
    fail += not ok
    shutil.rmtree(root, ignore_errors=True)
sys.exit(fail != 0)
