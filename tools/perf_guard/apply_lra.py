#!/usr/bin/env python3
"""apply_lra.py 1.0.0 (2026-10-26) -- enrol the IRT performance guard in the LRA master loop, by anchors on FRESH copies:
  analytics/lra/engines.py  + engine 'irt_performance' (before the "judges" entry), VERSION patch +1
  analytics/lra/learn.py    + item 'irt_perf' (first entry of ITEMS), VERSION patch +1
  data/self_improve/parts.json + the part "IRT performance guard (PerfGuard)" -> irt_perf:nightly
  analytics/lra/irt_perf.py, analytics/lra/test_irt_perf.py copied in
A missing anchor = an error and nothing written. Idempotent.  Usage: apply_lra.py <repo-copy> [--write]
"""
import json, os, re, shutil, sys

HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
ENGINE_LINE = ('    "irt_performance": ("IRT performance guard (PerfGuard.h in every plugin + lra.irt_perf)", "PerfGuard.h, lra.irt_perf, perf_lint",\n'
               '                        ["irt_perf", "2eng.irt_performance"], "share of indicators inside their calc / draw budget; memory slope; lag at the open"),\n')
ITEM_LINE = ('    "irt_perf": "IRT performance (PerfGuard): calc / draw time per indicator vs budget, smart features auto-off, IRT memory '
             'trend / leak, feed lag at the open, settings review (days loaded, hidden charts, duplicates), regressions + rollback",\n'
             '    "2eng.irt_performance": "Engine: IRT performance guard (registry data/knowledge/engines.json id \'irt_performance\'; champion / challengers / upgrade queue)",\n')
PART = {"part": "IRT performance guard (PerfGuard)", "area": "IRT + LRA", "nightly": "irt_perf:nightly",
        "score": ["data/irt_perf/history.csv"], "challenge": ["data/irt_perf/scorecard_*.json"],
        "judge": ["data/irt_perf/scorecard_latest.json"], "deploy": ["{gex}/PerfGuard.h"],
        "tests": ["analytics/lra/test_irt_perf.py"],
        "note": "2026-10-26 Rassul 13:49 'ok. do it': per-indicator calc/draw p95 vs budget, memory leak flag, lag at the open, "
                "settings review, regression -> rollback proposal (needs his yes)"}


def bump_version(t, name):
    m = re.search(r'^VERSION = "([^"\n]*?)(\d+)\.(\d+)\.(\d+)', t, re.M)   # "1.0.0" or "learn 1.0.5 (...)"
    if not m: raise SystemExit(f"{name}: no VERSION line")
    nv = f'{m.group(2)}.{m.group(3)}.{int(m.group(4)) + 1}'
    return t[:m.start()] + f'VERSION = "{m.group(1)}{nv}' + t[m.end():], nv


def apply(repo, write):
    rep = {"changes": [], "errors": []}
    ep = os.path.join(repo, "analytics", "lra", "engines.py"); lp = os.path.join(repo, "analytics", "lra", "learn.py")
    pp = os.path.join(repo, "data", "self_improve", "parts.json")
    et, lt = open(ep, encoding="utf-8").read(), open(lp, encoding="utf-8").read()
    new = {}
    if '"irt_performance"' in et: rep["changes"].append("engines.py: already enrolled")
    elif et.count('    "judges": (') != 1: rep["errors"].append("engines.py: the judges anchor moved")
    else:
        t = et.replace('    "judges": (', ENGINE_LINE + '    "judges": (', 1); t, v = bump_version(t, "engines.py"); new[ep] = t
        rep["changes"].append(f"engines.py: + irt_performance, VERSION {v}")
    if '"irt_perf"' in lt: rep["changes"].append("learn.py: item already there")
    elif lt.count("ITEMS = {\n") != 1: rep["errors"].append("learn.py: the ITEMS anchor moved")
    else:
        t = lt.replace("ITEMS = {\n", "ITEMS = {\n" + ITEM_LINE, 1); t, v = bump_version(t, "learn.py"); new[lp] = t
        rep["changes"].append(f"learn.py: + item irt_perf, VERSION {v}")
    raw = open(pp, encoding="utf-8").read(); pj = json.loads(raw)
    if any(p.get("nightly") == PART["nightly"] for p in pj.get("parts", [])): rep["changes"].append("parts.json: already there")
    else:
        pj["parts"].append(PART); new[pp] = json.dumps(pj, indent=1) + ("\n" if raw.endswith("\n") else "")
        rep["changes"].append("parts.json: + IRT performance guard")
    if rep["errors"]: return rep
    if write:
        for p, t in new.items():
            with open(p, "w", encoding="utf-8", newline="") as f: f.write(t)
        shutil.copyfile(os.path.join(ROOT, "lra", "irt_perf.py"), os.path.join(repo, "analytics", "lra", "irt_perf.py"))
        shutil.copyfile(os.path.join(ROOT, "lra", "test_irt_perf.py"), os.path.join(repo, "analytics", "lra", "test_irt_perf.py"))
        fx = os.path.join(repo, "analytics", "fixtures", "irt_perf")
        if os.path.exists(fx): shutil.rmtree(fx)
        shutil.copytree(os.path.join(ROOT, "tests", "fixtures", "perf_e2e"), fx)
    return rep


if __name__ == "__main__":
    r = apply(sys.argv[1], "--write" in sys.argv); print(json.dumps(r, indent=1)); sys.exit(1 if r["errors"] else 0)
