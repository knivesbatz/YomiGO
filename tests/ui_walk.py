#!/usr/bin/env python3
"""Plays the whole course through the real screens of both Windows builds, with no window, then checks the save."""
import json, os, shutil, subprocess, sys, tempfile
sys.stdout.reconfigure(encoding="utf-8") # Japanese in the output, even when it goes to a file

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DAYS = len(json.load(open(os.path.join(ROOT, "build", "gen", "content.json"), encoding="utf-8"))["days"])
START = {"YomiGo.exe": "w2;ow2;ow2;tWalker;ow2;odow2;G;", "YomiGoHTPC.exe": None}
fail = 0
for exe in ("YomiGo.exe", "YomiGoHTPC.exe"):
    root = tempfile.mkdtemp()
    if exe == "YomiGo.exe":
        script = "w2;ow2;ow2;tWalker;w1;ow2;ddw1;ow2;dw1;ow2;dw1;ow2;G;"
    else:
        # the TV build types its name on the on-screen keyboard, then presses Start
        script = "w2;ow2;ow2;ow1;sw2;rrw1;ow2;dw1;ow2;dw1;ow2;G;"
    r = subprocess.run([os.path.join(ROOT, "dist", exe), "--root", root, "--script", script], timeout=600)
    saves = [f for f in os.listdir(root) if f.endswith(".sav")]
    text = open(os.path.join(root, saves[0]), encoding="utf-8").read() if saves else ""
    vals = dict(l.split("=", 1) for l in text.splitlines() if "=" in l)
    cards = sum(1 for l in text.splitlines() if l.startswith("card "))
    ok = r.returncode == 0 and saves and int(vals.get("day", 0)) == DAYS and cards > 400 and int(vals.get("xp", 0)) > 5000 and "exam.N5" in vals and "exam.N4" in vals and int(vals.get("reads", 0)) >= 10
    print(exe, "exit", r.returncode, "day", vals.get("day"), "cards", cards, "xp", vals.get("xp"), "chats", vals.get("chats"), "hw", vals.get("hw"), "reads", vals.get("reads"), "OK" if ok else "FAILED")
    fail += not ok
    shutil.rmtree(root, ignore_errors=True)
# Listening, on a pretend voice since a test PC may have none: the whole course again, through those screens.
# Speaking aloud is optional on PC and TV, and the test player takes it once a day, so it's walked too.
for exe in ("YomiGo.exe", "YomiGoHTPC.exe"):
    root = tempfile.mkdtemp()
    script = "w2;ow2;ow2;tEars;w1;ow2;ddw1;ow2;dw1;ow2;dw1;ow2;G;" if exe == "YomiGo.exe" else "w2;ow2;ow2;ow1;sw2;rrw1;ow2;dw1;ow2;dw1;ow2;G;"
    r = subprocess.run([os.path.join(ROOT, "dist", exe), "--root", root, "--fake-voice", "1", "--script", script], timeout=600)
    saves = [f for f in os.listdir(root) if f.endswith(".sav")]
    vals = dict(l.split("=", 1) for l in open(os.path.join(root, saves[0]), encoding="utf-8").read().splitlines() if "=" in l) if saves else {}
    ok = r.returncode == 0 and int(vals.get("day", 0)) == DAYS and int(vals.get("listens", 0)) >= DAYS - 1
    print("listening walk", exe, "listens", vals.get("listens"), "OK" if ok else "FAILED")
    fail += not ok
    shutil.rmtree(root, ignore_errors=True)
