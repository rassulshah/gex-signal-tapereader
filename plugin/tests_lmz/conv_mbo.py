# conv_mbo.py -- a recorded Databento MBO session (data\mbo\store\raw parquet) -> the replay_lmz.cpp event file (python -I conv_mbo.py part.parquet out.csv)
import sys, pyarrow.parquet as pq, numpy as np
src, out = sys.argv[1], sys.argv[2]
d = pq.read_table(src).to_pandas()
ts = (d.ts.values // 1_000_000 - 5 * 3600 * 1000).astype(np.int64)
rem = {}
with open(out, 'w') as f:
    for t, a, s, pt, sz, oid in zip(ts, d.act.astype(str).values, d.side.astype(str).values, d.pt.values, d.sz.values, d.oid.values):
        oid = int(oid); sd = 0 if s == 'B' else 1 if s == 'A' else -1
        if a == 'A':
            rem[oid] = sz; f.write(f"E,{t},0,{sd},{pt},{sz},{oid}\n")
        elif a == 'M':
            if oid in rem: rem[oid] = sz
            f.write(f"E,{t},1,{sd},{pt},{sz},{oid}\n")
        elif a == 'C':
            r = rem.get(oid)
            if r is None: continue
            if sz >= r: f.write(f"E,{t},2,{sd},{pt},{r},{oid}\n"); rem.pop(oid, None)
            else: rem[oid] = r - sz; f.write(f"E,{t},1,{sd},{pt},{r - sz},{oid}\n")
        elif a == 'F':
            r = rem.get(oid)
            if r is None: continue
            f.write(f"E,{t},3,{1 - sd if sd >= 0 else -1},{pt},{sz},{oid}\n")
            if sz >= r: f.write(f"E,{t},2,{sd},{pt},0,{oid}\n"); rem.pop(oid, None)
            else: rem[oid] = r - sz
        elif a == 'T':
            f.write(f"T,{t},{pt},{sz}\n")
print('done', len(d))
