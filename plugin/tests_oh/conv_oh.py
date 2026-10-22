# conv_oh.py -- recorded Databento MBO (a daily .dbn.zst or a raw-store parquet) -> the replay_oh.cpp event file.
# Lines: "E,t_ct_ms,act,side,tick,size,oid" (NEW 0 / MOD 1 / DEL 2 / FILL 3; FILL side = the AGGRESSOR's side, as IRT sends it,
# followed by DEL size 0 when the resting order is used up) and "T,t_ct_ms,tick,lots" trades (for the bars).
# usage: python -I conv_oh.py <in.dbn.zst|part.parquet> <out.csv> <SYMBOL e.g. ESZ6 or -> <tick> <from HH:MM CT> <to HH:MM CT>
# The whole file is replayed for the book (orders resting before the window are known); only events inside the window are written.
import sys
src, out, sym, tick, t_from, t_to = sys.argv[1], sys.argv[2], sys.argv[3], float(sys.argv[4]), sys.argv[5], sys.argv[6]
CT = 5 * 3600 * 1000            # October: CDT = UTC-5
def hm(s): h, m = s.split(':'); return (int(h) * 60 + int(m)) * 60000
W0, W1 = hm(t_from), hm(t_to)
rem = {}; side_of = {}
n_out = 0
f = open(out, 'w')
def emit(t, a, sd, pt, sz, oid):
    global n_out
    if W0 <= t % 86400000 < W1: f.write(f"E,{t},{a},{sd},{pt},{sz},{oid}\n"); n_out += 1
def rows():
    if src.endswith('.parquet'):
        import pyarrow.parquet as pq
        pf = pq.ParquetFile(src)
        for b in pf.iter_batches(batch_size=1_000_000, columns=['ts', 'act', 'side', 'pt', 'sz', 'oid']):
            d = b.to_pandas()
            yield (d.ts.values // 1_000_000 - CT), d.act.astype(str).values, d.side.astype(str).values, d.pt.values, d.sz.values, d.oid.values
    else:
        import databento as db
        s = db.DBNStore.from_file(src)
        for d in s.to_df(count=1_000_000, pretty_ts=False, price_type='fixed', map_symbols=True):
            d = d[d.symbol == sym]
            pt = (d.price.values / 1e9 / tick).round().astype('int64')
            yield (d.ts_event.values.astype('int64') // 1_000_000 - CT), d.action.astype(str).values, d.side.astype(str).values, pt, d['size'].values, d.order_id.values
for ts, act, side, pts, szs, oids in rows():
    for t, a, s, pt, sz, oid in zip(ts, act, side, pts, szs, oids):
        t = int(t); oid = int(oid); sz = int(sz); sd = 0 if s == 'B' else 1 if s == 'A' else -1
        if a == 'R': rem.clear(); continue
        if a == 'A':
            rem[oid] = sz; emit(t, 0, sd, pt, sz, oid)
        elif a == 'M':
            if oid in rem: rem[oid] = sz
            emit(t, 1, sd, pt, sz, oid)
        elif a == 'C':
            r = rem.get(oid)
            if r is None: continue
            if sz >= r: emit(t, 2, sd, pt, r, oid); rem.pop(oid, None)
            else: rem[oid] = r - sz; emit(t, 1, sd, pt, r - sz, oid)
        elif a == 'F':
            r = rem.get(oid)
            if r is None: continue
            emit(t, 3, 1 - sd if sd >= 0 else -1, pt, sz, oid)
            if sz >= r: emit(t, 2, sd, pt, 0, oid); rem.pop(oid, None)
            else: rem[oid] = r - sz
        elif a == 'T':
            if W0 <= t % 86400000 < W1: f.write(f"T,{t},{pt},{sz}\n")
f.close()
print('done', out, n_out)