# The real Windows speech call, in silence. It needs some voice installed, and CI may not have one.
r = subprocess.run([os.path.join(ROOT, "dist", "YomiGo.exe"), "--speak-test", "1"], timeout=60)
print("speech call, silent", "OK" if r.returncode == 0 else "no voice or failed: not counted", r.returncode)
# Text Reader: Japanese copied by another program appears, a click looks a word up, and Add keeps the line as the sentence
from PIL import Image, ImageDraw, ImageFont
root = tempfile.mkdtemp()
esc = lambda s: "".join("\\u%04x" % ord(c) for c in s)
line = "今日は猫を見た"
# the rail's Text button is the sixth: from Start class, left twice reaches Settings, then up once
script = "w2;ow2;ow2;tText;w1;ow2;ddw1;ow2;dw1;ow2;dw1;ow2;lluw1;ow2;ow2;K" + esc(line) + ";w2;"
# then click 猫, the fourth character, and the first result's Add button
subprocess.run([os.path.join(ROOT, "dist", "YomiGo.exe"), "--root", root, "--script", script.replace(";ow2;ow2;K", ";ow2;K") + "ow2;M172,113;w1;M590,183;w1;"], timeout=60)
text = open(os.path.join(root, "Text.sav"), encoding="utf-8").read() if os.path.exists(os.path.join(root, "Text.sav")) else ""
ok = "\t猫\tねこ\t" in text and ("\t" + line) in text
print("text reader: a pasted line, a clicked word, and Add keep the whole line as its sentence:", "OK" if ok else "FAILED")
fail += not ok
shutil.rmtree(root, ignore_errors=True)
# A screenshot through the OCR helper, in English because the build PC has no Japanese OCR. Its lines need Japanese to show, so only the reader's own output is checked.
root = tempfile.mkdtemp()
im = Image.new("RGB", (640, 120), "white"); ImageDraw.Draw(im).text((20, 30), "OCR check 42", fill="black", font=ImageFont.truetype("C:/Windows/Fonts/arial.ttf", 40))
img = os.path.join(root, "shot.bmp"); im.save(img)
r = subprocess.run([os.path.join(ROOT, "dist", "YomiGoOCR.exe"), img, os.path.join(root, "out.txt"), "en"], timeout=120)
got = open(os.path.join(root, "out.txt"), encoding="utf-8").read().strip() if r.returncode == 0 else ""
print("screenshot reader", "OK" if got == "OCR check 42" else "FAILED", repr(got), r.returncode)
fail += got != "OCR check 42"
subprocess.run([os.path.join(ROOT, "dist", "YomiGo.exe"), "--root", root, "--script", "w2;ow2;ow2;tOcr;w1;ow2;ddw1;ow2;dw1;ow2;dw1;ow2;lluw1;ow2;I" + img.replace("\\", "/") + ";rrw1;ow1;W;pocr;"], timeout=120)
print("screenshot through YomiGo:", "OK" if os.path.exists(os.path.join(root, "ocr_out.txt")) and "OCR check 42" in open(os.path.join(root, "ocr_out.txt"), encoding="utf-8").read() else "FAILED")
fail += not (os.path.exists(os.path.join(root, "ocr_out.txt")) and "OCR check 42" in open(os.path.join(root, "ocr_out.txt"), encoding="utf-8").read())
shutil.rmtree(root, ignore_errors=True)
# Transcribing a file through YomiGo, in English as the build PC has no Japanese voice. It needs the speech model, which the app downloads on first use.
if os.path.exists(os.path.join(ROOT, "dist", "models", "ggml-large-v3-turbo-q5_0.bin")):
    root = tempfile.mkdtemp()
    wav = os.path.join(root, "talk.wav")
    subprocess.run(["powershell", "-NoProfile", "-Command", "Add-Type -AssemblyName System.Speech; $s = New-Object System.Speech.Synthesis.SpeechSynthesizer; $s.SetOutputToWaveFile('%s'); $s.Speak('Practice makes perfect.'); $s.Dispose()" % wav], check=True)
    subprocess.run([os.path.join(ROOT, "dist", "YomiGo.exe"), "--root", root, "--script", "w2;ow2;ow2;tEar;w1;ow2;ddw1;ow2;dw1;ow2;dw1;ow2;lluw1;ow2;A" + wav.replace("\\", "/") + " en;rrrw1;ow1;W;"], timeout=600)
    out = open(os.path.join(root, "listen_out.txt"), encoding="utf-8").read() if os.path.exists(os.path.join(root, "listen_out.txt")) else ""
    ok = "practice makes perfect" in out.lower() and os.path.exists(os.path.join(root, "talk.en.srt"))
    print("transcribing through YomiGo, with subtitles beside the file:", "OK" if ok else "FAILED", repr(out.strip()))
    fail += not ok
    shutil.rmtree(root, ignore_errors=True)
