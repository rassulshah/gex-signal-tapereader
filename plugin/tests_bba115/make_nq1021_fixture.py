# make_nq1021_fixture.py (lsBigBidAsk 1.21.0) -- the NQ 2026-10-21 3-min candles the approved breakout mockup v2 was made from
# (/home/claude/pt/bo/bo.py): fp_bars.csv (1-min OHLCV) + fp_1m.csv (1-min cells) -> 3-min candles stamped at the bar END
# (ceil to 3 min, as IRT and bo.py). One line per candle: end|open|high|low|close|volume|buy_lots|buy_px|sell_lots|sell_px
# = the biggest BUY (at the ask) / SELL (at the bid) cell, ties to the price nearest the close (then higher for buy / lower for sell -
# bba::biggest's rule). Usage: python3 -I make_nq1021_fixture.py <data dir> > nq_2026-10-21_3m.txt
import sys, csv, collections
D = sys.argv[1].rstrip('/') + '/'
def key(ts):
    d, t = ts.split(' '); h, m, _ = t.split(':'); mm = int(h) * 60 + int(m); e = ((mm + 2) // 3) * 3
    return d, e
bars = collections.OrderedDict(); cells = collections.defaultdict(lambda: collections.defaultdict(lambda: [0, 0]))
with open(D + 'fp_bars.csv') as f:
    for r in csv.DictReader(f, delimiter='|'):
        k = key(r['bar'])
        b = bars.get(k)
        o, h, l, c, v = float(r['open']), float(r['high']), float(r['low']), float(r['close']), int(r['volume'])
        if b is None: bars[k] = [o, h, l, c, v]
        else: b[1] = max(b[1], h); b[2] = min(b[2], l); b[3] = c; b[4] += v
with open(D + 'fp_1m.csv') as f:
    for r in csv.DictReader(f, delimiter='|'):
        c = cells[key(r['bar'])][r['price']]; c[0] += int(r['bought']); c[1] += int(r['sold'])
print('end|open|high|low|close|volume|buy_lots|buy_px|sell_lots|sell_px')
for k, (o, h, l, c, v) in bars.items():
    d, e = k; hh, mm = divmod(e, 60)
    if hh == 24: continue  # (none in this file)
    best = []
    for side in (0, 1):
        bl, bp = 0, None
        for p, x in cells[k].items():
            lots, px = x[side], float(p)
            if lots <= 0: continue
            if lots > bl: bl, bp = lots, px; continue
            if lots < bl: continue
            dn, dp = abs(px - c), abs(bp - c)
            if dn < dp or (dn == dp and (px > bp if side == 0 else px < bp)): bp = px
        best.append((bl, bp if bp is not None else 0.0))
    print('%s %02d:%02d|%.2f|%.2f|%.2f|%.2f|%d|%d|%.2f|%d|%.2f' % (d, hh, mm, o, h, l, c, v, best[0][0], best[0][1], best[1][0], best[1][1]))
