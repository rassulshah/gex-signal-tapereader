# conv_qm.py -- NQ raw MBO parquet -> replay_qm event file (same IRT-style mapping as tests_lmz/conv_mbo.py), cut at a CT time
import sys, pyarrow.parquet as pq, numpy as np
src, out, cut_ct = sys.argv[1], sys.argv[2], sys.argv[3]   # cut_ct "HH:MM" on the session date (CDT = UTC-5)
d = pq.read_table(src, columns=['ts','act','side','pt','sz','oid']).to_pandas()
ts = (d.ts.values // 1_000_000 - 5 * 3600 * 1000).astype(np.int64)
hh, mm = map(int, cut_ct.split(':'))
day0 = (ts[-1] // 86400000) * 86400000
cut = day0 + (hh * 3600 + mm * 60) * 1000
n = int(np.searchsorted(ts, cut))
rem = {}
act = d.act.astype(str).values[:n]; side = d.side.astype(str).values[:n]; pt = d.pt.values[:n]; sz = d.sz.values[:n]; oid = d.oid.values[:n]
with open(out, 'w') as f:
    w = f.write
    for i in range(n):
        a = act[i]; t = ts[i]
        if a == 'R': rem.clear(); w(f"R,{t}\n"); continue
        o = int(oid[i]); s = side[i]; sd = 0 if s == 'B' else 1 if s == 'A' else -1; p = pt[i]; z = sz[i]
        if a == 'A':
            rem[o] = z; w(f"E,{t},0,{sd},{p},{z},{o}\n")
        elif a == 'M':
            rem[o] = z   # (a re-priced order after a full-size fill row must stay known: the 08:44:38 NQ order 6883242054741)
            w(f"E,{t},1,{sd},{p},{z},{o}\n")
        elif a == 'C':
            r = rem.get(o)
            if r is None: continue
            if z >= r: w(f"E,{t},2,{sd},{p},{r},{o}\n"); rem.pop(o, None)
            else: rem[o] = r - z; w(f"E,{t},1,{sd},{p},{r - z},{o}\n")
        elif a == 'F':
            r = rem.get(o)
            if r is None: w(f"E,{t},3,{1 - sd if sd >= 0 else -1},{p},{z},{o}\n"); continue
            w(f"E,{t},3,{1 - sd if sd >= 0 else -1},{p},{z},{o}\n")
            if z >= r: w(f"E,{t},2,{sd},{p},0,{o}\n"); rem.pop(o, None)
            else: rem[o] = r - z
print('done', n)