else:
    print("transcribing: SKIPPED, no speech model in dist\\models")
# Who's learning: a man gets ぼく and おれ in My words, on the PC; a woman gets あたし on the TV
for exe, keys, want, gender in (("YomiGo.exe", "w1;ow2;dw1;ow2;", ["ぼく", "おれ"], 2), ("YomiGoHTPC.exe", "w1;sw2;ow2;", ["あたし"], 1),
                                ("YomiGo.exe", "w1;ow2;dddw1;ow2;", ["ぼく"], 4), ("YomiGoHTPC.exe", "w1;sw2;rrrw1;ow2;", ["ぼく"], 4)):
    root = tempfile.mkdtemp()
    subprocess.run([os.path.join(ROOT, "dist", exe), "--root", root, "--script", "w2;ow2;ow2;tWho;" + keys], timeout=60)
    save = open(os.path.join(root, "Who.sav"), encoding="utf-8").read()
    mine = [l.split("\t")[2] for l in save.splitlines() if l.startswith("my ")]
    ok = mine == want and f"gender={gender}" in save
    print("who's learning on", exe, "OK" if ok else "FAILED", mine)
    fail += not ok
    shutil.rmtree(root, ignore_errors=True)
# Talking with Sakura, as an N5 learner: a greeting from her, then a reply to a sentence with a mistake in it, with its checks
if os.path.exists(os.path.join(ROOT, "dist", "models", "gemma-4-E4B_q4_0-it.gguf")):
    import time
    sys.path.insert(0, os.path.join(ROOT, "tools"))
    import make_brand
    # Through YomiGo: the screen asks, waits, and reads each reply, so none is left over
    root = tempfile.mkdtemp()
    open(os.path.join(root, "Hana.sav"), "w", encoding="utf-8", newline="\n").write(make_brand.save_text(50, 0))
    said = "".join(c if ord(c) < 128 else "\\u%04x" % ord(c) for c in "きのう えいがを みるました。")
    r = subprocess.run([os.path.join(ROOT, "dist", "YomiGo.exe"), "--root", root, "--script", "w2;ow2;ow2;llw1;ow2;W;w2;t" + said + ";w1;ow1;W;w2;"], timeout=900)
    ok = r.returncode == 0 and not os.path.exists(os.path.join(root, "talk_out.txt")) and not os.path.exists(os.path.join(root, "talk_req.txt"))
    print("talking through YomiGo:", "OK" if ok else "FAILED", r.returncode)
    fail += not ok
    shutil.rmtree(root, ignore_errors=True)
    # The helper itself: a reply in Japanese, a correction of 見るました, and an English line
    root = tempfile.mkdtemp()
    p = subprocess.Popen([os.path.join(ROOT, "dist", "YomiGoTalk.exe"), os.path.join(ROOT, "dist", "models", "gemma-4-E4B_q4_0-it.gguf"), root, str(os.getpid())])
    open(os.path.join(root, "talk_req.txt"), "w", encoding="utf-8", newline="\n").write(
        "S\tYou are Sakura, chatting with someone learning Japanese at JLPT N5. Reply in one short sentence of simple Japanese.\nX\tfix en\nU\tきのう えいがを みるました。\n")
    out, t0 = os.path.join(root, "talk_out.txt"), time.time()
    while not os.path.exists(out) and p.poll() is None and time.time() - t0 < 300: time.sleep(0.2)
    time.sleep(0.2)
    got = open(out, encoding="utf-8").read() if os.path.exists(out) else ""
    p.kill()
    lines = dict(l.split(": ", 1) for l in got.splitlines() if ": " in l)
    ok = any(ord(c) > 0x3000 for c in lines.get("JA", "")) and any(w in lines.get("FIX", "") for w in ("見ました", "みました")) and len(lines.get("EN", "")) > 3
    print("Sakura's reply, correction, and translation:", "OK" if ok else "FAILED", repr(got[:200]))
    fail += not ok
    shutil.rmtree(root, ignore_errors=True)
