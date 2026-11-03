#!/usr/bin/env python3
"""Turn tests/vectors.json into a C header, so the C core and the JS core check the same cases."""
import io, json, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
v = json.load(io.open(os.path.join(ROOT, "tests", "vectors.json"), encoding="utf-8"))
lines = ["static const struct { const char *a, *b; } NORM[] = {"]
lines += ['{"%s","%s"},' % (a, b) for a, b in v["norm"]]
lines += ["};", "static const struct { int r, iv, e, g, wi, we, wr; } SRS[] = {"]
lines += ["{%d,%d,%d,%d,%d,%d,%d}," % (*s["start"], s["grade"], s["ivl"], s["ease"], s["reps"]) for s in v["srs"]]
lines += ["};", 'static const char MYLINE[] = %s;' % json.dumps(v["myline"], ensure_ascii=False)]
with io.open(os.path.join(ROOT, "build", "gen", "vectors_gen.h"), "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(lines) + "\n")
