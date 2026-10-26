#!/usr/bin/env python3
"""apply_perf.py 1.0.0 (2026-10-26) -- put the IRT PERFORMANCE GUARD (PerfGuard.h) into every plugin on the build list.

What it changes, on a COPY of plugin\\ + tools\\gex-build.bat (never the staged originals):
  1. <Name>.cpp: '#include "PerfGuard.h"' before the first #include, plus '#ifndef LSPERF_PLUGIN / #define LSPERF_PLUGIN "<Name>"'
     (a wrapper such as LiquidityFlow.cpp / TrappedMarks.cpp defines its OWN name before it includes the shared .cpp).
  2. LSPERF_SCOPE(LSPERF_PLUGIN, "calc" | "draw" | "preCalc") right after the opening brace of every calc / draw / preCalc definition of
     cppExtension or of a class derived from it in that file, when the body does real work (a bare 'return RTX_OK;' or a one-statement
     forward to another calc / draw is skipped, so nothing is timed twice). One line is inserted per function: line numbers barely move.
  3. The version: the setVersion("x.y.z") literal (the build log reads it) and every other "x.y.z" literal in the same .cpp get +1 on the
     last part; a version constant passed by name (setVersion(GP_VERSION)) is bumped on its definition. A plugin whose version also sits in
     a header (e.g. tr::VERSION "keep equal") gets that header bumped only when no other plugin hashes that header; otherwise it is reported.
  4. OrderFlowFeed.h: one guarded line feeds the exchange stamp of each event to lsperf::noteEvent (feed lag); no-op without PerfGuard.h.
  5. gex-build.bat: ' PerfGuard.h' appended to every patched plugin's 'call :one' line (CRLF kept) so a PerfGuard edit rebuilds them.
Idempotent: a file that already includes PerfGuard.h is left alone. Any anchor not found = an error in the report and NO write of that file.
Usage: python3 apply_perf.py --plugin DIR --bat FILE [--write] [--report out.json]
"""
from __future__ import annotations

import argparse
import json
import os
import re
import sys

VERSION = "1.0.0"
TODAY = "2026-10-26"
FNS = ("calc", "draw", "preCalc")
DEF_RE = re.compile(r"^([ \t]*)(?:virtual[ \t]+)?int[ \t]+(?:(\w+)[ \t]*::[ \t]*)?(calc|draw|preCalc)[ \t]*\(([^)]*)\)\s*(?:const\s*)?(?:override\s*)?\{", re.M)
CLASS_RE = re.compile(r"\b(?:class|struct)\s+(\w+)\s*(?:final\s*)?:\s*(?:public\s+|virtual\s+)*cppExtension\b")
CAST_RE = re.compile(r"static_cast\s*<\s*(\w+)\s*\*\s*>\s*\(\s*this\s*\)")
TRIVIAL_RE = re.compile(r"^\s*return\s+RTX_OK\s*;\s*$")
FORWARD_RE = re.compile(r"^\s*return\s+[^;]*\b(?:calc|draw|preCalc)\s*\([^;]*\)\s*;\s*$")
OFF_ANCHOR = "evTs = e.stamp > 0 ? e.stamp : e.arrMs / 1000;"
OFF_LINE = "#ifdef LSPERF_H\n        lsperf::noteEvent(e.stamp);                                        // (PerfGuard 1.0.0) feed lag\n#endif\n"


def strip_comments_strings(src: str) -> str:
    """same length text with comments / string / char literals blanked (so brace matching ignores them)"""
    out = list(src); i, n = 0, len(src)
    while i < n:
        c = src[i]
        if c == "/" and i + 1 < n and src[i + 1] == "/":
            j = src.find("\n", i); j = n if j < 0 else j
            for k in range(i, j): out[k] = " "
            i = j
        elif c == "/" and i + 1 < n and src[i + 1] == "*":
            j = src.find("*/", i + 2); j = n if j < 0 else j + 2
            for k in range(i, j):
                if out[k] != "\n": out[k] = " "
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and src[j] != c:
                j += 2 if src[j] == "\\" else 1
            for k in range(i + 1, min(j, n)):
                if out[k] != "\n": out[k] = " "
            i = j + 1
        else:
            i += 1
    return "".join(out)


