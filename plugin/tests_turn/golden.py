"""tests_turn/golden.py - the golden candles of Turn v1.0 through the plugin's C++ logic (tests_turn/replay = TurnLogic.h Live + decideLive).
golden_turn_v1.json holds the reviewed reference values (turnref.py, whole-second mode): the 110 past sweeps and the 9 mockup cases (5
candles each). For every candle the store's prints are fed as IRT FILL events (10 minutes before the candle for the order-ID clock) and the
C++ must return the same Stop Run lots, Effort, extreme, Exhaustion readings, FAST / HARD / BIG, marks, dem and verdict.
usage: golden.py <analytics dir with lra/turn_pane.py> <dir with seed_sweeps.csv + norms_seed.csv> <store dir> [work dir]
(needs the MBO store: run on the research machine; g++ -std=c++17 -O2 replay.cpp -o replay first)"""
import json, os, subprocess, sys
import numpy as np, pandas as pd
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, sys.argv[1]); os.environ["LRA_MBO_STORE"] = sys.argv[3]
from lra import turn_pane as TN  # noqa: E402
seed_dir, work = sys.argv[2], (sys.argv[4] if len(sys.argv) > 4 else "/tmp/turn_golden"); os.makedirs(work, exist_ok=True)
G = json.load(open(os.path.join(here, "golden_turn_v1.json")))
SEED = TN.read_ledger(os.path.join(seed_dir, "seed_sweeps.csv"))
N = pd.read_csv(os.path.join(seed_dir, "norms_seed.csv"), float_precision="round_trip", dtype={"m": str, "sess": str})
REPLAY = os.path.join(here, "replay"); NS = 10 ** 9; VN = {0: "UNCLEAR", 1: "STAND ASIDE", 2: "WAIT", 3: "BEST ODDS"}
_T = {}
def trades(m, day):
    if (m, day) not in _T:
        _T.clear(); _T[(m, day)] = TN.load_trades("", m, day)
    return _T[(m, day)]
def params(m, day):
    """the reviewed tables: the market's 110-sweep values (in-sample), the normals of the 10 sessions before `day`"""
    n15, avg = TN.norms_for(N, m, day); x = SEED[SEED.m == m]
    L = ["VERSION,1", f"MARKET,{m}", f"STOP_TYP,{TN.STOP_TYP[m]!r}", f"HARD_CUT,{TN.HARD_CUT[m]!r}", f"BIG_CUT,{TN.BIG_CUT[m]!r}", f"EXH_CUT,{TN.Q1!r},{TN.Q2!r}", "ODDS,-1,26,52,-1", "BEST_PROVEN,0",
         "N15," + ",".join(repr(float(v)) for v in n15), "AVG_ORDER," + ",".join(repr(float(v)) for v in avg)]
    for k, c in (("RANK_LATE", "late"), ("RANK_DECAY", "decay"), ("RANK_AFTER", "after")):
        L.append(k + "".join("," + repr(float(v)) for v in np.sort(x[c].dropna().values)))
    p = os.path.join(work, f"p_{m}_{day}.csv"); open(p, "w").write("\n".join(L) + "\nEND\n"); return p
def run(m, day, t1s, d, level):
    """-> (raw values dict, the decided read's 25 fields, dem)"""
    T = trades(m, day); t1 = pd.Timestamp(f"{t1s}", tz="America/Chicago").value; key = t1 // NS
    a = np.searchsorted(T.ts.values, t1 - (180 + 600) * NS, "left"); b = np.searchsorted(T.ts.values, t1, "left"); x = T.iloc[a:b]
    bs = np.where(x.ag.values > 0, 1, np.where(x.ag.values < 0, 0, 2)); f = os.path.join(work, "ev.txt")
    with open(f, "w") as o:
        for s, q, p, z, i in zip(x.sec.values, bs, x.pt.values, x.sz.values, x.oid.values):
            o.write(f"{s} 3 {q} {p} {z} 0 {i if q != 2 else 0}\n")
    c = TN.candle(T, t1); sg = -d; ie = int(np.argmax(c.pt.values * sg)); tod = TN.tod_of(int(c.ts.values[ie]))
    out = subprocess.run([REPLAY, f, "--params", params(m, day), m, str(key), str(d), str(int(level)), str(tod)], capture_output=True, text=True).stdout.strip().split("\n")[-1]
    a_, b_ = out.split(" | dem "); w = a_.split(); V = {w[i]: w[i + 1] for i in range(0, len(w), 2)}
    return V, b_.split(" | T|")[1].split("|"), float(b_.split()[0])
bad = 0; cnt = {}
def chk(tag, name, ok, got="", want=""):
    global bad
    cnt[name] = cnt.get(name, 0) + 1
    if not ok:
        bad += 1; print("MISMATCH", tag, name, "| C++", got, "| golden", want)
def close(x, y):
    return (y is None and not np.isfinite(x)) or (y is not None and abs(x - y) <= 1e-9 * max(1, abs(y)))
