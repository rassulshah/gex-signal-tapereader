#!/usr/bin/env python3
"""check_draw_io.py - THE SPEED CHECK BEFORE DELIVERY (2026-11-11, Rassul "its important that all the indicators and how they are processed
are improved / enhanced as part of the nightly improvement process"; BUILD-LESSONS: lsLimitProfile 2.64.0).

Runs an indicator's Linux test harness twice in its PERF mode (env PERF_DRAWS=N: the harness draws its standard scene N times with the
clock advancing 1 s a draw, then exits 0) under `strace -f -c`, and fails when a redraw touches the disk:

    per redraw:  file opens for writing / creates, renames, unlinks  <= 0.05      (one status write a minute is 0.017)
                 writes                                              <= 0.10
                 stats                                               <= 3         (reported; a stat is cheap but should not grow)

It also prints the harness's own "PERF ..." lines (ms per draw on the small and the 6,580-candle scene) and fails when the big scene
costs more than --grow times the small one AND 0.5 ms+ a draw (work that grows with the candles on screen).

usage:  python3 tests_perf/check_draw_io.py [--grow 6] -- ./t2640
A harness without PERF mode fails here: add it (see test_dealerprofile_2640.cpp H / tests_step/t_step.cpp).
"""
import os, re, subprocess, sys, tempfile

LIMITS = {"creates": 0.05, "rename": 0.05, "unlink": 0.05, "write": 0.10}
N1, N2 = 50, 450
GROW_MIN_MS = 0.5        # growth with the candles on screen fails only when the big scene costs 0.5 ms+ of the indicator's own work


def run(cmd, n):
    with tempfile.NamedTemporaryFile(suffix=".strace", delete=False) as f:
        out = f.name
    env = dict(os.environ, PERF_DRAWS=str(n))
    p = subprocess.run(["strace", "-f", "-c", "-o", out] + cmd, env=env, capture_output=True, text=True)
    calls = {}
    for ln in open(out, errors="replace"):
        a = ln.split()
        if len(a) >= 5 and a[3].isdigit() and re.match(r"^[a-z_0-9]+$", a[-1]):
            calls[a[-1]] = int(a[3])
    os.unlink(out)
    return p.returncode, p.stdout, calls


def main(argv):
    grow = 6.0
    if "--grow" in argv:
        i = argv.index("--grow"); grow = float(argv[i + 1]); del argv[i:i + 2]
    if "--" not in argv or not argv[argv.index("--") + 1:]:
        print(__doc__); return 2
    cmd = argv[argv.index("--") + 1:]
    rc1, _, c1 = run(cmd, N1); rc2, out2, c2 = run(cmd, N2)
    perf = [ln for ln in out2.splitlines() if ln.startswith("PERF ")]
    if rc1 or rc2 or not perf:
        print(f"FAIL the harness has no PERF mode or failed in it (exit {rc1} / {rc2}, {len(perf)} PERF lines): {' '.join(cmd)}"); return 1
    loops = max(1, sum(1 for ln in perf if " ms/draw" in ln))
    per = lambda k: (c2.get(k, 0) - c1.get(k, 0)) / float((N2 - N1) * loops)   # noqa: E731
    got = {"creates": per("openat") + per("open") + per("creat") - per("read") * 0, "rename": per("rename") + per("renameat") + per("renameat2"),
           "unlink": per("unlink") + per("unlinkat"), "write": per("write") + per("pwrite64") + per("writev")}
    stats = per("newfstatat") + per("stat") + per("statx") + per("lstat")
    bad = [k for k, v in got.items() if v > LIMITS[k]]
    ms = [float(m.group(1)) for m in (re.search(r"([0-9.]+) ms/draw", ln) for ln in perf) if m]
    grew = len(ms) >= 2 and min(ms) > 0 and max(ms) / min(ms) > grow and max(ms) >= GROW_MIN_MS
    for ln in perf:
        print(ln)
    print("per redraw: " + ", ".join(f"{k} {v:.3f} (limit {LIMITS[k]:g})" for k, v in got.items()) + f", stats {stats:.2f}")
    if stats > 3:
        bad.append("stats")
    if grew:
        print(f"FAIL the big scene costs {max(ms) / min(ms):.1f}x the small one (limit {grow:g}x): work grows with the candles on screen")
    if bad:
        print("FAIL a redraw touches the disk: " + ", ".join(bad))
    if bad or grew:
        return 1
    print("PASS the speed check")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