def body_of(clean: str, open_idx: int) -> tuple[int, int]:
    """(start after '{', index of the matching '}') or (-1, -1)"""
    depth = 0
    for k in range(open_idx, len(clean)):
        if clean[k] == "{": depth += 1
        elif clean[k] == "}":
            depth -= 1
            if depth == 0: return open_idx + 1, k
    return -1, -1


def find_sites(src: str) -> tuple[list[dict], list[str]]:
    clean = strip_comments_strings(src)
    # extension classes: derived from cppExtension here, or the target of static_cast<X*>(this) (indirect bases such as off::MboPane<>)
    derived = set(CLASS_RE.findall(clean)) | set(CAST_RE.findall(clean)) | {"cppExtension"}
    bodies = []   # (start, end, class) of every extension class body: an in-class definition there is timed too
    for name in derived - {"cppExtension"}:
        for cm in re.finditer(r"\b(?:class|struct)\s+" + name + r"\b[^;{]*\{", clean):
            a, b = body_of(clean, cm.end() - 1)
            if a > 0: bodies.append((a, b, name))
    sites, skipped = [], []
    for m in DEF_RE.finditer(clean):
        cls, fn = m.group(2), m.group(3)
        if cls is None:
            inside = [c for (a, b, c) in bodies if a <= m.start() < b]
            if not inside:
                skipped.append(f"{fn} (free definition at offset {m.start()}: not an extension member)"); continue
            cls = inside[-1]
        if cls not in derived:
            skipped.append(f"{cls}::{fn} (not an extension class)"); continue
        a, b = body_of(clean, m.end() - 1)
        if a < 0:
            skipped.append(f"{cls}::{fn} (no matching brace)"); continue
        body = clean[a:b]
        if TRIVIAL_RE.match(body) or FORWARD_RE.match(body) or not body.strip():
            skipped.append(f"{cls}::{fn} (trivial / forward)"); continue
        if "LSPERF_SCOPE" in src[a:b]:
            skipped.append(f"{cls}::{fn} (already timed)"); continue
        sites.append({"cls": cls, "fn": fn, "at": a, "line": src.count("\n", 0, a) + 1})
    return sites, skipped


def include_point(src: str) -> int:
    """offset to insert at: after the top-level #include "irtsdk.h" line, else at the first top-level #include; -1 = none"""
    depth, off, first = 0, 0, -1
    for line in src.splitlines(keepends=True):
        t = line.strip()
        if re.match(r"#\s*if", t): depth += 1
        elif re.match(r"#\s*endif", t): depth = depth - 1 if depth > 0 else 0
        elif depth == 0 and re.match(r"#\s*include\b", t):
            if '"irtsdk.h"' in t or "<irtsdk.h>" in t: return off + len(line)
            if first < 0: first = off
        off += len(line)
    return first


def bump(v: str) -> str:
    p = v.split("."); p[-1] = str(int(p[-1]) + 1).zfill(len(p[-1])) if p[-1].isdigit() else p[-1]
    return ".".join(p)


def find_version(src: str) -> tuple[str | None, str | None]:
    """(version literal, how) from setVersion("x") or setVersion(NAME) + NAME's definition"""
    m = re.search(r'setVersion\(\s*"(\d+(?:\.\d+)+)"\s*\)', src)
    if m: return m.group(1), "setVersion literal"
    m = re.search(r"setVersion\(\s*(\w+)\s*\)", src)
    if m:
        name = m.group(1)
        d = re.search(r'(?:#define\s+' + name + r'\s+|\b' + name + r'\s*=\s*)"(\d+(?:\.\d+)+)"', src)
        if d: return d.group(1), f"constant {name}"
    return None, None


def read_bat(path: str) -> list[str]:
    raw = open(path, "rb").read()
    if b"\r\n" not in raw: raise SystemExit("gex-build.bat is not CRLF: stop (it must stay CRLF)")
    return raw.decode("latin-1").split("\r\n")


def build_lines(lines: list[str]) -> dict[str, tuple[int, list[str]]]:
    out = {}
    for i, l in enumerate(lines):
        t = l.split()
        if len(t) >= 3 and t[0].lower() == "call" and t[1].lower() == ":one":
            out[t[2]] = (i, t[3:])
    return out


