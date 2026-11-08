# parity_zd.py PARQUET MARKET JOURNAL - an independent Python replay of the raw MBO checks the engine's decided numbers:
#   at each touch (ZT): the lots resting in the zone, the queue's 60-s cancel share (orders rested >= 30 s, zone or 1 tick behind)
#   at each reload decision (ZR): the attacker lots traded inside the zone in the 60 s, whether price broke 2+ ticks through
# Prints the matches; exit 1 on any mismatch. (Python logic written from the definitions, not from the C++ code.)
import sys, duckdb, collections
f, mkt, jpath = sys.argv[1:4]
ZN, ZT, ZR = {}, {}, {}
for ln in open(jpath):
    p = ln.strip().split(',')
    if p[0] == 'ZN': ZN[int(p[1])] = dict(side=int(p[3]), lo=int(p[4]), hi=int(p[5]))
    elif p[0] == 'ZT': ZT[int(p[1])] = dict(t=int(p[2]), at=int(p[3]), c60=float(p[4]), n60=int(p[6]))
    elif p[0] == 'ZR': ZR[int(p[1])] = dict(t=int(p[2]), v=int(p[3]), sold=int(p[4]))
need = sorted([(z['t'], 'T', i) for i, z in ZT.items()])
ev = duckdb.sql(f"""SELECT ts // 1000000 t, CASE act WHEN 'A' THEN 0 WHEN 'M' THEN 1 WHEN 'C' THEN 2 WHEN 'F' THEN 3 ELSE 9 END a,
    CASE side WHEN 'B' THEN 0 WHEN 'A' THEN 1 ELSE -1 END s, pt, sz, oid FROM '{f}' WHERE act IN ('A','M','C','F','R') ORDER BY ts""").fetchall()
book = {}; ended = collections.defaultdict(list); fills = []   # fills: (t, px, sz, restingSide)
ni = 0; atT = {}; q60 = {}; t_last = 0
def resting(side, lo, hi): return sum(o[2] for o in book.values() if o[0] == side and lo <= o[1] <= hi)
def queue(side, lo, hi, tt):
    qlo, qhi = (lo - 1, hi) if side == 0 else (lo, hi + 1)
    v = [(o[3], 1 << 62, False) for o in book.values() if o[0] == side and qlo <= o[1] <= qhi]
    for p in range(qlo, qhi + 1): v += [(a, e, c) for (a, e, c) in ended[(side, p)] if tt - e <= 17 * 60000]
    r = [x for x in v if x[0] <= tt - 90000 and x[1] > tt - 60000]
    return (sum(1 for x in r if x[1] <= tt and x[2]) / len(r) if r else -1.0), len(r)
for (t, a, s, pt, sz, oid) in ev:
    t = max(t, t_last); t_last = t
    while ni < len(need) and need[ni][0] <= t:      # the touch happens AT a fill: checked before the next event (the engine reads the book at the touch fill)
        ni += 1
    if a == 0 and s >= 0 and pt > 0: book[oid] = [s, pt, sz, t]
    elif a == 1:
        o = book.get(oid)
        if o is None:
            if s >= 0 and pt > 0: book[oid] = [s, pt, sz, t]
        else:
            if pt > 0 and pt != o[1]: o[1] = pt; o[3] = t
            if sz > 0: o[2] = sz
    elif a == 2:
        o = book.pop(oid, None)
        if o: ended[(o[0], o[1])].append((o[3], t, True))
    elif a == 3:
        o = book.get(oid); side = s; px = pt
        if o:
            side = o[0]; px = px if px > 0 else o[1]; o[2] -= sz
            if o[2] <= 0: book.pop(oid); ended[(o[0], o[1])].append((o[3], t, False))
        if side >= 0 and px > 0:
            fills.append((t, px, sz, side))
            for i, z in ZT.items():                     # the touch fill: read the book right after it, as the engine does
                if i not in atT and z['t'] == t:
                    n = ZN[i]
                    if (n['side'] == 0 and px <= n['hi']) or (n['side'] == 1 and px >= n['lo']):
                        atT[i] = resting(n['side'], n['lo'], n['hi']); q60[i] = queue(n['side'], n['lo'], n['hi'], t)
ok = bad = 0
for i, z in ZT.items():
    n = ZN[i]
    if i not in atT: print('no touch replayed', i); bad += 1; continue
    good = atT[i] == z['at'] and abs(q60[i][0] - z['c60']) < 0.0015 and q60[i][1] == z['n60']
    if not good: print('touch', i, 'engine', z, 'python', atT[i], q60[i]); bad += 1
    else: ok += 1
    if i in ZR:
        t0, t1 = z['t'], z['t'] + 60000; att = 0 if n['side'] == 0 else 1
        sold = sum(sz for (t, px, sz, side) in fills if t0 <= t <= t1 and side == n['side'] and n['lo'] <= px <= n['hi'])
        broke = any((px <= n['lo'] - 2) if n['side'] == 0 else (px >= n['hi'] + 2) for (t, px, sz, side) in fills if t0 <= t < t1)
        if sold == ZR[i]['sold'] and broke == (ZR[i]['v'] == 0): ok += 1
        else: print('reload', i, 'engine', ZR[i], 'python sold', sold, 'broke', broke); bad += 1
print(f'{mkt}: parity {ok} matched, {bad} mismatched ({len(ZT)} touches, {len(ZR)} reloads)')
sys.exit(1 if bad else 0)
