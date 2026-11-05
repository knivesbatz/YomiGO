#!/usr/bin/env python3
"""Release from this PC, the only place the signing key lives. Builds, tests, signs, then publishes to GitHub.
CI attaches the APK when it sees the new tag. usage: python tools/release.py"""
import io, json, os, shutil, subprocess, sys, zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DIST = os.path.join(ROOT, "dist")
ver = io.open(os.path.join(ROOT, "VERSION")).read().strip()
manifest = json.load(io.open(os.path.join(ROOT, "extension", "manifest.json"), encoding="utf-8"))
assert manifest["version"] == ver, "extension/manifest.json and VERSION disagree"

def run(*a, **k):
    print(">", " ".join(a))
    subprocess.run(a, check=True, cwd=ROOT, **k)

run("cmd", "/c", os.path.join(ROOT, "build.bat"))
# The real program, started off screen: the home, a quiz and the TV home must all draw
run(sys.executable, "tests/shots.py", os.path.join(DIST, "YomiGo.exe"), os.path.join(ROOT, "build", "shots"), "home", "quiz", "settings", "library-guide", "library-media", "mine", "tv-home")
run("node", "--test", "tests/test_core.mjs")
exes = ["YomiGo.exe", "YomiGoHTPC.exe", "YomiGoUpdate.exe", "YomiGoOCR.exe", "YomiGoListen.exe", "YomiGoTalk.exe"]
run("node", "tools/release_keys.mjs", "sign", *[os.path.join("dist", e) for e in exes])
# The updater proves here that it accepts what was just signed, and refuses a damaged copy
upd = os.path.join(DIST, "YomiGoUpdate.exe")
good = subprocess.run([upd, "--verify", os.path.join(DIST, "YomiGo.exe"), os.path.join(DIST, "YomiGo.exe.sig")]).returncode
bad = subprocess.run([upd, "--verify", os.path.join(DIST, "YomiGoOCR.exe"), os.path.join(DIST, "YomiGo.exe.sig")]).returncode  # YomiGo.exe and YomiGoHTPC.exe are the same file now
assert good == 0 and bad != 0, f"signature check failed: good={good} bad={bad}"

zip_win = os.path.join(DIST, "YomiGo-Windows.zip")
with zipfile.ZipFile(zip_win, "w", zipfile.ZIP_DEFLATED) as z:
    for e in exes: z.write(os.path.join(DIST, e), e)
    z.write(os.path.join(ROOT, "docs", "INSTALL.md"), "INSTALL.md")
    z.write(os.path.join(ROOT, "mobile", "app", "src", "main", "assets", "www", "fonts", "Inter-OFL.txt"), "Inter-OFL.txt")
zip_ext = os.path.join(DIST, "YomiGoReader.zip")
shutil.make_archive(zip_ext[:-4], "zip", os.path.join(ROOT, "extension"))

assets = ["dist/YomiGo-Setup.exe", "dist/YomiGo-TV-Setup.exe"] + [os.path.join("dist", n) for n in exes] + [os.path.join("dist", n + ".sig") for n in exes] + ["dist/YomiGo-Windows.zip", "dist/YomiGoReader.zip"]
notes = "See [CHANGELOG](https://github.com/knivesbatz/YomiGO/blob/main/docs/CHANGELOG.md) and [install guide](https://github.com/knivesbatz/YomiGO/blob/main/docs/INSTALL.md). The Android APK and the Mac app are attached by CI within about fifteen minutes."
run("gh", "release", "create", "v" + ver, *assets, "--title", "YomiGo " + ver, "--notes", notes)