def apply(plugin_dir: str, bat_path: str, write: bool) -> dict:
    rep = {"version": VERSION, "write": write, "plugins": {}, "headers": {}, "bat": {}, "errors": [], "warnings": []}
    lines = read_bat(bat_path)
    bl = build_lines(lines)
    # who hashes which header (to decide whether a header-held version can be bumped)
    users: dict[str, set] = {}
    for name, (_, toks) in bl.items():
        for h in toks[1:]: users.setdefault(h, set()).add(name)
    new_files: dict[str, str] = {}
    patched = []
    queue = [(n, t, True) for n, (_, t) in bl.items()]   # + shared sources only reached through wrappers (TapeFlow.cpp)
    qi = 0
    while qi < len(queue):
        name, toks, on_build = queue[qi]; qi += 1
        p = os.path.join(plugin_dir, name + ".cpp")
        if not os.path.exists(p):
            rep["plugins"][name] = {"status": "no source here (skipped)"}; continue
        src = open(p, encoding="utf-8", errors="surrogateescape", newline="").read()
        info = {"status": "", "sites": [], "skipped": [], "version": None}
        if '#include "PerfGuard.h"' in src:
            info["status"] = "already has PerfGuard.h"; rep["plugins"][name] = info
            if on_build: patched.append(name)
            continue
        inc = re.search(r'^#include\s+"([A-Za-z0-9_]+\.cpp)"', src, re.M)
        nl = "\r\n" if "\r\n" in src else "\n"
        if inc:
            shared = inc.group(1)[:-4]
            if shared not in bl and all(q[0] != shared for q in queue):
                queue.append((shared, toks, False))   # the shared source is instrumented once (its LSPERF_PLUGIN is #ifndef-guarded)
        if inc and "LSPERF_PLUGIN" in src:
            info["status"] = "wrapper: LSPERF_PLUGIN already set"; rep["plugins"][name] = info; patched.append(name); continue
        if inc:   # a wrapper (LiquidityFlow.cpp, TrappedMarks.cpp): its own name, then the shared .cpp
            new = src[:inc.start()] + f'#define LSPERF_PLUGIN "{name}"   // (PerfGuard 1.0.0) this DLL\'s name in the perf records{nl}' + src[inc.start():]
            info["status"] = f"wrapper of {inc.group(1)}: LSPERF_PLUGIN set"
            new_files[p] = new; rep["plugins"][name] = info; patched.append(name); continue
        sites, skipped = find_sites(src)
        info["skipped"] = skipped
        if not sites:
            info["status"] = "no calc / draw body found"; rep["warnings"].append(f"{name}: no calc / draw body to time"); rep["plugins"][name] = info; continue
        # 2. scopes, from the end so offsets hold
        new = src
        for s in sorted(sites, key=lambda s: -s["at"]):
            new = new[:s["at"]] + f' LSPERF_SCOPE(LSPERF_PLUGIN, "{s["fn"]}");' + new[s["at"]:]
            info["sites"].append(f'{s["cls"]}::{s["fn"]} line {s["line"]}')
        info["sites"].reverse()
        # 1. the include + the name: right AFTER the top-level '#include "irtsdk.h"' (windows.h / NOMINMAX set-up is done by then),
        #    else before the first top-level #include (never inside an #if block)
        at = include_point(new)
        if at < 0:
            rep["errors"].append(f"{name}: no top-level #include line"); continue
        head = (f'#include "PerfGuard.h"   // (PerfGuard 1.0.0, {TODAY}) calc / draw timing, memory, lag, budgets -> lsFlexLevels\\Perf{nl}'
                f'#ifndef LSPERF_PLUGIN{nl}#define LSPERF_PLUGIN "{name}"{nl}#endif{nl}')
        new = new[:at] + head + new[at:]
        # 3. version
        v, how = find_version(new)
        if v:
            nv = bump(v); cnt = new.count(f'"{v}"')
            new = new.replace(f'"{v}"', f'"{nv}"')
            info["version"] = f"{v} -> {nv} ({how}, {cnt} literal(s))"
            # the same version mirrored in this plugin's own headers (PLUGIN_VERSION in <Name>Settings.h, VERSION in <Name>Logic.h,
            # BAS_VERSION in a shared pane header): bumped only on a line whose constant belongs to THIS plugin, never a coincidence
            macros = set()
            for mm in re.finditer(r'(?:#define\s+(\w+)\s+|\b(\w+)\s*=\s*)"' + re.escape(v) + '"', src):
                macros.add(mm.group(1) or mm.group(2))
            for h in toks[1:]:
                hp = os.path.join(plugin_dir, h)
                if not h.endswith(".h") or not os.path.exists(hp): continue
                ht = new_files.get(hp) or open(hp, encoding="utf-8", errors="surrogateescape", newline="").read()
                stem = re.sub(r"(Logic|Settings|Engine|Core)$", "", h[:-2])
                own = name.startswith(stem) or stem.startswith(name)
                pat = re.compile(r'^(.*\b(\w+)\s*=\s*)"' + re.escape(v) + r'"(.*)$', re.M)
                def sub(mm):
                    cname, rest = mm.group(2), mm.group(3)
                    if cname in macros or (own and (cname in ("VERSION", "PLUGIN_VERSION") or cname.endswith("_VERSION"))) or "keep equal" in rest:
                        return mm.group(1) + f'"{nv}"' + rest
                    return mm.group(0)
                nt = pat.sub(sub, ht)
                if nt != ht:
                    new_files[hp] = nt; rep["headers"][h] = (rep["headers"].get(h, "") + f" version {v} -> {nv} ({name})").strip()
        else:
            rep["warnings"].append(f"{name}: no setVersion version found: not bumped")
        info["status"] = f"{len(sites)} function(s) timed" + ("" if on_build else " (shared source of wrappers)")
        new_files[p] = new; rep["plugins"][name] = info
        if on_build: patched.append(name)
    # 4. OrderFlowFeed.h lag hook
    ofp = os.path.join(plugin_dir, "OrderFlowFeed.h")
    if os.path.exists(ofp):
        t = open(ofp, encoding="utf-8", errors="surrogateescape", newline="").read()
        if "lsperf::noteEvent" in t: rep["headers"]["OrderFlowFeed.h"] = "lag hook already there"
        elif t.count(OFF_ANCHOR) != 1: rep["errors"].append("OrderFlowFeed.h: the evTs anchor is not there exactly once (source moved): lag hook NOT added")
        else:
            i = t.index(OFF_ANCHOR); j = t.index("\n", i) + 1
            nl = "\r\n" if t[j - 2:j] == "\r\n" else "\n"
            new_files[ofp] = t[:j] + OFF_LINE.replace("\n", nl) + t[j:]
            rep["headers"]["OrderFlowFeed.h"] = "lag hook added (guarded by #ifdef LSPERF_H)"
    # 5. build lines
    for name in patched:
        i, toks = bl[name]
        if "PerfGuard.h" in toks: rep["bat"][name] = "already listed"; continue
        lines[i] = lines[i].rstrip() + " PerfGuard.h"; rep["bat"][name] = "PerfGuard.h added"
    if rep["errors"]:
        rep["written"] = False
        return rep
    if write:
        for p, t in new_files.items():
            with open(p, "w", encoding="utf-8", errors="surrogateescape", newline="") as f: f.write(t)
        with open(bat_path, "wb") as f: f.write("\r\n".join(lines).encode("latin-1"))
        import shutil
        shutil.copyfile(os.path.join(os.path.dirname(os.path.abspath(__file__)), "PerfGuard.h"), os.path.join(plugin_dir, "PerfGuard.h"))
    rep["written"] = write
    rep["files"] = sorted(os.path.relpath(p, plugin_dir) for p in new_files) + ["PerfGuard.h"]
    return rep


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--plugin", required=True); ap.add_argument("--bat", required=True)
    ap.add_argument("--write", action="store_true"); ap.add_argument("--report")
    a = ap.parse_args(argv)
    rep = apply(a.plugin, a.bat, a.write)
    if a.report:
        with open(a.report, "w") as f: json.dump(rep, f, indent=1)
    timed = sum(1 for v in rep["plugins"].values() if "timed" in v.get("status", "") or "wrapper" in v.get("status", ""))
    print(f"apply_perf {VERSION}: {timed} plugins instrumented, {len(rep['warnings'])} warnings, {len(rep['errors'])} errors, written={rep.get('written')}")
    for e in rep["errors"]: print("ERROR", e)
    for w in rep["warnings"]: print("warn ", w)
    return 1 if rep["errors"] else 0


if __name__ == "__main__":
    sys.exit(main())
