"""fill_parity.py (lsLiquidityProfile 0.54.0 / LmtZonesLogic 1.3.0) -- the live AFTER-FILL READ vs the research H6-FILL rows.
For each market / session: the recorded Databento MBO -> conv_mbo.py -> replay_lmz (the C++ engine, LMZ_AFTER_OUT) -> the engine's
decided fills (order id, RE-BID / ACCEPTED, stack, delta). Compared with iter1024/iter6 res/lmt6_<MKT>.parquet FILL rows (raw, the same
order ids): Jaccard of the decided fills (engine fills the research labeller keeps, i.e. not resolved before the decision / not OPEN),
agreement of the read (RE-BID vs the research BOTH flag with the same per-market stack line), and the failed-sweep rate by read.
usage: python3 fill_parity.py <workdir> <replay_bin> MKT:session[,session...] ...   (sessions 'all' = every research session)"""
import sys, os, glob, subprocess, json, time
import numpy as np, pandas as pd
HERE = os.path.dirname(os.path.abspath(__file__))
ST = '/mnt/user-data/uploads/level-reversal-analytics/data/mbo/store'
IT6 = '/mnt/user-data/outputs/iter1024/iter6'
sys.path.insert(0, IT6 + '/code')
import lmt6_indep as L6                     # the research's own bars / ATR / labeller (same code path as the FILL rows)
import duckdb
NB = {'CL': 0.0102, 'ES': 0.0144, 'EU': 0.0114, 'GC': -0.0076, 'HG': 0.0149, 'NG': 0.0414, 'NQ': -0.0068}
wd, rbin = sys.argv[1], sys.argv[2]; os.makedirs(wd, exist_ok=True)
jobs = []
for a in sys.argv[3:]:
    m, ss = a.split(':'); ss = L6.sessions(m) if ss == 'all' else ss.split(',')
    jobs += [(m, s) for s in ss]
con = duckdb.connect(); con.sql("set threads=1; set memory_limit='1500MB'")
X = {}
rows = []
for m, s in jobs:
    t0 = time.time()
    af = f'{wd}/{m}_{s}_af.csv'
    if not os.path.exists(af):
        ev = f'{wd}/{m}_{s}.csv'
        subprocess.run([sys.executable, '-I', f'{HERE}/conv_mbo.py', f'{ST}/raw/m={m}/session={s}/part.parquet', ev], check=True, capture_output=True)
        r = subprocess.run([rbin, ev, m, '20'], capture_output=True, text=True, env=dict(os.environ, LMZ_AFTER_OUT=af))
        open(f'{wd}/{m}_{s}_replay.txt', 'w').write(r.stdout)
        os.remove(ev)
    A = pd.read_csv(af)
    if m not in X:
        R = pd.read_parquet(f'{IT6}/res/lmt6_{m}.parquet'); X[m] = R[R.kind == 'FILL'].copy()
    F = X[m][X[m].sess == s].copy()
    F['both'] = (F.nb >= NB[m]) & (F.dpos == 1)
    zo = set(A.oid)
    B = F[F.oid.isin(set(A[A.read != 'DUP'].oid))]   # one read per side / price per session is a display rule: zones the engine skipped as a repeat are left out of both sides
    D = A[A.read.isin(['RE-BID', 'ACCEPTED'])].copy()
    # the research labeller on the engine's decided fills (its own trades / bars / ATR): drop OPEN and resolved-before-decision rows
    ts, pt, sz, sd = L6.load_trades(con, m, s); Bb = L6.bars3(ts, pt, sz, s)
    keep, fail = [], []
    for r in D.itertuples():
        sg = 1 if r.side == 'offer' else -1; f_ns = (int(r.fill_ms) + 5 * 3600 * 1000) * 10**6; dec = f_ns + 30 * 10**9
        atr = L6.atr_at(Bb, (int(r.t0) + 5 * 3600 * 1000) * 10**6); w = max(1.0, 0.25 * atr)
        lab, tb, tac, ext = L6.label(ts, sg * pt, Bb, f_ns, sg * r.px, w, atr, sg)
        keep.append(lab != 'OPEN' and min(tb, tac) > dec); fail.append(int(lab in ('REVERSE_AT', 'SWEPT')))
    D['elig'] = keep; D['fail_lab'] = fail
    # the research loads raw rows only from RTH0 - 90 min (lmt6_indep t_r0): an order added before that has no FILL row there by design
    t_r0 = L6.ct_ns(s, L6.RTH0[m] - 90)
    if len(D):
        ol = ','.join(str(int(o)) for o in D.oid)
        fs = con.sql(f"select oid, min(ts) as t from read_parquet('{ST}/raw/m={m}/session={s}/*.parquet') where oid in ({ol}) and act in ('A','M') group by oid").df()
        first = dict(zip(fs.oid.astype('int64'), fs.t))
        D['pre'] = [first.get(int(o), 0) < t_r0 for o in D.oid]
    else: D['pre'] = []
    for r in D.itertuples():
        rows.append(dict(mkt=m, sess=s, oid=r.oid, side=r.side, read=r.read, stack=r.stack, delta=r.delta, adds=r.adds, cans=r.cans, elig=r.elig and not r.pre, pre=r.pre, fail_lab=r.fail_lab,
                         fill_ms=r.fill_ms, px=r.px, size=r.size, kind=r.kind, drawn=r.drawn))
    for r in B.itertuples():
        if r.oid not in set(D.oid):
            rows.append(dict(mkt=m, sess=s, oid=r.oid, side=r.side, read='-', elig=None, research_only=1))
    Bset = set(B.oid); Aset = set(D[D.elig].oid)
    both_agree = B.set_index('oid').both.to_dict()
    print(m, s, f'engine decided {len(D)} (eligible {len(Aset)}), research rows on zone orders {len(Bset)}, common {len(Aset & Bset)}, {time.time() - t0:.0f}s', flush=True)
