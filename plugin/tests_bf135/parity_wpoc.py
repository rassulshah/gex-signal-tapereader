"""parity_wpoc.py -- lra.book_level_combo 1.3.0 read_wpoc_records + wpoc_lines == lsBookFlow 1.35.0 blb::wpocLines (C++) on the ES
10-29 / 10-30 fixture at every closed bar; wpoc_windows on 10-30 = the C++ sets bar by bar.
usage: python parity_wpoc.py <path to book_level_combo.py> <dump dir from dump_wpoc>"""
import importlib.util, sys
spec = importlib.util.spec_from_file_location("blc", sys.argv[1]); M = importlib.util.module_from_spec(spec); spec.loader.exec_module(M)
d = sys.argv[2]
recs = M.read_wpoc_records(d, "ES", 0.25)
bars = []
for ln in open(f"{d}/bars.csv"):
    t, h, l, c = ln.strip().split(","); bars.append({"t": int(t), "h": float(h), "l": float(l), "c": float(c)})
cpp = {}
for ln in open(f"{d}/cpp_lines.csv"):
    f = ln.strip().split(","); cpp[int(f[0])] = sorted(f[1:])
n = max(recs, key=lambda r: r["seq"])["n"]
same = bad = 0
TB = [{"t": b["t"], "h": round(b["h"] / .25), "l": round(b["l"] / .25), "c": round(b["c"] / .25)} for b in bars]
for i in range(len(TB)):
    py = sorted(f"{p}{'o' if o else 't'}" for p, o in M.wpoc_lines([r for r in recs if r["tc"] <= TB[i]["t"]] if False else recs, TB[: i + 1], n))
    if py == cpp[TB[i]["t"]]: same += 1
    else:
        bad += 1
        if bad < 4: print("diff", i, py, cpp[TB[i]["t"]])
print(f"wpoc_lines parity: {same}/{len(TB)} bars identical ({len(recs)} waves)")
# windows on 10-30: at every 10-30 session bar, the windows covering it == the C++ set
W = M.wpoc_windows(None, "ES", "2026-10-30", 0.25, flex=d, bars=bars, recs=recs)
ok = tot = 0
for b in TB:
    if not (M.np.datetime64("2026-10-29T17:00").astype("datetime64[s]").astype(int) < b["t"] <= M.np.datetime64("2026-10-30T17:00").astype("datetime64[s]").astype(int)):
        continue
    tot += 1
    have = sorted({w["tick"] for w in W if w["t_from"] <= b["t"] < w["t_to"]})
    want = sorted({int(x[:-1]) for x in cpp[b["t"]]})
    ok += have == want
print(f"wpoc_windows 2026-10-30: {ok}/{tot} bars covered by exactly the C++ set; {len(W)} windows, e.g. {[ (w['name'], w['t_from'], w['t_to']) for w in W[:3]]}")
assert all(w['name'] == 'WPOC %.2f' % (w['tick'] * .25) for w in W), 'names WPOC <px> with the tick decimals'
sys.exit(0 if same == len(TB) and ok == tot and tot > 0 else 1)
