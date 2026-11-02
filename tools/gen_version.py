#!/usr/bin/env python3
"""VERSION is the single source of the version number."""
import io, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
v = io.open(os.path.join(ROOT, "VERSION")).read().strip()
parts = v.split(".")
assert len(parts) == 3 and all(p.isdigit() for p in parts), "VERSION must look like 0.1.0"
with io.open(os.path.join(ROOT, "build", "gen", "version_gen.h"), "w", newline="\n") as f:
    f.write(f'#define YG_VERSION "{v}"\n#define YG_REPO "knivesbatz/YomiGO"\n')
