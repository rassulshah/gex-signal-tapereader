# reversal-first (1 ATR of 3-min candles) for each St: from the fire (St price) and FROM THE LIVE DECISION MOMENT (last trade then)
# usage: brackets2.py <variant dir>   -> prints per market absorbed vs not, n, counts per day
import pandas as pd, numpy as np, sys, glob
V=sys.argv[1]; rows=[]; days={}
for f in sorted(glob.glob(f'{V}/ev-*.csv')):
    tag=f.split('ev-')[1][:-4]; m=tag[:2]; tick={'ES':.25,'NQ':.25,'CL':.01,'GC':.1}[m]
    days.setdefault(m,set()).add(tag)
    E=pd.read_csv(f)
    if E.empty: continue
    T=pd.read_csv(f'{tag}.csv',header=None,names=['ms','sec','act','side','px','sz','oid'],usecols=[0,2,4]); T=T[T.act=='T']
    ms=T.ms.values; px=T.px.values
    for _,e in E.iterrows():
        atr=e.atr_ticks*tick
        if not atr>0: continue
        rev=1 if e.side=='down' else -1
        def first(i0,p0,k=1):
            seg=px[i0:i0+400000]
            up=np.where(rev*(seg-p0)>=k*atr)[0]; dn=np.where(rev*(seg-p0)<=-k*atr)[0]
            a=up[0] if len(up) else 10**12; b=dn[0] if len(dn) else 10**12
            return 'rev' if a<b else ('cont' if b<a else 'none')
        i0=np.searchsorted(ms,e.fired_ms,side='right'); i1=np.searchsorted(ms,e.decided_ms,side='right')
        rows.append(dict(mkt=m,tag=tag,outcome=e.outcome,kind=e.ref_kind,ref=e.ref,f1=first(i0,e.st_px),d1=first(i1,e.decided_px),
                         dsec=e.decided_sec-e.fired_sec,rth=510<=e.minute<900))
R=pd.DataFrame(rows); R.to_csv(f'{V}/brackets.csv',index=False)
def pr(x):
    d=(x!='none').sum(); r=(x=='rev').sum(); return f"{r}/{d}={100*r/max(d,1):.0f}%"
for m in ['ES','NQ','CL','GC']:
    g=R[R.mkt==m]
    if g.empty: continue
    nd=len(days[m]); ab=(g.outcome=='absorbed').mean()
    print(f"{m}: {len(g)} St in {nd} days = {len(g)/nd:.1f}/day (RTH {g.rth.sum()/nd:.1f}/day); absorbed {100*ab:.0f}%; key {100*(g.kind==0).mean():.0f}% pivot {100*(g.kind==1).mean():.0f}%; median decide {g.dsec.median():.0f}s")
    for o in ['absorbed','not']:
        h=g[g.outcome==o]
        print(f"   {o:9s} n={len(h):3d}  rev-first 1ATR from St {pr(h.f1):>12s} | FROM DECISION {pr(h.d1):>12s}")
A=R
print('ALL markets:'); 
for o in ['absorbed','not']:
    h=A[A.outcome==o]; print(f"   {o:9s} n={len(h):3d}  from St {pr(h.f1)} | FROM DECISION {pr(h.d1)}")
