#!/usr/bin/env python3
"""End-to-end update test against the live GitHub release, run in the background. usage: python tests/update_test.py [old version]
1. An old install (default 0.20.0, downloaded from GitHub) updates itself to the latest release with its own updater.
2. A running copy of the latest app is closed by the latest updater and swapped in place, as every later update will do.
Both use --yes: install without asking and don't restart, so nothing appears on screen. Signatures are checked as usual."""
import hashlib, json, os, shutil, subprocess, sys, tempfile, time, urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OLD = sys.argv[1] if len(sys.argv) > 1 else "0.20.0"
latest = json.load(urllib.request.urlopen("https://api.github.com/repos/knivesbatz/YomiGO/releases/latest"))["tag_name"].lstrip("v")
sha = lambda p: hashlib.sha256(open(p, "rb").read()).hexdigest()
def get(ver, name, to):
    urllib.request.urlretrieve(f"https://github.com/knivesbatz/YomiGO/releases/download/v{ver}/{name}", to)

work = tempfile.mkdtemp(prefix="yg-update-")
try:
    want = os.path.join(work, "want"); os.makedirs(want)
    for n in ("YomiGo.exe", "YomiGoUpdate.exe"): get(latest, n, os.path.join(want, n))
    app = os.path.join(work, "app"); os.makedirs(os.path.join(app, "saves"))
    for n in ("YomiGo.exe", "YomiGoUpdate.exe"): get(OLD, n, os.path.join(app, n))
    open(os.path.join(app, "saves", "Test.sav"), "w", newline="\n").write("YOMIGO-SAVE 1\nname=Test\nxp=5\n")
    assert sha(os.path.join(app, "YomiGo.exe")) != sha(os.path.join(want, "YomiGo.exe")), "old and new are the same file"

    # 1. the old updater brings the old install up to date. A made-up parent id: nothing of the user's is closed.
    r = subprocess.run([os.path.join(app, "YomiGoUpdate.exe"), "YomiGo.exe", OLD, "4000000", "--yes"], timeout=600)
    assert r.returncode == 0, f"old updater failed: {r.returncode}"
    assert sha(os.path.join(app, "YomiGo.exe")) == sha(os.path.join(want, "YomiGo.exe")), "YomiGo.exe wasn't replaced"
    assert sha(os.path.join(app, "YomiGoUpdate.exe")) == sha(os.path.join(want, "YomiGoUpdate.exe")), "the updater didn't refresh itself"
    assert open(os.path.join(app, "saves", "Test.sav")).read().startswith("YOMIGO-SAVE 1"), "the save changed"
    print(f"ok: {OLD} updated itself to {latest}")

    # 2. the new updater closes a running new app (off screen, test saves) and swaps it
    env = dict(os.environ, WEBVIEW2_USER_DATA_FOLDER=os.path.join(work, "webview"), WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS="--disable-features=CalculateNativeWinOcclusion")
    p = subprocess.Popen([os.path.join(app, "YomiGo.exe"), "--root", os.path.join(work, "tsaves")], env=env)
    time.sleep(4)
    assert p.poll() is None, "the new app didn't start"
    t0 = time.time()
    r = subprocess.run([os.path.join(app, "YomiGoUpdate.exe"), "YomiGo.exe", OLD, str(p.pid), "--yes"], timeout=600)
    try: p.wait(timeout=20)
    except subprocess.TimeoutExpired: p.kill(); raise AssertionError("the updater didn't close the running app")
    assert r.returncode == 0, f"new updater failed: {r.returncode}"
    assert sha(os.path.join(app, "YomiGo.exe")) == sha(os.path.join(want, "YomiGo.exe"))
    print(f"ok: the {latest} updater closed the running app in {time.time() - t0:.1f}s and swapped it")
finally:
    shutil.rmtree(work, ignore_errors=True)
