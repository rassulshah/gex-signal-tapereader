"""Parity: ExhaustLogic.h decide() vs the study's own mbo_study.exh_at_zone (copied verbatim below) on 400 random visits.
usage: python3 parity.py DUMPDIR   (DUMPDIR from ./test_ex dump DUMPDIR)"""
import math, sys
import numpy as np, pandas as pd
NS = 1_000_000_000
DEFENCE_S = 180

def exh_at_zone(tr_t, tr_pt, tr_sd, tr_sz, c: dict, tick: float) -> dict:      # verbatim from analytics/lra/mbo_study.py
    out = {"exh_zone": None, "zone_secs_share": None}
    sup = c["side"] == "support"
    lo, hi = int(math.floor(c["lo"] / tick + 1e-9)), int(math.ceil(c["hi"] / tick - 1e-9))
    ta, td = c.get("t_arr_ns"), c.get("t_dec_ns")
    if ta is None or td is None or td <= ta or not len(tr_t):
        return out
    a, b = np.searchsorted(tr_t, ta), np.searchsorted(tr_t, td, side="right")
    if b <= a:
        return out
    t, pt, sd, sz = tr_t[a:b], tr_pt[a:b], tr_sd[a:b], tr_sz[a:b]
    aside = 1 if sup else 0
    nsec = int((td - ta) // NS) + 1
    k = ((t - ta) // NS).astype(np.int64)
    last = np.full(nsec, np.nan)
    last[k] = pt
    last = pd.Series(last).ffill().to_numpy()
    at = (last >= lo - 1) & (last <= hi + 1)
    att = np.bincount(k[(pt >= lo) & (pt <= hi) & (sd == aside)], weights=sz[(pt >= lo) & (pt <= hi) & (sd == aside)], minlength=nsec)[:nsec]
    h = nsec // 2
    s1, s2 = at[:h].sum(), at[h:].sum()
    out["zone_secs_share"] = round(float(at.mean()), 3)
    if s1 >= 5 and s2 >= 5 and att[:h].sum() > 0:
        out["exh_zone"] = round(float((att[h:].sum() / s2) / (att[:h].sum() / s1)), 3)
    return out

d = sys.argv[1]
V = pd.read_csv(f"{d}/visits.csv"); C = pd.read_csv(f"{d}/cpp.csv")
bad = n = ex = 0
for _, r in C.iterrows():
    v = V[V.case == r.case]
    t = v.t_ms.to_numpy(np.int64) * 1_000_000; pt = v.px.to_numpy(float); sd = v.aggr.to_numpy(); sz = v.lots.to_numpy(float)
    inz = np.nonzero((pt >= r.lo) & (pt <= r.hi))[0]
    ta = int(t[inz[0]]); assert ta == int(r.ta) * 1_000_000, (r.case, ta, r.ta)          # the same arrival
    o = exh_at_zone(t, pt, sd, sz, {"side": r.side, "lo": r.lo, "hi": r.hi, "t_arr_ns": ta, "t_dec_ns": ta + DEFENCE_S * NS}, 1.0)
    pe = -1.0 if o["exh_zone"] is None else o["exh_zone"]
    pex = o["exh_zone"] is not None and o["exh_zone"] <= 0.164 and o["zone_secs_share"] >= 0.5
    n += 1; ex += pex
    if abs(pe - r.exh) > 1e-9 or abs(o["zone_secs_share"] - r.share) > 1e-9 or int(pex) != int(r.is_ex):
        bad += 1; print("MISMATCH", r.case, pe, r.exh, o["zone_secs_share"], r.share, pex, r.is_ex)
print(f"parity: {n} visits, {ex} Ex, {bad} mismatches")
sys.exit(1 if bad or n < 300 else 0)
