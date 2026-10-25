# conv_mbo.py -- a recorded Databento MBO session (data\mbo\store\raw parquet) -> the replay_lmz.cpp event file (python -I conv_mbo.py part.parquet out.csv)
import sys, pyarrow.parquet as pq, numpy as np
src, out = sys.argv[1], sys.argv[2]
T = pq.read_table(src, columns=['ts', 'act', 'side', 'pt', 'sz', 'oid'])   # (0.54.0) column arrays, no DataFrame (memory)
ts = (T.column('ts').to_numpy() // 1_000_000 - 5 * 3600 * 1000).astype(np.int64)
ACT = T.column('act').to_pylist(); SIDE = T.column('side').to_pylist(); PT = T.column('pt').to_numpy(); SZ = T.column('sz').to_numpy(); OID = T.column('oid').to_numpy()
n_rows = len(ts); del T
rem = {}
# (0.54.0) mirrors the Databento rows: F = FILL only (the book is updated by the C / M rows that follow, as CME sends them); C = DEL of
# what remains (MOD on a partial cancel); M = MOD and re-arms the size (an iceberg reload after a fill keeps its id). The 0.53 converter
# wrote a synthetic DEL on a full fill and then turned the reload M into a phantom order that never ended (found by fill_parity.py).
with open(out, 'w') as f:
    for t, a, s, pt, sz, oid in zip(ts, ACT, SIDE, PT, SZ, OID):
        oid = int(oid); sd = 0 if s == 'B' else 1 if s == 'A' else -1
        if a == 'A':
            rem[oid] = sz; f.write(f"E,{t},0,{sd},{pt},{sz},{oid}\n")
        elif a == 'M':
            rem[oid] = sz
            f.write(f"E,{t},1,{sd},{pt},{sz},{oid}\n")
        elif a == 'C':
            r = rem.get(oid)
            if r is None: continue
            if sz >= r: f.write(f"E,{t},2,{sd},{pt},{max(r, 0)},{oid}\n"); rem.pop(oid, None)
            else: rem[oid] = r - sz; f.write(f"E,{t},1,{sd},{pt},{r - sz},{oid}\n")
        elif a == 'F':
            r = rem.get(oid)
            if r is None: continue
            f.write(f"E,{t},3,{1 - sd if sd >= 0 else -1},{pt},{sz},{oid}\n")
            rem[oid] = r - sz
        elif a == 'T':
            f.write(f"T,{t},{pt},{sz}\n")
print('done', n_rows)
