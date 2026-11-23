"""tests_turn/eff_ref.py - Effort of every candle, calculated apart from the plugin (pandas), for the 1.1.0 checks.
in : the fills of ev_ES.txt with an aggressor ("sec bs px size", bs 1 = buy / 0 = sell):  awk '$2==3 && $3!=2 {print $1, $3, $4, $5}' ev_ES.txt > fills_ES.txt
out: eff_ref_ES.csv  key (candle end, the mock's clock), top = buyers' Effort at the top of the candle, bot = sellers' at the bottom, L = price levels
     top = (buy lots per price level in the top 25% of the candle) / (per level in the rest); bot = the mirror with sell lots. Under 4 levels: none."""
import sys, pandas as pd, numpy as np
src, out = sys.argv[1], sys.argv[2]
d = pd.read_csv(src, sep=' ', header=None, names=['sec', 'bs', 'px', 'sz']); d['key'] = (d.sec // 180 + 1) * 180
rows = []
for key, g in d.groupby('key'):
    lo, hi = g.px.min(), g.px.max(); L = hi - lo + 1
    if L < 4: rows.append((key, np.nan, np.nan, L)); continue
    z = max(1, (L + 2) // 4)
    b = g[g.bs == 1].groupby('px').sz.sum(); s = g[g.bs == 0].groupby('px').sz.sum()
    bt, br = b[b.index > hi - z].sum(), b[b.index <= hi - z].sum(); st, sr = s[s.index < lo + z].sum(), s[s.index >= lo + z].sum()
    rows.append((key, (bt / z) / (br / (L - z)) if br > 0 else np.nan, (st / z) / (sr / (L - z)) if sr > 0 else np.nan, L))
pd.DataFrame(rows, columns=['key', 'top', 'bot', 'L']).to_csv(out, index=False); print('ok', len(rows), 'candles')
