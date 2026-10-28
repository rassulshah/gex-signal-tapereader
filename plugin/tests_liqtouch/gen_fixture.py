"""Fixtures for the LiqTouch.h parity test: real Databento MBO around chosen touches of the 10-28 study (touch_t_<M>.parquet, W=15),
converted to IRT semantics (A->NEW, M->MOD, C->DEL, F->FILL with the resting side); T rows dropped (IRT has no trade row - the FILL is the print).
Per case: window from the level's qualification start (episode t0 - 30 s) to the outcome (+10 min cap); the book at the window start
(only orders that are ever at the case price) as NEW rows; every fill (others aggregated per ms / price / side, id 0); the study's own ATR
as change points. Output: <name>.bin (rows '<qbbqqq': ms, act, side, px, sz, id) + cases.csv (expected values)."""
import sys, os, struct, numpy as np, pandas as pd
LF = "/tmp/claude-0/-home-claude/62b1afce-8372-5fe7-a188-ef315fa0ebcd/scratchpad/liqfull"; sys.path.insert(0, LF)
import eng2
from touch import own_atr
OUT = sys.argv[1]; os.makedirs(OUT, exist_ok=True)
SEC = "/tmp/claude-0/-home-claude/62b1afce-8372-5fe7-a188-ef315fa0ebcd/scratchpad/liq/out"
T = pd.read_parquet(f"{LF}/res/touch_t_ES.parquet"); T = T[T.W == 15]
CASES = [("stays_plus", "2026-10-22", 30720, 0, "13:03:28"), ("gone", "2026-10-22", 30720, 0, "13:04:20"), ("weak", "2026-10-22", 30744, 0, "13:01:23"),
         ("stays", "2026-10-22", 30784, 0, "12:48:13"), ("gone_plus", "2026-10-22", 30760, 1, "13:09:37"), ("stays_side1", "2026-10-22", 30764, 1, "13:09:37")]
sess = "2026-10-22"
ts, act, side, pt, sz, oid = eng2.load("ES", sess)
Sx = pd.read_parquet(f"{SEC}/sec_ES_{sess}.parquet", columns=["t", "last", "hi", "lo"])
st = Sx.t.values; last = Sx["last"].values.astype(float); last[last <= 0] = np.nan; last = pd.Series(last).ffill().bfill().values
hi = np.where(Sx.hi.values > 0, Sx.hi.values, last).astype(float); lo = np.where(Sx.lo.values > 0, Sx.lo.values, last).astype(float)
A = own_atr(last, hi, lo)
rows = []
for name, s, px, sd, hhmm in CASES:
    d = T[(T.sess == s) & (T.px == px) & (T.side == sd)].copy()
    d["ct"] = pd.to_datetime(d.t, utc=True).dt.tz_convert("America/Chicago").dt.strftime("%H:%M:%S")
    r = d[d.ct == hhmm].iloc[0]
    stt = int(np.searchsorted(st, r.t)); s0 = int(r.s0i)
    t0 = int(st[s0]); W0 = t0 - int(os.environ.get("LEAD", "120")) * 10**9   # the study keeps a stale qualify stamp while a level is out of range: start well before
    W1 = int(r.t) + 640 * 10**9
    # the outcome second (study touch_out from s1 = stt + 15): hold / broke time, to size the window
    sideSel = sd
    # orders ever at px on that side
    sideCode = 66 if sd == 0 else 65
    m_px = (pt == px) & (side == sideCode) & np.isin(act, [65, 77, 67, 70])
    ids = set(oid[m_px & (ts < W1)].tolist())
    pre = (ts < W0)
    # book at W0 for those ids: last A/M before W0 with no later C before W0
    msk = pre & np.isin(oid, list(ids)) & np.isin(act, [65, 77, 67])
    df = pd.DataFrame(dict(ts=ts[msk], act=act[msk], side=side[msk], pt=pt[msk], sz=sz[msk], oid=oid[msk]))
    book = df.groupby("oid").tail(1); book = book[(book.act != 67) & (book.sz > 0)]
    lastpx = int(last[np.searchsorted(st, W0)])  # Python last at the boundary W0 (the second ending at W0)
    win = (ts >= W0) & (ts <= W1)
    ev = []
    W0ms = W0 // 10**6
    for b in book.itertuples():
        ev.append((W0ms - 1, 0, 0 if b.side == 66 else 1, int(b.pt), int(b.sz), int(b.oid)))
    w_ts, w_act, w_side, w_pt, w_sz, w_oid = ts[win], act[win], side[win], pt[win], sz[win], oid[win]
    inset = np.isin(w_oid, list(ids))
    agg = {}
    for i in range(len(w_ts)):
        a = w_act[i]; ms = int(w_ts[i] // 10**6); sdn = 0 if w_side[i] == 66 else (1 if w_side[i] == 65 else -1)
        if inset[i]:
            if a == 65: ev.append((ms, 0, sdn, int(w_pt[i]), int(w_sz[i]), int(w_oid[i])))
            elif a == 77: ev.append((ms, 1, sdn, int(w_pt[i]), int(w_sz[i]), int(w_oid[i])))
            elif a == 67: ev.append((ms, 2, sdn, int(w_pt[i]), int(w_sz[i]), int(w_oid[i])))
            elif a == 70: ev.append((ms, 3, sdn, int(w_pt[i]), int(w_sz[i]), int(w_oid[i])))
        elif a == 70 and sdn >= 0 and w_pt[i] > 0:
            k = (ms, sdn, int(w_pt[i]))
            if k in agg: agg[k][4] += int(w_sz[i])
            else: agg[k] = [ms, 3, sdn, int(w_pt[i]), int(w_sz[i]), 0, i]; ev.append(agg[k])
    ev = [tuple(e[:6]) for e in ev]
    with open(f"{OUT}/{name}.bin", "wb") as f:
        for e in ev: f.write(struct.pack("<qbbqqq", *e))
    # ATR change points (study own ATR, per second from W0 to W1)
    i0 = int(np.searchsorted(st, W0)); i1 = min(len(st) - 1, int(np.searchsorted(st, W1)))
    cps = []; prev = None
    for i in range(i0, i1 + 1):
        if A[i] != prev: cps.append(f"{st[i] // 10**6}:{A[i]:.10g}"); prev = A[i]
    rows.append(dict(name=name, W0=W0ms, last=lastpx, px=px, side=sd, tMs=int(r.t) // 10**6, szT=int(r.szT), szmax=int(r.szmax), pull60=float(r.pull60),
                     a=int(r.a), e=int(r.e), p=int(r.p), s=int(r.s), dwell=int(r.dwell), g=float(r.g), opp=float(r.opp), state=r.state, hold=int(r.hold),
                     A=float(r.A), age=int(r.age), nev=len(ev), atr=";".join(cps)))
    print(name, len(ev), os.path.getsize(f"{OUT}/{name}.bin"), r.state, r.dwell, flush=True)
pd.DataFrame(rows).to_csv(f"{OUT}/cases.csv", index=False)
