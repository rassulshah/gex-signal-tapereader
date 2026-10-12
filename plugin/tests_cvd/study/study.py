"""study.py -- does the CVD signal set work? (lsCVD 1.0.0, real IRT 3-min bars Aug-Oct 2026, 7 markets).

For every signal: enter at the close of the bar that printed it, stop = the frozen invalidation price, target 2R, 20-bar time stop,
same session only; 1 tick of cost each way. Rows: confirmed (filled label) and warnings (the '?' label, entered at once) so we can
see whether waiting for confirmation helps. Baseline: every bar that touched one of the same levels (tolerance as the engine),
traded in the bounce direction with the same stop distance rule - is CVD adding anything over just 'trade the level'?
"""
import sys, json
import numpy as np, pandas as pd

TICK = {'ES': .25, 'NQ': .25, 'CL': .01, 'GC': .1, 'HG': .0005, 'NG': .001, 'EU': .00005}
H, TARGET = 20, 2.0


def outcome(bars, i, side, entry, stop, tick):
    risk = abs(entry - stop)
    if risk <= 0:
        return np.nan
    sess = bars.session.iat[i]
    cost = 2 * tick / risk
    for k in range(i + 1, min(i + 1 + H, len(bars))):
        if bars.session.iat[k] != sess:
            k -= 1
            break
        lo, hi = bars.l.iat[k], bars.h.iat[k]
        if side > 0:
            if lo <= stop: return -1 - cost
            if hi >= entry + TARGET * risk: return TARGET - cost
        else:
            if hi >= stop: return -1 - cost
            if lo <= entry - TARGET * risk: return TARGET - cost
    last = bars.c.iat[min(k, len(bars) - 1)]
    return side * (last - entry) / risk - cost


def main(outdir):
    rows, base = [], []
    for m, tick in TICK.items():
        b = pd.read_csv(f'{outdir}/{m}_bars.csv')
        e = pd.read_csv(f'{outdir}/{m}_events.csv')
        idx = {r: i for i, r in enumerate(b.raw)}
        for _, x in e.iterrows():
            if x.state not in ('pending', 'confirmed'):
                continue
            i = idx[x.raw]; side = 1 if x.dir == 'bull' else -1
            rows.append(dict(market=m, type=x.type, dir=x.dir, state=x.state, session=b.session.iat[i],
                             r=outcome(b, i, side, b.c.iat[i], x.invalidation, tick)))
        # baseline: first touch per level per session of the same levels the engine saw (from the event log level ids is too
        # sparse) -> use the session's prior-day high/low proxy: every bar whose range touches the prior session RTH high/low
        b['t'] = pd.to_datetime(b.raw, unit='s')
        sessions = b.groupby('session')
        prev = None
        for s, g in sessions:
            if prev is not None and s > 0:
                for lvl, side in ((prev.h.max(), -1), (prev.l.min(), 1)):
                    rng = (g.h - g.l).median()
                    tol = max(tick, round(3 * (rng / tick) / 15) * tick)
                    hit = g[(g.l <= lvl + tol) & (g.h >= lvl - tol)]
                    if len(hit):
                        i = idx[hit.raw.iat[0]]
                        stop = (b.l.iat[i] - max(tick, round(2 * (rng / tick) / 15) * tick)) if side > 0 else (b.h.iat[i] + max(tick, round(2 * (rng / tick) / 15) * tick))
                        base.append(dict(market=m, side=side, r=outcome(b, i, side, b.c.iat[i], stop, tick)))
            prev = g
    d = pd.DataFrame(rows); bs = pd.DataFrame(base)
    out = {}
    def summ(x):
        x = x.dropna()
        return dict(n=int(len(x)), avgR=round(float(x.mean()), 3) if len(x) else None,
                    win=round(float((x > 0).mean()), 3) if len(x) else None,
                    t=round(float(x.mean() / (x.std(ddof=1) / np.sqrt(len(x)))), 2) if len(x) > 2 and x.std() > 0 else None)
    out['baseline_first_touch_PD_high_low'] = summ(bs.r)
    for st in ('pending', 'confirmed'):
        out[st] = summ(d[d.state == st].r)
        for t in ('Div', 'Abs', 'Exh', 'BR'):
            out[f'{st}:{t}'] = summ(d[(d.state == st) & (d.type == t)].r)
    for m in TICK:
        out[f'confirmed:{m}'] = summ(d[(d.state == 'confirmed') & (d.market == m)].r)
    # halves (first half of sessions vs second) - does it hold out of sample?
    c = d[d.state == 'confirmed'].copy()
    med = c.session.median()
    out['confirmed:first_half'] = summ(c[c.session <= med].r); out['confirmed:second_half'] = summ(c[c.session > med].r)
    d.to_csv(f'{outdir}/study_trades.csv', index=False)
    print(json.dumps(out, indent=1))


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else 'out')