else:
    print("talking: SKIPPED, no conversation model in dist\\models")
# An older save, from before this update's new words: opening it adds the cards its earlier classes now have
sys.path.insert(0, os.path.join(ROOT, "tools"))
import make_brand
C = json.load(open(os.path.join(ROOT, "build", "gen", "content.json"), encoding="utf-8"))
root = tempfile.mkdtemp()
open(os.path.join(root, "Hana.sav"), "w", encoding="utf-8", newline="\n").write(make_brand.save_text(50, 0) + "gender=3\n")
subprocess.run([os.path.join(ROOT, "dist", "YomiGo.exe"), "--root", root, "--script", "w2;ow2;ow2;w2;"], timeout=60)
have = {l.split(" ")[1] for l in open(os.path.join(root, "Hana.sav"), encoding="utf-8").read().splitlines() if l.startswith("card ")}
want = {i for d in C["days"] if d["src"] < 50 for i in d["new"]}  # the save is from before 0.20, so day 50 is class 50 in course.txt
ok = want <= have
print("an older save catches up on cards added to its earlier classes:", "OK" if ok else "FAILED", len(want - have), "missing")
fail += not ok
shutil.rmtree(root, ignore_errors=True)
# Your own words on the TV: find 猫 in the dictionary, add it, and it must be saved
root = tempfile.mkdtemp()
subprocess.run([os.path.join(ROOT, "dist", "YomiGoHTPC.exe"), "--root", root, "--script", "w2;ow2;ow2;tMine;w1;sw2;rrw1;ow2;dw1;ow2;dw1;ow2;rrrrw8;ow2;Rw2;tneko;w1;sw2;xw2;"], timeout=60)
text = open(os.path.join(root, "Mine.sav"), encoding="utf-8").read() if os.path.exists(os.path.join(root, "Mine.sav")) else ""
ok = "\t猫\tねこ\t" in text
print("adding from the dictionary", "OK" if ok else "FAILED")
fail += not ok
shutil.rmtree(root, ignore_errors=True)
# The PC dictionary: Enter searches even though clicking the Dict tab left it focused, then Add saves the top match
root = tempfile.mkdtemp()
open(os.path.join(root, "Hana.sav"), "w", encoding="utf-8", newline="\n").write(make_brand.save_text(30, 0))
subprocess.run([os.path.join(ROOT, "dist", "YomiGo.exe"), "--root", root, "--script", "w2;ow2;ow2;M50,233;w2;M369,61;w2;tneko;w1;ow2;M600,120;w2;"], timeout=60)
ok = "\t猫\tねこ\t" in open(os.path.join(root, "Hana.sav"), encoding="utf-8").read()
print("Enter searches the PC dictionary", "OK" if ok else "FAILED")
fail += not ok
shutil.rmtree(root, ignore_errors=True)
# Leaving YomiGo during an exam fails the open question: switch away (F) before picking each answer, so none can score
root = tempfile.mkdtemp()
open(os.path.join(root, "Hana.sav"), "w", encoding="utf-8", newline="\n").write("".join(l + "\n" for l in make_brand.save_text(5, 0).splitlines() if not l.startswith("exam.S0=")))
subprocess.run([os.path.join(ROOT, "dist", "YomiGo.exe"), "--root", root, "--script", "w2;ow2;ow2;M50,150;w2;ow2;" + "Fw1;1w1;ow1;" * 20 + "w2;"], timeout=60)
vals = dict(l.split("=", 1) for l in open(os.path.join(root, "Hana.sav"), encoding="utf-8").read().splitlines() if "=" in l)
ok = vals.get("exam.S0", "0") == "0"
print("leaving during an exam fails the question", "OK" if ok else "FAILED", vals.get("exam.S0"))
fail += not ok
shutil.rmtree(root, ignore_errors=True)
# Home Wi-Fi sync, playing the phone's part: the PC's Sync screen refuses a wrong code, then merges a phone save
# that's further along (day 40) into its own (day 30), sends the merged save back, and keeps it
import time, urllib.request, urllib.error
def post(body):
    try:
        with urllib.request.urlopen(urllib.request.Request("http://127.0.0.1:47820/sync", data=body.encode("utf-8"), headers={"Content-Type": "text/plain"}), timeout=10) as r:
            return r.status, r.read().decode("utf-8")
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode("utf-8")
    except OSError as e:
        return 0, str(e)
