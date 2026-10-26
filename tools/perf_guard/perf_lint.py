#!/usr/bin/env python3
"""perf_lint.py 1.0.0 (2026-10-26) -- build-time performance checks for the IRT plugins (PerfGuard part 3).

Static, heuristic checks of every plugin .cpp on the build list (and the headers it hashes), looking at what IRT runs on EVERY call:
  P1 full-history loop in calc / draw: a loop bounded by getBarCount() / the bar count inside a calc / draw body (calc must only do the
     new bar; draw only the visible range)
  P2 file I/O in calc / draw: fopen / ofstream / ifstream / CreateFile / getline directly in the body (writes must be batched, reads cached)
  P3 board write in a loop: bbSet / SetEnvironmentVariableA / mst::publish inside a for / while body (one write per pass, not per item)
  P4 possibly unbounded buffer: a member container that is push_back'ed / emplace_back'ed / inserted with no size cap, erase, pop or
     clear anywhere in the same file (bounded buffers)
  P6 board writer without a cap: a file that writes the board (bbSet / mst::publish) with no size cap at all
  P5 no work cap per pass: a calc / draw body with a loop and no 'max' / cap / budget / limit word in the file (BUILD-LESSONS #40)
Each finding has a stable key (rule|file|function|snippet hash). A BASELINE (perf-lint-baseline.json) records today's findings = the
"before" number; --gate fails only on findings NOT in the baseline, so a new build cannot add a performance smell silently.
Usage: python3 perf_lint.py --plugin DIR --bat FILE [--baseline B.json] [--write-baseline] [--gate] [--json out.json] [--md out.md]
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from apply_perf import DEF_RE, body_of, build_lines, read_bat, strip_comments_strings  # noqa: E402

VERSION = "1.0.0"
LOOP_RE = re.compile(r"\b(for|while)\s*\(")
P1_UNUSED = re.compile(r"\b(for|while)\s*\([^;{)]*;[^;{]*\b(getBarCount\s*\(\s*\)|barCount|nBars|bars\b|BarCount)")
IO_RE = re.compile(r"\b(fopen|_wfopen|std::ofstream|std::ifstream|ofstream|ifstream|CreateFileA?W?|std::getline|fgets|fread|fwrite|fprintf)\b")
BOARD_RE = re.compile(r"\b(bbSet|SetEnvironmentVariableA|mst::publish)\s*\(")
PUSH_RE = re.compile(r"(\b[A-Za-z_]\w*\s*(?:\.|->)\s*)?\b([A-Za-z_]\w*)\s*(?:\.|->)\s*(push_back|emplace_back|insert|emplace|push)\s*\(")
BARS_VAR_RE = re.compile(r"\b(\w+)\s*=\s*(?:\(\s*\w+\s*\)\s*)?(?:\w+\s*->\s*|\w+\s*\.\s*)?getBarCount\s*\(\s*\)")
CAP_WORDS = re.compile(r"\b(MAX_\w*|\w*_MAX\b|\w*[Cc]ap\w*|\w*[Ll]imit\w*|\w*[Bb]udget\w*|kMax\w*)")


def loops_in(body: str) -> list[tuple[int, int]]:
    """(start, end) of each loop body inside body (brace bodies only; single-statement loops = to the next ';')"""
    out = []
    for m in LOOP_RE.finditer(body):
        depth, k = 0, m.end() - 1
        while k < len(body):
            if body[k] == "(": depth += 1
            elif body[k] == ")":
                depth -= 1
                if depth == 0: break
            k += 1
        j = k + 1
        while j < len(body) and body[j] in " \t\r\n": j += 1
        if j < len(body) and body[j] == "{":
            a, b = body_of(body, j)
            if a > 0: out.append((m.start(), b))
        else:
            e = body.find(";", j); out.append((m.start(), e if e > 0 else len(body)))
    return out


def key(rule: str, f: str, fn: str, snip: str) -> str:
    return f"{rule}|{f}|{fn}|" + hashlib.sha1(re.sub(r"\s+", " ", snip).encode()).hexdigest()[:10]


def lint_file(path: str, rel: str) -> list[dict]:
    src = open(path, encoding="utf-8", errors="replace").read()
    clean = strip_comments_strings(src)
    out = []
    def add(rule, fn, at, snip, msg):
        line = src.count("\n", 0, at) + 1
        out.append({"rule": rule, "file": rel, "fn": fn, "line": line, "msg": msg, "snippet": snip.strip()[:140], "key": key(rule, rel, fn, snip)})
    has_cap = bool(CAP_WORDS.search(clean))
    for m in DEF_RE.finditer(clean):
        cls, fn = m.group(2) or "", m.group(3)
        if fn not in ("calc", "draw"): continue
        a, b = body_of(clean, m.end() - 1)
        if a < 0: continue
        body = clean[a:b]; name = f"{cls}::{fn}" if cls else fn
        bars_vars = set(BARS_VAR_RE.findall(body))
        for lm in re.finditer(r"\bfor\s*\(([^;]*);([^;]*);", body):
            init, cond = lm.group(1), lm.group(2)
            uses = "getBarCount" in cond or any(re.search(r"\b" + re.escape(v) + r"\b", cond) for v in bars_vars)
            if uses and re.search(r"=\s*0\b|=\s*0L\b", init):
                add("P1", name, a + lm.start(), body[lm.start():lm.start() + 120], "loop over ALL bars inside " + fn + " (only new bars in calc / the visible range in draw)")
        for im in IO_RE.finditer(body):
            add("P2", name, a + im.start(), body[im.start():im.start() + 100], "file I/O directly in " + fn + " (batch writes on a timer / cache reads by file stamp)")
        lps = loops_in(body)
        for (s, e) in lps:
            for bm in BOARD_RE.finditer(body, s, e):
                add("P3", name, a + bm.start(), body[bm.start():bm.start() + 100], "board write inside a loop (one write per pass)")
        if lps and not has_cap:
            add("P5", name, a, body[:80], "loops in " + fn + " and no cap / limit / budget anywhere in the file (cap the work per pass)")
    # P6: board writers must bound what they write (a size cap on the value / the number of entries)
    bm = BOARD_RE.search(clean)
    if bm and not re.search(r"\b(MAX_\w*|\w*_MAX\b|\w*[Cc]ap\w*)\b|\.size\s*\(\s*\)\s*(>|>=|<|<=)|\bsubstr\s*\(\s*0\s*,", clean):
        add("P6", "-", bm.start(), clean[bm.start():bm.start() + 80], "writes the board with no size cap anywhere in the file (board / index / question caps)")
    # P4: containers grown with no cap / erase / pop / clear in the same file
    grown = {}
    for pm in PUSH_RE.finditer(clean):
        qual, name = pm.group(1), pm.group(2)
        member = bool(qual) or name.endswith("_") or name.startswith("m_")   # a field of a state object / a member: lives across calls
        if member: grown.setdefault(name, pm.start())
    for name, at in grown.items():
        if name in ("std", "this", "rows", "out", "o", "row"): continue
        if re.search(r"\b" + re.escape(name) + r"\s*(?:\.|->)\s*(size\s*\(\s*\)|erase|pop_front|pop_back|pop|clear|resize|assign|shrink_to_fit|swap)", clean) \
           or re.search(r"(<|>|>=|<=)\s*" + re.escape(name) + r"\s*(?:\.|->)\s*size\s*\(", clean) \
           or re.search(r"\b" + re.escape(name) + r"\s*=[^=]", clean):
            continue
        add("P4", "-", at, clean[at:at + 80], f"'{name}' grows (push / insert) with no size cap, erase, pop or clear in this file")
    return out


def lint(plugin_dir: str, bat_path: str) -> dict:
    bl = build_lines(read_bat(bat_path))
    files = {}
    for name, (_, toks) in bl.items():
        p = os.path.join(plugin_dir, name + ".cpp")
        if os.path.exists(p): files[name + ".cpp"] = p
        for h in toks[1:]:
            hp = os.path.join(plugin_dir, h)
            if h.endswith(".h") and os.path.exists(hp) and h != "PerfGuard.h": files[h] = hp
    findings = []
    for rel in sorted(files):
        findings += lint_file(files[rel], rel)
    by_rule = {}
    for f in findings: by_rule[f["rule"]] = by_rule.get(f["rule"], 0) + 1
    return {"version": VERSION, "files": len(files), "findings": findings, "by_rule": by_rule}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--plugin", required=True); ap.add_argument("--bat", required=True)
    ap.add_argument("--baseline"); ap.add_argument("--write-baseline", action="store_true"); ap.add_argument("--gate", action="store_true")
    ap.add_argument("--json"); ap.add_argument("--md")
    a = ap.parse_args(argv)
    rep = lint(a.plugin, a.bat)
    base = set()
    if a.baseline and os.path.exists(a.baseline):
        base = set(json.load(open(a.baseline)).get("keys", []))
    new = [f for f in rep["findings"] if f["key"] not in base]
    rep["new"] = new
    if a.write_baseline and a.baseline:
        json.dump({"version": VERSION, "keys": sorted(f["key"] for f in rep["findings"]), "by_rule": rep["by_rule"]}, open(a.baseline, "w"), indent=1)
    if a.json: json.dump(rep, open(a.json, "w"), indent=1)
    if a.md:
        with open(a.md, "w") as f:
            f.write(f"# Perf lint {VERSION}\n\n{rep['files']} files, {len(rep['findings'])} findings {rep['by_rule']}, {len(new)} new vs baseline.\n\n")
            f.write("| rule | file | function | line | what |\n|---|---|---|---|---|\n")
            for x in sorted(rep["findings"], key=lambda x: (x["rule"], x["file"], x["line"])):
                f.write(f"| {x['rule']}{' NEW' if x in new else ''} | {x['file']} | {x['fn']} | {x['line']} | {x['msg']} |\n")
    print(f"perf_lint {VERSION}: {rep['files']} files, {len(rep['findings'])} findings {rep['by_rule']}, {len(new)} new vs baseline")
    if a.gate and new and base:
        for x in new: print("NEW", x["rule"], x["file"], x["fn"], x["line"], x["msg"])
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
