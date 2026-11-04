#!/usr/bin/env python3
"""VERSION is the single source of the version number."""
import io, os, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
v = io.open(os.path.join(ROOT, "VERSION")).read().strip()
parts = v.split(".")
assert len(parts) == 3 and all(p.isdigit() for p in parts), "VERSION must look like 0.1.0"
os.makedirs(os.path.join(ROOT, "build", "gen"), exist_ok=True)
for p, pat, rep in (("desktop/Cargo.toml", r'(?m)^version = "[^"]*"', f'version = "{v}"'), ("desktop/tauri.conf.json", r'"version": "[^"]*"', f'"version": "{v}"')):
    p = os.path.join(ROOT, p); t = io.open(p, encoding="utf-8").read(); n = re.sub(pat, rep, t, count=1)
    if n != t: io.open(p, "w", encoding="utf-8", newline="\n").write(n)
with io.open(os.path.join(ROOT, "build", "gen", "version_gen.h"), "w", newline="\n") as f:
    f.write(f'#define YG_VERSION "{v}"\n#define YG_REPO "knivesbatz/YomiGO"\n')