root = tempfile.mkdtemp()
open(os.path.join(root, "Hana.sav"), "w", encoding="utf-8", newline="\n").write(make_brand.save_text(30, 0))
app = subprocess.Popen([os.path.join(ROOT, "dist", "YomiGo.exe"), "--root", root, "--script", "w2;ow2;ow2;M50,359;w2;M526,101;w2;N9000;"])
time.sleep(3)
wrong = post("YOMIGO-SYNC 1 9999\n" + make_brand.save_text(40, 0))
right = post("YOMIGO-SYNC 1 1234\n" + make_brand.save_text(40, 0))
app.wait(timeout=60)
kept = open(os.path.join(root, "Hana.sav"), encoding="utf-8").read()
ok = wrong[0] == 403 and right[0] == 200 and "\nclass=4001\n" in right[1] and "\nclass=4001\n" in kept  # class 40, part 1
print("sync with a phone over the home network", "OK" if ok else "FAILED", wrong[0], right[0], "class=4001" in right[1], "class=4001" in kept)
fail += not ok
shutil.rmtree(root, ignore_errors=True)
# Copy and paste, using a stand-in clipboard so the real one is left alone: "Ab" pasted after itself makes "AbAb"
root = tempfile.mkdtemp()
subprocess.run([os.path.join(ROOT, "dist", "YomiGo.exe"), "--root", root, "--script", "w2;ow2;ow2;tAb;w1;C;w1;V;w1;ow2;"], timeout=60)
ok = os.path.exists(os.path.join(root, "AbAb.sav"))
print("copy and paste", "OK" if ok else "FAILED", os.listdir(root))
fail += not ok
shutil.rmtree(root, ignore_errors=True)
# Text a Japanese keyboard finishes: type every word of day 1 in real kana. Finishing moves the class on and earns 1 XP a word.
# Focus is moved to Copy first, as a mouse resting there does: Enter must still send the answer.
esc = lambda s: "".join("\\u%04x" % ord(c) for c in s)
kana = ["おはようございます", "こんにちは", "こんばんは", "さようなら", "おやすみなさい"]
root = tempfile.mkdtemp()
C = json.load(open(os.path.join(ROOT, "build", "gen", "content.json"), encoding="utf-8"))
pages = len(C["days"][0]["notes"]) + len(C["days"][0]["new"])
script = "w2;ow2;ow2;tKana;w1;ow2;ddw1;ow2;dw1;ow2;dw1;ow2;ow2;ow2;" + "ow1;" * pages + "lw1;" + "".join(f"t{esc(k)};w1;ow1;" * 3 for k in kana)
subprocess.run([os.path.join(ROOT, "dist", "YomiGo.exe"), "--root", root, "--script", script], timeout=60)
vals = dict(l.split("=", 1) for l in open(os.path.join(root, "Kana.sav"), encoding="utf-8").read().splitlines() if "=" in l)
ok = vals.get("step") == "3" and vals.get("xp") == "5"
print("typing practice in real kana", "OK" if ok else "FAILED", "step", vals.get("step"), "xp", vals.get("xp"))
fail += not ok
shutil.rmtree(root, ignore_errors=True)
sys.exit(fail != 0)