P = pd.DataFrame(rows)
P.to_csv(f'{wd}/parity_rows.csv', index=False)
# summary
out = {}
Xall = pd.concat(X.values()); Xall['both'] = [(nb >= NB[mk]) & (dp == 1) for nb, dp, mk in zip(Xall.nb, Xall.dpos, Xall.mkt)]
E = P[P.read.isin(['RE-BID', 'ACCEPTED'])]
Ae = E[E.elig == True]
key = lambda d: set(zip(d.mkt, d.sess, d.oid))
Rk = set()
for (m, s), g in P.groupby(['mkt', 'sess']):
    pass
# research rows on the engine's zone orders = engine-decided-and-in-research + research_only
Rset = key(P[P.get('research_only', pd.Series(0, index=P.index)).fillna(0) == 1])
Xi = Xall.set_index(['mkt', 'sess', 'oid'])
inR = [k in Xi.index for k in zip(E.mkt, E.sess, E.oid)]
E = E.assign(inR=inR); Rset |= key(E[E.inR])
Aset = key(Ae)
inter = Aset & Rset; uni = Aset | Rset
out['jaccard_decided'] = round(len(inter) / max(len(uni), 1), 3)
out['n'] = dict(engine_pre_window=int(E.pre.fillna(False).astype(bool).sum()), engine_resolved_before_decision=int(((E.elig == False) & ~E.pre.fillna(False).astype(bool)).sum()), engine_decided=len(E), engine_eligible=len(Aset), research_rows_on_zone_orders=len(Rset), common=len(inter), engine_only=len(Aset - Rset), research_only=len(Rset - Aset))
C = E[E.inR & (E.elig == True)].copy()
C['r_both'] = [bool(Xi.loc[k].both if not isinstance(Xi.loc[k], pd.DataFrame) else Xi.loc[k].both.iloc[0]) for k in zip(C.mkt, C.sess, C.oid)]
C['r_fail'] = [int(Xi.loc[k].fail if not isinstance(Xi.loc[k], pd.DataFrame) else Xi.loc[k].fail.iloc[0]) for k in zip(C.mkt, C.sess, C.oid)]
C['e_rebid'] = C.read == 'RE-BID'
out['read_agreement_on_common'] = round(float((C.e_rebid == C.r_both).mean()), 3) if len(C) else None
out['read_confusion'] = C.groupby(['e_rebid', 'r_both']).size().reset_index().astype(str).values.tolist()
out['fail_rate_by_engine_read'] = C.groupby('read').r_fail.agg(['mean', 'size']).round(3).reset_index().astype(str).values.tolist()
out['fail_rate_by_engine_read_all_eligible'] = Ae.groupby('read').fail_lab.agg(['mean', 'size']).round(3).reset_index().astype(str).values.tolist()
out['fail_rate_by_research_both_same_rows'] = C.groupby('r_both').r_fail.agg(['mean', 'size']).round(3).reset_index().astype(str).values.tolist()
out['per_market'] = {m: dict(decided=int((E.mkt == m).sum()), rebid=int(((E.mkt == m) & (E.read == 'RE-BID')).sum()), agree=round(float((g.e_rebid == g.r_both).mean()), 3) if len(g) else None)
                     for m, g in [(mm, C[C.mkt == mm]) for mm in sorted(E.mkt.unique())]}
json.dump(out, open(f'{wd}/parity_summary.json', 'w'), indent=1)
print(json.dumps(out, indent=1))