for g in G["sweeps"]:
    V, rd, dem = run(g["m"], g["sess"], g["t1"], int(g["dir"]), g["L"]); tag = f"{g['id']} {g['m']} {g['t1']}"
    chk(tag, "candle whole", V["complete"] == "1" and V["stopOk"] == "1" and V["thin"] == "0")
    chk(tag, "stop lots", int(V["stop"]) == g["stop_lots_sec"], V["stop"], g["stop_lots_sec"])
    chk(tag, "effort", (g["effort_sec"] is None and V["effOk"] == "0") or (g["effort_sec"] is not None and V["effOk"] == "1" and close(float(V["eff"]), g["effort_sec"])), V["eff"], g["effort_sec"])
    chk(tag, "extreme", int(V["ext"]) == g["ext_sec"] and int(V["into"]) == int(g["ext_into_s_sec"]) and int(V["left"]) == int(g["ext_left_s_sec"]))
    chk(tag, "late", close(float(V["late"]), g["late_rate_sec"]), V["late"], g["late_rate_sec"])
    chk(tag, "decay", (g["decay_sec"] is None and V["decayOk"] == "0") or (g["decay_sec"] is not None and V["decayOk"] == "1" and close(float(V["decay"]), g["decay_sec"])), V["decay"], g["decay_sec"])
    chk(tag, "after", close(float(V["after"]), g["after_buy_sec"]), V["after"], g["after_buy_sec"])
    chk(tag, "fast", int(V["fresh"]) == int(g["fast_sec"])); chk(tag, "hard lots", int(V["hard"]) == int(round(g["hard_lots_sec"]))); chk(tag, "big lots", int(V["big"]) == int(round(g["big_lots_sec"])))
    chk(tag, "dem", close(dem, g["dem"]), dem, g["dem"])
    chk(tag, "marks", int(rd[15]) == g["marks"] and int(rd[12]) == int(g["fast_sec"]) and (int(rd[13]) == 1) == bool(g["H_sec"]) and (int(rd[14]) == 1) == bool(g["B_sec"]) and bool(int(rd[16])) == bool(g["ans_ok"]), rd[12:17], g["marks"])
    chk(tag, "verdict", VN[int(rd[21])] == g["verdict"], VN[int(rd[21])], g["verdict"])
    chk(tag, "stop ratio", abs(int(rd[5]) / 1000 - g["stop_x_sec"]) <= 0.00051)
nrow = 0
for c in G["mock_cases"]:
    for r in c["rows"]:
        V, rd, dem = run(c["m"], c["day"], f"{c['day']} {r['t1']}", int(c["d"]), c["L"]); tag = f"mock {c['m']} {c['day']} {r['t1']} (Sw {c['sw']})"; nrow += 1
        if not c.get("exact_stop"):
            chk(tag, "stop lots", int(V["stop"]) == r["stop_lots"], V["stop"], r["stop_lots"])      # (with the order file the mockup left out orders seen resting: the live rule, tested in test_turn_host)
        chk(tag, "effort", (r["effort"] is None and V["effOk"] == "0") or (r["effort"] is not None and close(float(V["eff"]), r["effort"])), V["eff"], r["effort"])
        chk(tag, "extreme", int(V["ext"]) == r["ext"] and int(V["into"]) == int(r["ext_into_s"]))
        chk(tag, "late", close(float(V["late"]), r["late_rate"]), V["late"], r["late_rate"]); chk(tag, "after", close(float(V["after"]), r["after_buy"]), V["after"], r["after_buy"])
        chk(tag, "decay", (r["decay"] is None and V["decayOk"] == "0") or (r["decay"] is not None and close(float(V["decay"]), r["decay"])), V["decay"], r["decay"])
        chk(tag, "fast", int(V["fresh"]) == int(r["fast"])); chk(tag, "hard lots", int(V["hard"]) == int(round(r["hard_lots"]))); chk(tag, "big lots", int(V["big"]) == int(round(r["big_lots"])))
        through = (r["ext"] > c["L"]) if c["d"] < 0 else (r["ext"] < c["L"])                        # a read is decided only on a candle that traded through the level
        chk(tag, "read only through the level", (rd[24] == "L") == through, rd[24], through)
        if rd[24] == "L":
            chk(tag, "marks", int(rd[15]) == r["marks"], rd[15], r["marks"])
            chk(tag, "verdict", VN[int(rd[21])] == r["verdict"], VN[int(rd[21])], r["verdict"])
            chk(tag, "exhaustion word", {0: "No", 2: "Moderate", 3: "Heavy", -1: "No"}[int(rd[11])] == r["exh_word"], rd[11], r["exh_word"])
print(f"golden candles: {len(G['sweeps'])} sweeps + {nrow} mockup candles, {sum(cnt.values())} checks, {bad} mismatches")
print(cnt); sys.exit(1 if bad else 0)
